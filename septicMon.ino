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
#define FLASH_ESP_BAUD 9600

Adafruit_ILI9341 tft = Adafruit_ILI9341(PLDuino::LCD_CS, PLDuino::LCD_DC);
PLDTouch touch(PLDuino::TOUCH_CS, PLDuino::TOUCH_IRQ);
TMRpcm tmrpcm;
Sd2Card card; bool card_initialized;
Label lblMsg("--", ILI9341_WHITE, ILI9341_WHITE);

void setClock();

int lowAirPin = 30;
int highLevelPin = 31;
int UVPin = 32;
int spareAlarmPin = 33;
HardwareSerial &wifi = Serial2;

void blinkLED()
{
  for(int i=0; i<3; ++i)
  {
    digitalWrite(PLDuino::LED_PIN, LOW); delay(200);
    digitalWrite(PLDuino::LED_PIN, HIGH); delay(200);
  }
}

void wifiSetup()
{

 using namespace PLDuino;
  
 // Setup screen
 tft.fillScreen(ILI9341_BLACK);
 tft.setCursor(0, 0);
 tft.setTextColor(ILI9341_WHITE);
 tft.setTextSize(3);
 tft.println("Wi-Fi setup. Connect to SepticMon ip 192.168.4.1.");
 tft.setTextSize(1);
 tft.println();
 tft.print("Initializing ESP8266... ");

 // Initializing ESP module
 PLDuino::enableESP();
 wifi.begin(FLASH_ESP_BAUD);
 Serial.begin(FLASH_ESP_BAUD);
 
 tft.println("done.");
 tft.println();

 // Reset ESP
 digitalWrite(PLDuino::ESP_RST, LOW);
 delay(500);
 digitalWrite(PLDuino::ESP_RST, HIGH);
 delay(3000);

 
 // Skip its boot messages
 while(wifi.available())
    tft.write((char)wifi.read());
 tft.println();
 
 
 String recv = "";
 tft.println("Ready to setup wifi.");
 wifi.println("dofile(\"wifiConfig.lua\");");
 while(1)
 {
   if (wifi.available())
   {
      char ch = wifi.read();
      Serial.write(ch);
      tft.write((char)ch);
   }
   
   if (Serial.available()) wifi.write(Serial.read());
 }
 
 /*while(true)
  {
    if (wifi.available())
    {
      char ch = wifi.read();
      if (ch == 'C') recv = String();
      else recv += ch;
      
      while(recv.length() >= 2)
      {
        // Extract 2-char command
        String cmd = recv.substring(0, 2);
        // Remove this command from the buffer
        recv = recv.substring(2);
        
        Serial.print(cmd);
        // Print only "R*"/"r*" and "O*"/"o*" commands
        if (cmd != "ss")
          tft.print(cmd);

        // Execute the command
        processCommand(cmd, wifi);
      }
    }
  }*/
  
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

  LOG("Init wifi");
  // Initializing ESP module
  PLDuino::enableESP();
  
  wifi.begin(FLASH_ESP_BAUD);
  Serial.begin(FLASH_ESP_BAUD);
 
  // Reset ESP
  digitalWrite(PLDuino::ESP_RST, LOW);
  delay(500);
  digitalWrite(PLDuino::ESP_RST, HIGH);
  delay(3000);

  // Skip its boot messages
  while(wifi.available())
    tft.write((char)wifi.read());
  tft.println();
  wifi.println("dofile(\"wifiConnect.lua\");");
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
   wifiSetup();
   initDisplay();
  
}

void showSepticStatus ()
{

  
  pinMode(lowAirPin, INPUT_PULLUP);
  pinMode(highLevelPin, INPUT_PULLUP);
  pinMode(UVPin, INPUT_PULLUP);
  pinMode(spareAlarmPin, INPUT_PULLUP);
  
  bool soundPlayed = false;
  unsigned long starttime = millis();
  int soundCounter = 5;
  unsigned long timer = 1*60*1000; //every 1 minute
  long currentTimer = 0;
  bool backLight = 1;
  
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
      if (backLight)
      {
        Serial.print("Settings");
        settings();
      } else {
        Serial.print("Turn Backlight back on");
        pinMode(PLDuino::LCD_BACKLIGHT, OUTPUT); digitalWrite(PLDuino::LCD_BACKLIGHT, LOW);
        backLight = 1;
        currentTimer = 0;
      }
      
    }

    if (wifi.available())
    {
      char ch = wifi.read();
      Serial.write(ch);
    }

     
    bool alarmStatus = false; //Indicate if we have an alarm
    String msg;
    
    
    if (digitalRead(lowAirPin) == HIGH)
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

    if (digitalRead(spareAlarmPin) == LOW)
    {
      msg = "Spare Alarm";
      alarmStatus = true;
    }

    if (currentTimer++ > 1000 )
    {
       Serial.println("Timer Triggered");
       

       String sensorData;
       sensorData += "LowAir=";
       sensorData += digitalRead(lowAirPin) ? "1" : "0";
       
       sensorData += "&HighLevel=";
       sensorData += digitalRead(highLevelPin) ? "0" : "1";
       
       sensorData += "&UV=";
       sensorData += digitalRead(UVPin) ? "0" : "1";
       
       sensorData += "&Spare=";
       sensorData += digitalRead(spareAlarmPin) ? "0" : "1";
       wifi.println("send_data(\"" + sensorData + "\")");
       Serial.println("send_data(\"" + sensorData + "\")");
       
       pinMode(PLDuino::LCD_BACKLIGHT, OUTPUT); digitalWrite(PLDuino::LCD_BACKLIGHT, HIGH); //Turn LCD off
       backLight = 0;
       currentTimer = 0;
      
    }
     
    lblMsg.updateTextAndColor(
        (alarmStatus? msg: "System OK"),
        (alarmStatus? ILI9341_RED : ILI9341_BLACK), tft);
        
           
    delay(50);
  }
  
}


void loop(){}

