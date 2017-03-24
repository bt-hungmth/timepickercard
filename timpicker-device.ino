
/**
 *  Created by Hungmth
 */

#include <Arduino.h>
#include "MFRC522.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
ESP8266WiFiMulti WiFiMulti;

int ledWifi = 4; // GPIO4 - D2
int ledSuccess = 5; // GPI05 - D1
#define RST_PIN 15 // RST-PIN for RC522 - RFID - SPI - Modul GPIO15 
#define SS_PIN  2  // SDA-PIN for RC522 - RFID - SPI - Modul GPIO2 
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

void dump_byte_array(String &cardID, byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    cardID+=buffer[i] < 0x10 ? " 0" : " ";
    cardID+=buffer[i], HEX;
  }
}


String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
}

void setup() {
    delay(1);                        // give RF section time to shutdown
    Serial.begin(9600);              // turn off ESP8266 RF
    //LED
    pinMode(ledWifi, OUTPUT);
    digitalWrite(ledWifi, LOW);
    pinMode(ledSuccess, OUTPUT);
    digitalWrite(ledSuccess, LOW);
    
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Run");
    WiFiMulti.addAP("be-tech", "be-tech153");
    Serial.flush();
}

void loop() {
  /**
   * GET
   * */
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        digitalWrite(ledSuccess, LOW);
        if ( ! mfrc522.PICC_IsNewCardPresent()) {
          delay(50);
          return;
        }
        // Select one of the cards
        if ( ! mfrc522.PICC_ReadCardSerial()) {
          delay(50);
          return;
        }
        String cardID;
        dump_byte_array(cardID, mfrc522.uid.uidByte, mfrc522.uid.size);
        String url = "http://127.0.0.1:3000/check?card="+urlencode(cardID);
        Serial.println(url);`
        HTTPClient http;
        //Serial.print("[HTTP] begin...\n");
        // configure traged server and url
        http.begin(url); //HTTP
        //Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String result = http.getString();
                Serial.println(result);
                if (result == "success") {
                  digitalWrite(ledSuccess, HIGH);
                  delay(500);
                }
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    } else {
      digitalWrite(ledSuccess, HIGH);  
    }
    digitalWrite(ledSuccess, LOW); 
    delay(1000);
}
