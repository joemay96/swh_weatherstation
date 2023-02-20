/*
Verbindungen:
Display:
Braun [SCK]3 -> D7 (GPIO13) - 13
Weiß [SDA]4 -> D6 (GPIO12) - 11
Lila [RES]5 -> D2 (GPIO4) - 8
Weiß [RS]6 -> D1 (GPIO5) - 9
Blau [CS]7 -> D5 (GPIO14) - 10
Grün [LEDA]8 -> 3V
*/

#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define TFT_CS   D3
#define TFT_RST  D4  
#define TFT_DC   D6

#define TFT_SCLK D5   
#define TFT_MOSI D7   

//TEMP
#define T_H_PIN 10//!!1
#define DHT_TYPE DHT22
DHT_Unified dht(T_H_PIN, DHT_TYPE);

//Lightsensor
#define LightPin A0
int lightSensorValue = 0;
String lightIntensity = "Light";

//Rotary
// #define ROTARY_SW // Benötigt keinen Pin, da direkt an RST angeschlossen
#define ROTARY_DT D2
#define ROTARY_CLK D1//!!3
int pageCounter = 0;
int rotaryCurrentState;
int rotaryLastState;
String rotaryCurrentDir ="";
unsigned long lastButtonPress = 0;

// Geschwindigkeitssensor
#define WINDSPEED 9//!!D1
int speedCum = 0;
double speedQuot = 0;

// WiFi config
const char* ssid = "iPhone von Josef";
const char* password = "pw1234567890";
const char* username = "ww";
const char* apiKey = "secPW123456VerySecureWeatherStationPassword";
const char* serverName = "https://weatherstation.sharky.live/api/v1/weather";
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long requestTime = 600000;
// sende request alle 10 Sekunden -> später kann das auf 600.000 gesetzt werden -> schickt Daten ale 10 Minuten
unsigned long requestTime = 10000;

// Bring NodeMCU into sleep mode
// https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/
// int deepSleep = 0;

// Display output
int startState = 1;

int rotaryStateChange = 0;
String rotaryMessage = "";

int tempChange = 0;
float temp = 0;
float humidity = 0;
int tempCounter = 0;

int lightsensorChange = 0;
int lastLightsensorValue = 0;

// Global counter for updating values and reprinting the screen
int COUNTER = 3000;
int buttonPressed = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup(void) {
  Serial.begin(115200);
  setupWeatherStation();
  setupWifi();
}

void loop() {
  COUNTER++;
  if(rotaryStateChange == 1) {
    rotaryStateChange = 0;
    if(pageCounter%3 == 0) {
      page1();
    } else if(pageCounter%3 == -1 || pageCounter == 2) {
      page2();      
    } else if (pageCounter%3 == -2 || pageCounter == 1) {
      page3();
    }
  }
  if(COUNTER > 5000) {
    resetState();
    updateValues();
  }
  // Rotary Sensor
  rotary();
  // Temp and Humidity
  getHumidityAndTemp();
  // Lightsensor - works
  lightSensorRead();
  // Windspeed sensor
  windspeed();
  // send request to api
  sendHTTPRequest();
  
  /* Checking for deepSleep
   DeepSleep ist schwierig, da nach gewissen Anforderungen der Arduino wieder aufgeweckt werden muss, um 
   1. den Wind zu messen
   2. regelämßig die Daten zu checken und an die API zu senden
   3. an und aus zu gehen, wenn der button gedrückt wird -> durch button drücken wird allerdings nur ein reset durchgeführt! 
   -> man bräuchte einen zusätzlichen GPIO über welchen man das ganze besser steuern kann
  */
  // if(deepSleep%2==1)
  // {
  //   ESP.deepSleep(0);
  // }
}

// All setup needed for Station 
// Gets called in setup method
void setupWeatherStation() {
  //********** CHANGE PIN FUNCTION  TO GPIO **********
  // https://arduino.stackexchange.com/questions/29938/how-to-i-make-the-tx-and-rx-pins-on-an-esp-8266-01-into-gpio-pins
  //GPIO 1 (TX) swap the pin to a GPIO.
  pinMode(1, FUNCTION_0);  //!!
  // To swap back pinMode(1, FUNCTION_0);
  //GPIO 3 (RX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_0);  //!!
  // to swap back pinMode(3, FUNCTION_0);
  //**************************************************

  /* Display */
  setupDisplay();
  rotaryStateChange = 1;
  /* Temp und Humidiy*/
  dht.begin();
  
  /* Rotray Encoder */
  pinMode(ROTARY_CLK, INPUT);
  pinMode(ROTARY_DT, INPUT);
  rotaryLastState = digitalRead(ROTARY_CLK);
}

// !! changing Serial to a page -- something special maybe
void setupWifi() {
  // Connecting to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void setupDisplay() {
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  // tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  // tft.setTextSize(0);
  tft.setRotation(1);
  // Print inital start
  tft.setCursor(5, 10);
  tft.setTextSize(2);
  tft.setCursor(40, 35);
  tft.println("Weather");
  tft.setCursor(40, 70);
  tft.println("Woman");
  tft.setTextSize(0);
}

void resetState() {
  COUNTER = 0;
  speedCum = 0;
}

void getHumidityAndTemp() {
  if((COUNTER % 2000) == 0) {
    sensors_event_t event;
    dht.humidity().getEvent(&event);
    humidity = event.relative_humidity;
    dht.temperature().getEvent(&event);
    temp = event.temperature;
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp)) {
      humidity = 0;
      temp = 0;
      return;
    }
  }
}

void rotary() {
  rotaryCurrentState = digitalRead(ROTARY_CLK);
  if (rotaryCurrentState != rotaryLastState  && rotaryCurrentState == 1){
    rotaryStateChange = 1;
    if (digitalRead(ROTARY_DT) != rotaryCurrentState) {
      pageCounter--;
      rotaryCurrentDir = "CCW";
    } else {
      pageCounter++;
      rotaryCurrentDir = "CW";
    }
    // Ausgabe des Rotary Sensors
    rotaryMessage = pageCounter;
  }
  // Remember last CLK state
	rotaryLastState = rotaryCurrentState;
  delay(1);
}

// Problem: bekomme ständig 1024, sobald das Licht an ist - Widerstand bringt nichts...
// Bringt es was die dinger in Reihe zu schalten? -> Ich glaube nicht Sinnvoll, kann aber nicht mehr als einen Sensor messen glaube ich.
// Es sei denn ich schalte sie alle in Reihe und der die Wiederstände werden größer wenn einzelne Sensoren verdeckt werden.
// ==> Reihenschaltung!
void lightSensorRead() {
  if((COUNTER % 1000) == 0) {
    lightSensorValue = analogRead(LightPin) -25;
    if(lightSensorValue < 0) {
      lightSensorValue = 0;
    }
    if(lightSensorValue > lastLightsensorValue+10 || lightSensorValue < lastLightsensorValue-10) {
      lastLightsensorValue = lightSensorValue;
    }
    lightintensity();
  }
}

void lightintensity() {
    if (lightSensorValue < 150) {
    lightIntensity = "Dark";
  } else if (lightSensorValue < 450) {
    lightIntensity = "Dim";
  } else if (lightSensorValue < 650) {
    lightIntensity = "Light";
  } else if (lightSensorValue < 900) {
    lightIntensity = "Bright";
  } else {
    lightIntensity = "Very bright";
  }
}

// Windgeschiwindigkeitssensor
void windspeed() {
  int speed = digitalRead(WINDSPEED);
  speedCum = speedCum + speed;
  speedQuot = (double)(speedCum)/(double)1000.0; //!! vielleicht noch anpassen, wenn kostruktion steht
}

/*
 * WiFi
 */
void sendHTTPRequest() {
  //Send an HTTP POST request every "requestTime" minutes
  if ((millis() - lastTime) > requestTime) {
    //Check WiFi connection status
    if(WiFi.status() == WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
      
      http.begin(client, serverName);
      http.setAuthorization("ww", apiKey);
      // If you need an HTTP request with a content type: application/json, use the following:
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST("{\"temperature\":\"23.4\",\"humidity\":\"37.2\",\"lightintensity\":\"320.4\",\"windspeed\":\"1.23\",\"windspeed_cum\":\"1234\"}");
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

/*
 * Pages
 */

void pageHeader() {
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);  // set text color to white and black background
  tft.setTextSize(1);
  tft.setCursor(39, 0);
  tft.print("Wetter Station");
  tft.setCursor(42, 15);
  tft.print("Weather Woman");
  tft.drawFastHLine(0, 30,  tft.width(), ST7735_WHITE);
}

void page1() {
  tft.fillScreen(ST7735_BLACK);
  pageHeader();
  // Temperature and Humidity
  tft.drawFastHLine(0, 76,  tft.width(), ST7735_WHITE);
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  tft.setCursor(40, 39);
  tft.print("Temperatur: ");
  tft.setCursor(34, 85);
  tft.print("Luftfeuchtigkeit: ");
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.setCursor(60, 54);
  tft.print(temp);
  tft.drawCircle(87, 56, 2, ST7735_YELLOW);  // print degree symbol ( ° )
  tft.setCursor(90, 54);
  tft.print("C");
  tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  tft.setCursor(64, 100);
  tft.print(humidity);
  tft.setCursor(90, 100);
  tft.print("%");
}

void page2() {  
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);  // set text color to white and black background
  tft.setTextSize(1);
  pageHeader();
  // Lichtstärke und Windgeschwindigkeit
  tft.drawFastHLine(0, 76,  tft.width(), ST7735_WHITE);
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  tft.setCursor(30, 39);
  tft.print("Windgeschwindigkeit: ");
  tft.setCursor(39, 85);
  tft.print("Lichtstaerke: ");
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.setCursor(64, 54);
  tft.print(speedQuot);
  tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  tft.setCursor(60, 100);
  tft.print(lightIntensity);
}

void page3() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(0);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  pageHeader();
  tft.setCursor(5,45);
  tft.println("Temperature: ");
  tft.setCursor(130, 45);
  tft.println(temp);
  tft.setCursor(5,65);
  tft.println("Humidity: ");
  tft.setCursor(130, 65);
  tft.println(humidity);
  tft.setCursor(5, 85);
  tft.println("Windgeschwindigkeit: ");
  tft.setCursor(130, 85);
  // vielleicht noch speedCum hier einbauen
  tft.println(speedQuot);
  tft.setCursor(5,105);
  tft.println("Lightsensor:");
  tft.setCursor(80, 105);
  tft.print(lightSensorValue);      
}

void updateValues() {
  if(pageCounter%3 == 0) {
      // update values page 1      
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      tft.setCursor(60, 54);
      tft.print(temp);
      tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
      tft.setCursor(64, 100);
      tft.print(humidity);
    } else if(pageCounter%3 == -1 || pageCounter == 2) {
      // update values page 2
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      tft.setCursor(64, 54);
      tft.print(speedQuot);
      tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
      tft.setCursor(60, 100);
      tft.print(lightIntensity);
    } else if (pageCounter%3 == -2 || pageCounter == 1) {
      // update values page 3
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setCursor(130, 45);
      tft.println(temp);
      tft.setCursor(130, 65);
      tft.println(humidity);
      tft.setCursor(130, 85);
      // vielleicht noch speedCum hier einbauen
      tft.println(speedQuot);
      tft.setCursor(80, 105);
      // tft.setTextColor(ST7735_BLACK, ST7735_BLACK);
      // tft.print("XXXX");
      // tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print(lightSensorValue);      
    }
}