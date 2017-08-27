#include <SPI.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>

const char* ssid = "wifiname";
const char* password = "wifip";

SoftwareSerial rfid = SoftwareSerial(0, 2);

// variables to keep state
int readVal = 0; // individual character read from serial
unsigned int readData[10]; // data read from serial
int counter = -1; // counter to keep position in the buffer
char tagId[11]; // final tag ID converted to a string

int ledWifi = 4; // GPIO4 - D2
int ledSuccess = 5; // GPI05 - D1

void setup() {

  Serial.begin(9600);
  rfid.begin(9600);
  
  //LED
  pinMode(ledWifi, OUTPUT);
  digitalWrite(ledWifi, LOW);
  pinMode(ledSuccess, OUTPUT);
  digitalWrite(ledSuccess, LOW);
    
  Serial.println("RFID Ready to listen");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  digitalWrite(ledSuccess, HIGH);
  delay(500);
}


// convert the int values read from serial to ASCII chars
void parseTag() {
  int i;
  for (i = 0; i < 10; ++i) {
    tagId[i] = readData[i];
  }
  tagId[10] = 0;
}

// once a whole tag is read, process it
void processTag() {
  // convert id to a string
  parseTag();
  
  // print it
  printTag();

}

void printTag() {
  Serial.print("Tag value: ");
  Serial.println(tagId);
  
  String url = "http://163.44.150.246:3000/check?card=";
  url += tagId;
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK)
  {
      Serial.print("HTTP response code ");
      Serial.println(httpCode);
      String response = http.getString();
      Serial.println(response);
      if (response == "success") {
        digitalWrite(ledSuccess, HIGH);
        delay(1000);
      }
  }
  else
  {
     Serial.println("Error in HTTP request");
  }
  http.end();
}

// this function clears the rest of data on the serial, to prevent multiple scans
void clearSerial() {
  while (rfid.read() >= 0) {
    ; // do nothing
  }
}

void loop() {
  if (rfid.available() > 0) {
    // read the incoming byte:
    readVal = rfid.read();
    
    // a "2" signals the beginning of a tag
    if (readVal == 2) {
      counter = 0; // start reading
    } 
    // a "3" signals the end of a tag
    else if (readVal == 3) {
      // process the tag we just read
      processTag();
      
      // clear serial to prevent multiple reads
      clearSerial();
      
      // reset reading state
      counter = -1;
    }
    // if we are in the middle of reading a tag
    else if (counter >= 0) {
      // save valuee
      readData[counter] = readVal;
      
      // increment counter
      ++counter;
    } 
  }
  digitalWrite(ledSuccess, LOW);
}

