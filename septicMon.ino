#include <TMRpcm_PLDuino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <PLDuino.h>
#include <PLDTouch.h>
#include <PLDuinoGUI.h>
#include <using_namespace_PLDuinoGUI.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>
#include "utils.h"

#define VERSION "1.0.0"

Adafruit_ILI9341 tft = Adafruit_ILI9341(PLDuino::LCD_CS, PLDuino::LCD_DC);
PLDTouch touch(PLDuino::TOUCH_CS, PLDuino::TOUCH_IRQ);
TMRpcm tmrpcm;
Sd2Card card; bool card_initialized;
Label lblMsg("--", ILI9341_WHITE, ILI9341_WHITE);

void setClock();

int lowAirPin = 30;
int highLevelPin = 31;
int UVPin = 32;
int spareAlarm = 33;

void blinkLED()
{
  for(int i=0; i<3; ++i)
  {
    digitalWrite(PLDuino::LED_PIN, LOW); delay(200);
    digitalWrite(PLDuino::LED_PIN, HIGH); delay(200);
  }
}

void setup()
{
  // A convenient macro which prints start-up messages to both LCD and Serial.
  #define LOG(msg) { tft.println(msg); Serial.println(msg); }
  
  // Set pin modes and initialize stuff
  // NB: This line is necessary in all sketches which use PLDuino library stuff.
  PLDuino::init();
  
  // Show that we are starting
  blinkLED();
  
  // Power-on LCD and set it up
  PLDuino::enableLCD();
  tft.begin();
  tft.setRotation(3);

  // Setup serials. Serial2 is connected to ESP-02 Wi-Fi module.
  Serial.begin(9600);
  Serial2.begin(9600);
  
  // Print version info.
  tft.fillScreen(ILI9341_BLACK);
  LOG("SepticMon firmware v" VERSION ", built " __DATE__)
  LOG("")

  // We need to initialize SD card at startup!
  LOG("Initializing SD card...")
  card_initialized = card.init(SPI_HALF_SPEED, PLDuino::SD_CS);
  if (!SD.begin(PLDuino::SD_CS))
    LOG("ERROR: Can't initialize SD card!")

  // Initializing touch screen.
  LOG("Initializing touch...")
  touch.init(1);

  // Initializing real-time clock.
  LOG("Initializing RTC...")
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet)
    LOG("ERROR: Unable to sync with the RTC")
  
  // Setup speaker pin to play sounds.
  tmrpcm.speakerPin = 9;
  
  // Initialization is complete. 
  LOG("")
  LOG("Initialization complete.")
  LOG("")
  LOG("-- Touch to keep the log on screen --")
  delay(1500);
  while(touch.dataAvailable()) touch.read();
  tft.fillScreen(ILI9341_BLACK);

  //playSound(tmrpcm, "init.wav");
  
  showSepticStatus();
}

void initDisplay()
{

  bmpDraw(tft, "septic.bmp", 0, 0);
  tft.fillRoundRect(2, 0, 200, 35, 10, ILI9341_WHITE);
  
  lblMsg.setPositionAndSize(10, 3, 320, 28); 
  lblMsg.draw(tft);

}
void settings()
{

   tft.fillScreen(ILI9341_BLACK);
   setClock();
   initDisplay();
  
}

void showSepticStatus ()
{

  
  pinMode(lowAirPin, INPUT_PULLUP);
  pinMode(highLevelPin, INPUT_PULLUP);
  pinMode(UVPin, INPUT_PULLUP);
  pinMode(spareAlarm, INPUT_PULLUP);
  
  bool soundPlayed = false;
  unsigned long starttime = millis();
  int soundCounter = 5;

  initDisplay();  
  
  while(true)
  {
    if (touch.dataAvailable())
    {
      starttime = millis();
      
      Serial.println("ready");
      Point pt = touch.read();
      int x = pt.x;
      int y = pt.y;
      Serial.print("x = "); Serial.print(x); Serial.print("; "); Serial.print("y = "); Serial.println(y);
      settings();
      
    }

    bool alarmStatus = false; //Indicate if we have an alarm
    String msg;
    
    if (digitalRead(lowAirPin) == LOW)
    {
      msg = "Low Air Alarm";
      alarmStatus = true;
    }

    if (digitalRead(highLevelPin) == LOW)
    {
      msg = "High Level Alarm";
      alarmStatus = true;
    }

    if (digitalRead(UVPin) == LOW)
    {
      msg = "UV Alarm";
      alarmStatus = true;
    }

    if (digitalRead(spareAlarm) == LOW)
    {
      msg = "Spare Alarm";
      alarmStatus = true;
    }
    
     
    lblMsg.updateTextAndColor(
        (alarmStatus? msg: "System OK"),
        (alarmStatus? ILI9341_RED : ILI9341_BLACK), tft);
        
           
    delay(50);
  }
  
}


void loop(){}

