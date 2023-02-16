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

/*
Pressure at D0 and D1
SCL - D0
SCA - D1
*/

#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
//Pressure
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
/*#include <SPI.h> */
// #define BME_SCK 14
// #define BME_MISO 12
// #define BME_MOSI 13
// #define BME_CS 15
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
unsigned long delayTime;


#define TFT_CS    D3//10
#define TFT_RST   D4//8  
#define TFT_DC    D6//9 //Weiß ich nicht genau D2?

#define TFT_SCLK D5//13   
#define TFT_MOSI D7//11   

//TEMP
#define T_H_PIN D2
#define DHT_TYPE DHT22
DHT_Unified dht(T_H_PIN, DHT_TYPE);

//Lightsensor
#define LightPin A0
int lightSensorValue = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup(void) {
  Serial.begin(115200);

  /* Display */
  // tft.setRotation(tft.getRotation()+1);
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  // tft.setRotation(tft.getRotation()+1);
  tft.setRotation(1);
  // rotateText();

  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(0);
  tft.setCursor(30,55);
  tft.println("Hello World!");  
  delay(1000);

  // Temp und Humidiy
  dht.begin();
  
  // Pressure and Humidity
  /* Wegen Pressure Sensor herausgeschmissen  */
  // bool status = bme.begin(0x76);  
  // if (!status) {
  //   Serial.println("Could not find a valid BME280 sensor, check wiring!");
  //   while (1);
  // }
  // delayTime = 1000;

}

void loop() {
  // Display
  printToDisplay();
  // Temp and Humidity
  // getHumidityAndTemp();
  // Pressure Sensor
  //pressureSensorValues();
  // Lightsensor
  lightSensorRead();
}

void printToDisplay() {
  tft.fillScreen(ST7735_WHITE); 
  delay(1000);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(0);
  tft.setCursor(30,80);
  tft.println("Hey you! You got it!");  
  delay(500);
}

void getHumidityAndTemp() {
  // float h = dht.Humidity();
  // float t = dht.Temperature();
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  float h = event.relative_humidity;
  dht.temperature().getEvent(&event);
  float t = event.temperature;
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Humidity: "); 
  Serial.print(String(h));
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.println(" *C ");
  delay(2000);
}

void pressureSensorValues() {
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");
  
  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}

// Problem: bekomme ständig 1024, sobald das Licht an ist - Widerstand bringt nichts...
// Bringt es was die dinger in Reihe zu schalten? -> Ich glaube nicht Sinnvoll, kann aber nicht mehr als einen Sensor messen glaube ich.
// Es sei denn ich schalte sie alle parallel und der Strom sucht sich den Pfad mit dem geringsten Widerstand und daher zählt nur der Wert mit
// dem hellsten Wert?! :D 
void lightSensorRead() {
  lightSensorValue = analogRead(LightPin);
  Serial.print("LightSensor = ");
  Serial.println(lightSensorValue);
  delay(1000);
}
