// Wifi credentials
#include <credentials.h>

// Metro library for timed events
#include <Metro.h>
Metro publishTimer = Metro(300);

// DFPlayer
#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>
SoftwareSerial mySerial(D3, D4); // RX, TX

// esphelper
#include "ESPHelper.h"
#include <strings.h>
#include <time.h>
netInfo homeNet = {  
          .mqttHost = "192.168.1.76",      //can be blank if not using MQTT
          .mqttUser = "",   //can be blank
          .mqttPass = "",   //can be blank
          .mqttPort = 1883,         //default port for MQTT is 1883 - only chance if needed.
          .ssid = my_SSID, 
          .pass = my_PASSWORD
          };
ESPHelper myESP(&homeNet);

int randInt = 0;
int prevRandInt = 0;
int fadeAmount = 5;
int brightness = 0;
int eyes;
int owlState = false;
bool radarStatus = false;
bool mqttActive= false;
char* owlMp3Topic = "owl/mp3";
// this is the list of mp3 files corresponding to each player
// (the actual filenames are 0020.mp3 ... 0031.mp3)
char *players[]={
"20", 
"21", 
"22", 
"23", 
"24", 
"25", 
"26", 
"27", 
"28", 
"29",
"30",
"31"
};
int playNum = sizeof(players);
long player;
String currentPlayer;



void setup() {
  Serial.begin(9600);

    // esphelper
    Serial.println("Connecting to Wi-Fi");
    myESP.addSubscription(owlMp3Topic);  
    myESP.begin();
    myESP.setCallback(mqttCallback); 
    

    // initialise eyes
    pinMode(D1, OUTPUT);
    pinMode(D2, OUTPUT);
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);

    // dfplayer
    mySerial.begin (9600);
    mp3_set_serial (mySerial);  //set softwareSerial for DFPlayer-mini mp3 module 
    delay(1);                     // delay 1ms to set volume
    mp3_set_volume (30);          // value 0~30
    blink(3);
    
  
}

void loop() {

  if(myESP.loop() == FULL_CONNECTION && !mqttActive){
    Serial.println("connected");
    mqttActive= true;
    publishTimer.interval(8000);
    owlState = true;    
    mp3_play (19);
  };
  
  //Serial.println(millis() / 1000);

  unsigned long seed=seedOut(50);
  randomSeed(seed);

  // is the owl active?
  if(owlState){
    brightness = brightness + fadeAmount;
    if (brightness <= 0 || brightness >= 250) fadeAmount = -fadeAmount;
  }
  else{ // fade eyes to 0
    publishTimer.interval(300);
    brightness = brightness -5;
    if(brightness<5) {
      brightness=0;
      fadeAmount = 5;
      owlState = false;
    }
  }


  // reset the owl when timer has expired
  if(publishTimer.check()){
    owlState = false;

    // pir sensor code
    if((analogRead(A0)>1000) != radarStatus){
      radarStatus = !radarStatus;

      randInt = random(3,13);
      while(randInt == prevRandInt){
        randInt = random(3,13);
      }
      mp3_play (randInt);
      publishTimer.interval(20000);
      owlState = true;
    }
  }

  // control eyes
  eyes = brightness - 70;
  if(eyes<0)eyes=0;
  analogWrite(D1, eyes);
  analogWrite(D2, eyes);  
  delay(3);
}

// Blink Eyes
void blink(int repeat){
  for(int i=0; i<repeat; i++){
    digitalWrite(D1, HIGH);
    digitalWrite(D2, HIGH);     
    delay(100);
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);
    delay(900);
  }
}

// get a pretty random seed (based on https://www.instructables.com/id/Arduino-Random-Name-Generator/ )
unsigned int bitOut(void) {
  static unsigned long firstTime=1, prev=0;
  unsigned long bit1=0, bit0=0, x=0, port=0, limit=10;
  if (firstTime) {
    firstTime=0;
    prev=analogRead(port);
  }
  while (limit--) {
    x=analogRead(port);
    bit1=(prev!=x?1:0);
    prev=x;
    x=analogRead(port);
    bit0=(prev!=x?1:0);
    prev=x;
    if (bit1!=bit0)
      break;
  }
  return bit1;
}

unsigned long seedOut(unsigned int noOfBits) {
  unsigned long seed=0;
  for (int i=0;i<noOfBits;++i)
    seed = (seed<<1) | bitOut();
  return seed;
}

// the callback function for incoming MQTT traffic
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String payloadStr = String((char *)payload).substring(0,length);
  Serial.print("mqtt: ");
  if(String(topic) == owlMp3Topic){
    publishTimer.interval(5000);
    owlState = true;

    switch( payloadStr.toInt()){
      case 1:
        Serial.println("plant");
        mp3_play (1);
        break;
      case 2:
        Serial.println("visitor");
        mp3_play (2);
        break;
      case 99:
        {
          // here we pick a random player,
          // check if they have been picked before.
          // if so we repeat, otherwise we play that player's audio file.
          Serial.println("random number");
          bool unique = false;
          do{
            player = (random(playNum/sizeof(char*)));
            if(players[player] != ""){
              currentPlayer = players[player];
              Serial.println(currentPlayer);
              mp3_play (currentPlayer.toInt());
              players[player] = "";
              unique = true;
            }
            yield();
          }
          while (unique == false);
        }
        
        break;
      default:
        Serial.println("unknown:"+payloadStr);
        mp3_play (payloadStr.toInt());
        break;
    }  
  }
}
