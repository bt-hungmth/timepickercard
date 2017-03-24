var bodyParser    = require("body-parser")
var app     	  = require("express")()
var http    	  = require('http').Server(app)
var io      	  = require("socket.io")(http)
var mysql   	  = require('mysql')
var request       = require('request')

var config        = require('./config.json');
var card 		  = ""
var cardObject    = {}
var slackToken    = config.slackToken
var slackChannel  = config.betech_assistant
var slackUsername = config.slackUsername
var slackIconUrl  = config.slackIconUrl
// Connect to database
var db = mysql.createConnection({
    host: '127.0.0.1',
    user: config.dbuser,
    password: config.dbpass,
    database: config.dbname
});
// Log any errors connected to the db
db.connect(function(err) {
    if (err) console.log(err)
});

app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())

// Home page
app.get("/", function(req,res){
    res.sendFile(__dirname + '/index.html')
});

// API check-in
app.get('/check', function(req,res) {
	card = req.query.card
	sql = 'SELECT * FROM user_cards WHERE card_code = ? limit 1'
    db.query(sql,card,
      function(error, rows) {
        if (error) {
            console.log(error)
            return;
        }
        if (rows.length == 0) {
            console.log("This card does exist in system!")
            cardObject = {'status': 'error','card_code' : card}
            io.sockets.emit('card connected', cardObject)
        } else {
        	cardData = rows[0]
        	console.log("Card ID = "+cardData.id+"; Card Name = "+cardData.card_name)

          // Get user check list of all user for show on website
          getAllUserCheckListURL = 'SELECT user_check_in_lists.card_id, user_cards.card_name, user_check_in_lists.created_time FROM user_check_in_lists INNER JOIN user_cards on user_check_in_lists.card_id = user_cards.id ORDER BY user_check_in_lists.id DESC LIMIT 10'
          db.query(getAllUserCheckListURL, function(error, checkInLists) {
            if (error) {
              console.log(error)
              return;
            }
            io.sockets.emit('check in lists', checkInLists)
          });
          cardObject = {'status': 'success', 'card_no' : cardData.id, 'card_name' : cardData.card_name, 'card_email' : cardData.card_email}
			    io.sockets.emit('card connected', cardObject)
          // End get check list of all user and show on website

          // Get check list of this user
          getUserCheckListURL = 'SELECT * FROM user_check_in_lists WHERE card_id = ? AND DATE(created_time) = CURDATE()'
          db.query(getUserCheckListURL, cardData.id, function(error, userCheckInList) {
            if (error) {
              console.log(error)
              return;
            }

            if (userCheckInList.length > 1) {
              // Send request update message to slack
              lastUserCheckIn = userCheckInList[userCheckInList.length - 1]
              updateSlackText = cardData.card_name +'-san updated check out at ' + getCurrentTime('H:M')
              if (!lastUserCheckIn.slack_ts) {
                updateCheckTime(lastUserCheckIn.id);
              } else {
                request({
                  url: 'https://slack.com/api/chat.update?token=' + slackToken + '&ts=' + lastUserCheckIn.slack_ts + '&channel=' + lastUserCheckIn.slack_channel + '&text=' + updateSlackText + '&pretty=1',
                  json: true
                }, function(error, response, body) {
                  updateCheckTime(lastUserCheckIn.id);
                });
              }
              //End send request update message to slack
            } else {
              if (userCheckInList.length == 0) {
                // Send request message check in to slack
                slackText = cardData.card_name +'-san check in at ' + getCurrentTime('H:M')
                request({
                  url: 'https://slack.com/api/chat.postMessage?token=' + slackToken + '&channel=' + slackChannel + '&text=' + slackText +'&username=' + slackUsername + '&icon_url=' + slackIconUrl + '&pretty=1',
                  json: true
                }, function(error, response, body) {
                  if (body.ok) {
                    db.query('INSERT INTO user_check_in_lists SET ?', {'card_id' : cardData.id, 'slack_channel' : body.channel, 'slack_ts' : body.ts})
                  } else {
                    db.query('INSERT INTO user_check_in_lists SET ?', {'card_id' : cardData.id})
                  }
                });
                // End send request mesage check in to slack
              } else {
                // Send request message check out to slack
                slackText = cardData.card_name +'-san check out at ' + getCurrentTime('H:M')
                request({
                  url: 'https://slack.com/api/chat.postMessage?token=' + slackToken + '&channel=' + slackChannel + '&text=' + slackText +'&username=' + slackUsername + '&icon_url=' + slackIconUrl + '&pretty=1',
                  json: true
                }, function(error, response, body) {
                  if (body.ok) {
                    db.query('INSERT INTO user_check_in_lists SET ?', {'card_id' : cardData.id, 'check_type' : 1, 'slack_channel' : body.channel, 'slack_ts' : body.ts})
                  } else {
                    db.query('INSERT INTO user_check_in_lists SET ?', {'card_id' : cardData.id, 'check_type' : 1})
                  }
                });
                // End send request mesage check out to slack
              }
            }
          });
          // End get check list of this user
        }
      }
    )

	res.end("success")
});

io.sockets.on('connection', function(socket) {
  socket.on('addCard', function(inputName, inputEmail, inputCardCode) {
    var post = {'card_code' : inputCardCode, 'card_name': inputName, 'card_email': inputEmail}
    var query = db.query('INSERT INTO user_cards SET ?', post)
    io.sockets.emit('cardAlert','Register is success')
  })
})
http.listen(3000, function() {
  console.log("Started on PORT 3000")
})

function getCurrentTime(format) {
  datetime = require('node-datetime')
  dt = datetime.create()
  fomratted = dt.format(format)
  return fomratted
}

function updateCheckTime(user_check_in_id) {
  db.query('UPDATE user_check_in_lists SET created_time = NOW() WHERE id = ' + user_check_in_id);
}
