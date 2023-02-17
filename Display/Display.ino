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
//! Brauche ich nicht mehr - Pressure
// #include <Wire.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>

//! Herausgelöscht
/*#include <SPI.h> */
// #define BME_SCK 14
// #define BME_MISO 12
// #define BME_MOSI 13
// #define BME_CS 15
// #define SEALEVELPRESSURE_HPA (1013.25)
// Adafruit_BME280 bme; // I2C
// unsigned long delayTime;


#define TFT_CS    D3//10
#define TFT_RST   D4//8  
#define TFT_DC    D6//9 //Weiß ich nicht genau D2?

#define TFT_SCLK D5//13   
#define TFT_MOSI D7//11   

//TEMP
#define T_H_PIN D2 // -> vielleicht kann ich den auf D0 schalten
#define DHT_TYPE DHT22
DHT_Unified dht(T_H_PIN, DHT_TYPE);

//Lightsensor
#define LightPin A0
int lightSensorValue = 0;

//Rotary
// D8, RX, TX
#define ROTARY_SW 5//D1
#define ROTARY_DT 3
#define ROTARY_CLK D5
int rotaryCounter = 0;
int rotaryCurrentState;
int rotaryLastState;
String rotaryCurrentDir ="";
unsigned long debounceDelay = 0;
unsigned long lastButtonPress = 0;


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup(void) {
  //********** CHANGE PIN FUNCTION  TO GPIO **********
  // https://arduino.stackexchange.com/questions/29938/how-to-i-make-the-tx-and-rx-pins-on-an-esp-8266-01-into-gpio-pins
  //GPIO 1 (TX) swap the pin to a GPIO.
  pinMode(1, FUNCTION_3); 
  // To swap back pinMode(1, FUNCTION_0); 
  //GPIO 3 (RX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_3); 
  // to swap back pinMode(3, FUNCTION_0); 
  //**************************************************
  Serial.begin(9600);

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
  // dht.begin();
  
  // Pressure and Humidity
  /* Wegen Pressure Sensor herausgeschmissen  */
  // bool status = bme.begin(0x76);  
  // if (!status) {
  //   Serial.println("Could not find a valid BME280 sensor, check wiring!");
  //   while (1);
  // }
  // delayTime = 1000;

  // Rotray Sensor
  pinMode(ROTARY_CLK, INPUT);
  pinMode(ROTARY_DT, INPUT);
  pinMode(ROTARY_SW, INPUT_PULLUP);

  // Read the initial state of CLK
  rotaryLastState = digitalRead(ROTARY_CLK);
  Serial.println(rotaryLastState);
  // ROTARTY_SW = 5
  attachInterrupt(digitalPinToInterrupt(5), switch_interrupt, CHANGE);
}

void loop() {
  // Display
  printToDisplay();
  // Temp and Humidity
  // getHumidityAndTemp();
  // Pressure Sensor
  //pressureSensorValues();
  // Lightsensor
  // lightSensorRead();

  //! Rotray -> Brauche ich eigentlich auch nicht mehr - Ich muss hier den Rotary dreh Ding erkennen 
  // int btnState = digitalRead(ROTARY_SW);
  // if (btnState == LOW) {
  //     if (millis() - lastButtonPress > 50) {
  //         Serial.println("Button pressed!");
  //     }
  //     lastButtonPress = millis();
  // }
  /*rotaryCurrentState = digitalRead(ROTARY_CLK);
  if (rotaryCurrentState != rotaryLastState  && rotaryCurrentState == 1){
    if (digitalRead(ROTARY_DT) != rotaryCurrentState) {
      rotaryCounter --;
      rotaryCurrentDir ="CCW";
    } else {
        rotaryCounter ++;
        rotaryCurrentDir ="CW";
    }
    // Ausgabe des Rotary Sensors
    Serial.print("Direction: ");
		Serial.print(rotaryCurrentDir);
		Serial.print(" | Counter: ");
		Serial.println(rotaryCounter);
  }
  // Remember last CLK state
	rotaryLastState = rotaryCurrentState;
  delay(500);*/
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

//! Kann raus, weil ich den Sensor nicht mehr verwende
/*void pressureSensorValues() {
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
}*/

// Problem: bekomme ständig 1024, sobald das Licht an ist - Widerstand bringt nichts...
// Bringt es was die dinger in Reihe zu schalten? -> Ich glaube nicht Sinnvoll, kann aber nicht mehr als einen Sensor messen glaube ich.
// Es sei denn ich schalte sie alle parallel und der Strom sucht sich den Pfad mit dem geringsten Widerstand und daher zählt nur der Wert mit
// dem hellsten Wert?! :D 
// S.90-92 Reihenshcaltung
// S.93-97 Parallelschaltung
// ==> Reihenschaltung!
void lightSensorRead() {
  lightSensorValue = analogRead(LightPin);
  Serial.print("LightSensor = ");
  Serial.println(lightSensorValue);
  delay(1000);
}

// functions for rotary sensor
void button_press()
{
  int buttonVal = digitalRead(ROTARY_SW);
  //If we detect LOW signal, button is pressed
  if (buttonVal == LOW) {
    if (millis() - debounceDelay > 200) {
      Serial.println("Button pressed!");
    }
    debounceDelay = millis();
  }
}

// Das kann raus, wenn der Rotary Sensor funktioniert
/*void encoder_value() {
  // Read the current state of CLK
  rotaryCurrentState = digitalRead(ROTARY_CLK);
  // If last and current state of CLK are different, then we can be sure that the pulse occurred
  if (rotaryCurrentState != rotaryLastState  && rotaryCurrentState == 1) {
    // Encoder is rotating counterclockwise so we decrement the counter
    if (digitalRead(ROTARY_DT) != rotaryCurrentState) {
      rotaryCounter ++;
    } else {
      // Encoder is rotating clockwise so we increment the counter
      rotaryCounter --;
    }
    // print the value in the serial monitor window
    Serial.print("Counter: ");
    Serial.println(rotaryCounter);
  }
  // Remember last CLK state for next cycle
  rotaryLastState = rotaryCurrentState;
}*/

// Function to trigger after the Rotray Interrupt with switch is triggered
// ISRs need to have ICACHE_RAM_ATTR before the function definition to run the interrupt code in RAM.
// https://randomnerdtutorials.com/interrupts-timers-esp8266-arduino-ide-nodemcu/

ICACHE_RAM_ATTR void switch_interrupt() {
  // Code for the standard button state
  int btnState = digitalRead(ROTARY_SW);

  if (btnState == LOW) {
      if (millis() - lastButtonPress > 50) {
          Serial.println("Button pressed!");
      }
      lastButtonPress = millis();
  }
}
