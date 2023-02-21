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
#define T_H_PIN 1// debugging 10
#define DHT_TYPE DHT22
DHT_Unified dht(T_H_PIN, DHT_TYPE);

//Lightsensor
#define LightPin A0
int lightSensorValue = 0;
String lightIntensity = "Light";

//Rotary
// #define ROTARY_SW // Benötigt keinen Pin, da direkt an RST angeschlossen
#define ROTARY_DT D2
#define ROTARY_CLK 3 // debugging D1
int pageCounter = 0;
int rotaryCurrentState;
int rotaryLastState;

// Geschwindigkeitssensor
#define WINDSPEED D1 // debugging 9
int speedCum = 0;
double speedQuot = 0;
int apiSpeedCum = 0;
double apiSpeedQuot = 0;

/* WiFi config */
const char* ssid = "public-wohnhaus";//"iPhone von Josef";
const char* password = "";//"pw1234567890";
const char* username = "ww";
const char* apiKey = "secPW123456VerySecureWeatherStationPassword";
const char* serverName = "https://weatherstation.sharky.live/api/v1/weather";
unsigned int wifiTimeCheck = 0;
// the following variables are unsigned longs because the time, measured in milliseconds, will quickly become a bigger number than can be stored in an int.
// after 1 minute the first request is send. Than every 10 minutes
unsigned long lastTime = 0;
// Timer set to 10 minutes (=600000)
unsigned long requestTime = 60000;
IPAddress IP;
String connectionStatus = "";
int firstResponse = 0;
int secondResponse = 0;

// Bring NodeMCU into sleep mode
// https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/
// int deepSleep = 0;

int rotaryStateChange = 0;

/* Global sensor values */
float temp = 0;
float humidity = 0;

int lastLightsensorValue = 0;

// Global counter for updating values and reprinting the screen
// Counter Mechanik und kein delay wegen des Geschwindigkeitssensors
// Für normale Sensoren reicht ein delay von 1000-2000, allerdings kann in dieser Zeit der Geschwindigkeitssensor nicht messen und wird somit nicht nutzbar.
// Durch den Counter behält er seine Funktionalität und die anderen Snesoren erhalten weiterhin ihr delay von ca. 1-2 sekunden
// Eine andere, vielleicht bessere, Methode ist die Verwendung von millis(), allerdings müsste man dafür mehr Variablen verwenden, was sich aber auch im Rahmen halten sollte.
int COUNTER = 3000;

// Display setup with lib
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup(void) {
  setupWeatherStation();
  setupWifi(30000);
}

void loop() {
  COUNTER++;
  if(rotaryStateChange == 1) {
    rotaryStateChange = 0;
    if(pageCounter%4 == 0) {
      page1();
    } else if(pageCounter%4 == -1 || pageCounter == 3) {
      page2();      
    } else if (pageCounter%4 == -2 || pageCounter == 2) {
      page3();
    } else if (pageCounter%4 == -3 || pageCounter == 1) {
      page4();
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

/*
 * All needed setup code for  the station 
 */
void setupWeatherStation() {
  //********** CHANGE PIN FUNCTION  TO GPIO **********
  // https://arduino.stackexchange.com/questions/29938/how-to-i-make-the-tx-and-rx-pins-on-an-esp-8266-01-into-gpio-pins
  //GPIO 1 (TX) swap the pin to a GPIO.
  pinMode(1, FUNCTION_3);  // debugging pinMode 0
  // To swap back pinMode(1, FUNCTION_0);
  //GPIO 3 (RX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_3);  // debugging pinMode 0
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

/*
 * Setup the WiFi connection
 */
void setupWifi(unsigned int connectionWaitingTime) {
  // Connecting to WiFi
  WiFi.begin(ssid, password);
  connectionStatus = "Connecting";
  wifiTimeCheck = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - wifiTimeCheck < connectionWaitingTime) {    
    delay(500);
    connectionStatus += ".";
  }
  connectionStatus = "Connected";
  IP = WiFi.localIP();
}

/* 
 * The display setup page for booting, inital WiFi connection etc.
 */
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
  tft.setCursor(40, 25);
  tft.println("Weather");
  tft.setCursor(40, 60);
  tft.println("Woman");
  tft.setTextSize(0);
  tft.setCursor(40, 100);
  tft.setTextSize(0);
  tft.println("Connecting ..."); 
}

/*
 * reset variables after updating all values
 */
void resetState() {
  COUNTER = 0;
  speedCum = 0;
}

/*
 * BMP280 Sensor for temperature and humidity
 */
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

/*
 * Rotary encoder
 */
void rotary() {
  rotaryCurrentState = digitalRead(ROTARY_CLK);
  if (rotaryCurrentState != rotaryLastState  && rotaryCurrentState == 1){
    rotaryStateChange = 1;
    if (digitalRead(ROTARY_DT) != rotaryCurrentState) {
      pageCounter--;
    } else {
      pageCounter++;
    }
  }
  // Remember last CLK state
	rotaryLastState = rotaryCurrentState;
  delay(1);
}

/* 
 * Problem: bekomme ständig 1024, sobald das Licht an ist - Widerstand bringt nichts...
 * Bringt es was die dinger in Reihe zu schalten? -> Ich glaube nicht Sinnvoll, kann aber nicht mehr als einen Sensor messen glaube ich.
 * Es sei denn ich schalte sie alle in Reihe und der die Wiederstände werden größer wenn einzelne Sensoren verdeckt werden.
 * ==> Reihenschaltung!
 */
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
  apiSpeedCum += speedCum;
  apiSpeedQuot += speedQuot;
}

/*
 * WiFi
 */
void sendHTTPRequest() {
  // Send an HTTP POST request every "requestTime" minutes
  if ((millis() - lastTime) > requestTime) {
    //Check WiFi connection status
    if(WiFi.status() == WL_CONNECTED){
      WiFiClientSecure client;
      HTTPClient http;
      const int port = 443; // using https port - otherwise 301 response
      // => No fingerprint - pretty insecure
      client.setInsecure();
      client.connect(serverName, port);
      connectionStatus = "Connected";
      http.begin(client, serverName);

      http.addHeader("Content-Type", "application/json");
      http.addHeader("ww", apiKey);

      // Sending mean of wind sensor values, otherwise probably most of the time 0 -> its not windy specifically on the time the request is send - all other values get lost.
      int cumMean = apiSpeedCum / (requestTime/1000);
      double quotMean = apiSpeedQuot / (double)requestTime*(double)1000.0;
      apiSpeedCum = 0;
      apiSpeedQuot = 0;
      
      // creating the http post request
      int httpResponseCode = http.POST("{\"temperature\":\""+String(temp)+"\",\"humidity\":\""+String(humidity)+"\",\"lightintensity\":\""+String(lightSensorValue)+"\",\"windspeed\":\""+String(quotMean)+"\",\"windspeed_cum\":\""+String(cumMean)+"\"}");
     
      // saveing a few status codes
      secondResponse = firstResponse;
      firstResponse = httpResponseCode;

      // Free resources
      http.end();
    } else {
      connectionStatus = "Disconnected";
      setupWifi(3500);
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
  tft.drawCircle(94, 56, 2, ST7735_YELLOW);  // print degree symbol ( ° )
  tft.setCursor(99, 54);
  tft.print("C");
  tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  tft.setCursor(64, 100);
  tft.print(humidity);
  tft.setCursor(97, 100);
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
  tft.println(speedQuot);
  tft.setCursor(5,105);
  tft.println("Lightsensor:");
  tft.setCursor(80, 105);
  tft.print(lightSensorValue);      
}

/* 
 * WiFi debudding Page 
 * Data:
 * - IP-Adress
 * - Connection Status
 * - last HTTP Request
 */
void page4() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(0);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 5);
  tft.print("Wifi debugging Page");
  tft.drawFastHLine(0, 15,  tft.width(), ST7735_WHITE);

  tft.setCursor(5,30);
  tft.println("IP: ");
  tft.setCursor(25, 30);
  tft.println(IP);
  tft.setCursor(5,50);
  tft.println("WiFi Status: ");
  tft.setCursor(90, 50);
  tft.println(connectionStatus);
  tft.setCursor(5, 75);
  tft.println("1. HTTP code: ");
  tft.setCursor(85, 75);
  tft.println(firstResponse);
  tft.setCursor(5,95);
  tft.println("2. HTTP code: ");
  tft.setCursor(85, 95);
  tft.print(secondResponse);      
}

void updateValues() {
  if(pageCounter%4 == 0) {
      // update values page 1      
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      tft.setCursor(60, 54);
      tft.print(temp);
      tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
      tft.setCursor(64, 100);
      tft.print(humidity);
    } else if(pageCounter%4 == -1 || pageCounter == 3) {
      // update values page 2
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      tft.setCursor(64, 54);
      tft.print(speedQuot);
      tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
      tft.setCursor(60, 100);
      tft.print(lightIntensity);
    } else if (pageCounter%4 == -2 || pageCounter == 2) {
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
    } else if (pageCounter%4 == -3 || pageCounter == 1) {
      // update values page 4
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setCursor(25, 30);
      tft.println(IP);
      tft.setCursor(90, 50);
      tft.println(connectionStatus);
      tft.setCursor(85, 75);
      tft.println(firstResponse);
      tft.setCursor(85, 95);
      tft.print(secondResponse);      
    }
}
