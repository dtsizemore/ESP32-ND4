//ESP-ND4
//20251111 - DTSizemore
//Replacement for the original Rabbit2000 processor in the Symmetricom ND-4 clock.

//Remove the existing Rabbit2000 controller board, and utilize the existing pin sockets for connection.
//+5v VCC is present so no additional regulation needed.
//Below are pins on the ND-4 Driver board and their respective connections


//LINE   ESP          ND-4 Driver
//-----+-------------+------------
// VCC = VCC      --- VCC
//MOSI = GPIO 23  --- PA2
//SCLK = GPIO 18  --- PA0
// /CS = GPIO 5   --- PA1

//Dependencies:
// ESP-WiFiSettings 3.9.2
// LedController 2.0.2 https://github.com/noah1510/LedController


#include <LedController.hpp>
#include <WiFi.h>
#include <DNSServer.h>
//#include <ESP32WebServer.h>
#include <WiFiSettings.h>
#include <SPIFFS.h>
#include <Ticker.h>


#include <strings_en.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>

#include <ArduinoOTA.h>
#include "math.h"
#include <time.h>


#define CS 5
#define Segments 1
#define digitsPerSegment 6
#define positionOffset 0

#define delayTime 500
#define shortDelay 500

//NTP Config Settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

bool shouldSaveConfig = false;
long timer = 0;
int t1Count = 0;
bool colons = true;
bool showTime = true;

Ticker updateTicker;

LedController<Segments,1> lc = LedController<Segments,1>();


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}


void printLEDTime()
{
  auto now  =time(nullptr);
  const tm* timeinfo = localtime(&now);

  lc.setDigit(0,0,(timeinfo->tm_hour/10U)%10,false);
  lc.setDigit(0,1,(timeinfo->tm_hour/1U)%10,colons); // Colons are tied to DP of Pos 2
  lc.setDigit(0,2,(timeinfo->tm_min/10U)%10,false);
  lc.setDigit(0,3,(timeinfo->tm_min/1U)%10,false);
  lc.setDigit(0,4,(timeinfo->tm_sec/10U)%10,false);
  lc.setDigit(0,5,(timeinfo->tm_sec/1U)%10,false);
  
}


//Routine for scrolling text across screen. 
//Still needs tweaks and cleaning, but it works.
void scrollText(const char* textBuf)
{
  char buf[6];
  
  //Initialize 'screen' buffer
  for (int l = 0; l<6; l++)
  {
    buf[l] = ' ';
  }

  //get input buffer length
  int sLen = strlen(textBuf);
  for (int s = 0; s < sLen; s++)
  {
    buf[0] = buf[1];
    buf[1] = buf[2];
    buf[2] = buf[3];
    buf[3] = buf[4];
    buf[4] = buf[5];
    if (textBuf[s] == '.')
    {
      buf[5] = '-';
    } else {
      buf[5] = textBuf[s];
    }

    lc.setChar(0,0,buf[0],false);
    lc.setChar(0,1,buf[1],false);
    lc.setChar(0,2,buf[2],false);
    lc.setChar(0,3,buf[3],false);
    lc.setChar(0,4,buf[4],false);
    lc.setChar(0,5,buf[5],false);
    delay(shortDelay);
  }

}


void writeArduinoOn7Segment() {
  lc.setChar(0,0,'a',false);
  delay(delayTime);
  lc.setRow(0,0,0x05);
  delay(delayTime);
  lc.setChar(0,0,'d',false);
  delay(delayTime);
  lc.setRow(0,0,0x1c);
  delay(delayTime);
  lc.setRow(0,0,B00010000);
  delay(delayTime);
  lc.setRow(0,0,0x15);
  delay(delayTime);
  lc.setRow(0,0,0x1D);
  delay(delayTime);
  lc.clearMatrix();
  delay(delayTime);
}

void scrollDigits() {
  for(int i=0; i<13; i++) {
    lc.setDigit(0,3,i,false);
    lc.setDigit(0,2,i+1,false);
    lc.setDigit(0,1,i+2,false);
    lc.setDigit(0,0,i+3,false);
    delay(delayTime);
  }
  lc.clearMatrix();
  delay(delayTime);
}


void setLEDs (unsigned long long number) {
  //the loop is used to split the given number and set the right digit on the Segments
  for(unsigned int i = 0; i < Segments*digitsPerSegment; i++) {
    unsigned long long divisor = 1;
    for(unsigned int j=0; j < i; j++) {
      divisor *= 10;
    }
    //lc.setChar(unsigned int segmentNumber, unsigned int digit, char value, boolean dp)
    byte num = number/divisor % 10;
    lc.setDigit(Segments-i/digitsPerSegment-1,i%digitsPerSegment+positionOffset,num,false);
  }

}

void setup() {
  //just make sure that the config is valid
  Serial.begin(115200);
  SPIFFS.begin(true);
  static_assert(positionOffset+digitsPerSegment<9,"invalid configuration");
 
  WiFi.mode(WIFI_STA);

  WiFiSettings.connect();

  //initilize a ledcontroller with a hardware spi and one row
  lc.init(CS);

  //disables all Digits by default
  for(unsigned int i = 0; i < Segments; i++) {
    for(unsigned int j = 0; j < 8; j++) {
      lc.setRow(i,j,0x00);
    }
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //Set them to the lowest brightness
  lc.setIntensity(2);
  lc.clearMatrix();
  writeArduinoOn7Segment();
  Serial.println(WiFi.localIP().toString().c_str());
  scrollText(WiFi.localIP().toString().c_str());

  updateTicker.attach_ms(100, []() {
    if (showTime) {
      t1Count ++;
      //Serial.println(t1Count);
      if (t1Count %5 == 0)
      {
        if (!colons) {
          
          colons = true;
          //Serial.println("colons on");
        } else {
          
          colons = false;
          //Serial.println("colons off");
        }
      }
      if (t1Count > 1000)
      {
        t1Count = 0;
      }
      
      printLEDTime();
    }
  });
  
}

void loop() {

  //Nothing here now since everything currently is tied to a Ticker
}
