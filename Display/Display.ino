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

#define TFT_CS    D3
#define TFT_RST   D4  
#define TFT_DC    D6

#define TFT_SCLK D5   
#define TFT_MOSI D7   

//TEMP
// !! ausprobieren
#define T_H_PIN 9//D0 // -> vielleicht kann ich den auf D0 schalten
#define DHT_TYPE DHT22
DHT_Unified dht(T_H_PIN, DHT_TYPE);

//Lightsensor
#define LightPin A0
int lightSensorValue = 0;

//Rotary
#define ROTARY_SW D1
// !! ausprobieren
#define ROTARY_DT 15//1 // TX
#define ROTARY_CLK D2
int rotaryCounter = 0;
int rotaryCurrentState;
int rotaryLastState;
String rotaryCurrentDir ="";
unsigned long lastButtonPress = 0;

// Geschwindigkeitssensor
// !! ausprobieren
#define WINDSPEED 10//3 // RX
int speedCum = 0;
int speedTimespan = 0;
double speedQuot = 0;
int speedUpdate = 0;

// !! Bring NodeMCU into sleep mode
boolean sleepState = false;

// Display output
int startState = 1;

int rotaryStateChange = 0;
String rotaryMessage = "Welcome to Weather Woman";

int tempChange = 0;
String tempMessage = "";
int tempCounter = 0;

int lightsensorChange = 0;
int lightCounter = 1000;
// int lightSensorValue = 0;
int lastLightsensorValue = 0;


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
  // Switching other pins functioinality to work as standard GPIO Pins
  pinMode(9, FUNCTION_3); 
  pinMode(10, FUNCTION_3);
  pinMode(15, FUNCTION_3);
  //**************************************************
  Serial.begin(115200);

  /* Display */
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_WHITE);
  // tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(0);
  tft.setRotation(1);
  // tft.setTextColor(ST7735_WHITE);
  // tft.setTextSize(0);
  // tft.setCursor(30,55);
  // tft.println("Hello World!");  
  // delay(1000);

  // Temp und Humidiy
  dht.begin();
  
  // Rotray Sensor
  pinMode(ROTARY_CLK, INPUT);
  pinMode(ROTARY_DT, INPUT);
  pinMode(ROTARY_SW, INPUT_PULLUP);

  // Read the initial state of CLK
  rotaryLastState = digitalRead(ROTARY_CLK);
  // !! mit dem rotary verbinden
  attachInterrupt(digitalPinToInterrupt(5), switch_interrupt, CHANGE);
}

void loop() {
  // Display - not always reprinting but only changing state after values change
  if(startState == 1 || rotaryStateChange == 1 || tempChange == 1 || lightsensorChange == 1 || speedUpdate == 1) {
    resetState();
    printToDisplay();
  }
  // Temp and Humidity
  getHumidityAndTemp();
  // Lightsensor - works
  lightSensorRead();
  // Rotary Sensor
  rotary();
  // Wegen Temp Sensor?!
  // delay(1000);
  windspeed();
}

void printToDisplay() {
  // delay(1000);
  tft.fillScreen(ST7735_WHITE);
  tft.setCursor(5,10);
  tft.println(rotaryMessage);
  tft.setCursor(5,40);
  tft.println(tempMessage);
  tft.setCursor(5, 60);
  tft.println("Windgeschwindigkeit: ");
  tft.setCursor(130, 60);
  tft.println(speedQuot);
  tft.setCursor(5,80);
  tft.println("Lightsensor:");
  tft.setCursor(80, 80);
  tft.print(lightSensorValue);      
  // delay(100);
}

void resetState() {
  startState = 0;
  rotaryStateChange = 0;
  tempChange = 0;
  tempCounter = 0;
  lightsensorChange = 0;
  speedUpdate = 0;
}

void getHumidityAndTemp() {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  float h = event.relative_humidity;
  dht.temperature().getEvent(&event);
  float t = event.temperature;
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    tempChange = 1;
    tempMessage = "Failed to read from DHT sensor!";
    return;
  }
  tempCounter++;
  if(tempCounter == 50) {
    tempCounter = 0;
    Serial.print("Humidity: "); 
    Serial.print(String(h));
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(t);
    Serial.println(" *C ");
    tempChange = 1;
    tempMessage = "Humidity: "+String(h) + " | Temperature: " + t + " °C";
  }
  // davor delay(2000); - brauche ich das?!
  delay(500);
}

void rotary() {
  rotaryCurrentState = digitalRead(ROTARY_CLK);
  if (rotaryCurrentState != rotaryLastState  && rotaryCurrentState == 1){
    rotaryStateChange = 1;    
    if (digitalRead(ROTARY_DT) != rotaryCurrentState) {
      rotaryCounter--;
      rotaryCurrentDir = "CCW";
    } else {
        rotaryCounter++;
        rotaryCurrentDir = "CW";
    }
    // Ausgabe des Rotary Sensors
    rotaryMessage = "Direction: " + rotaryCurrentDir + " | Counter: " + rotaryCounter;
    Serial.println(rotaryMessage);
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
  lightCounter++;
  // !! vielleicht einen allgemeinen Counter auch für Windspeed machen oder irgendwie mit milis() arbeiten.
  if(lightCounter >= 10000) {
    lightCounter = 0;
    lightSensorValue = analogRead(LightPin);
    // Serial.println(lightSensorValue);
    if(lightSensorValue > lastLightsensorValue+10 || lightSensorValue < lastLightsensorValue-10) {
      lastLightsensorValue = lightSensorValue;
      lightsensorChange = 1;
    }
  }
}

// Windgeschiwindigkeitssensor
void windspeed() {
  int speed = digitalRead(WINDSPEED);
  speedTimespan++;
  speedCum = speedCum + speed;
  
  if(speedTimespan >= 10000) {
    speedQuot = (double)(speedCum*10)/(double)speedTimespan;
    Serial.print("SpeedCum: ");
    Serial.println(speedCum);
    Serial.print("SpeedQuot: ");
    Serial.println(speedQuot);
    speedUpdate = 1;
    speedTimespan = 0;
    speedCum = 0;
  }
}

// Function to trigger after the Rotray Interrupt with switch is triggered
// ISRs need to have ICACHE_RAM_ATTR before the function definition to run the interrupt code in RAM.
// https://randomnerdtutorials.com/interrupts-timers-esp8266-arduino-ide-nodemcu/

ICACHE_RAM_ATTR void switch_interrupt() {
  // Code for the standard button state
  int btnState = digitalRead(ROTARY_SW);

  if (btnState == LOW) {
      if (millis() - lastButtonPress > 50) {
          //TODO: Start and stop the var to boot up or shut down
          // !! wake NodeMCU or go into deep sleep
          Serial.println("Button pressed!");
      }
      lastButtonPress = millis();
  }
}
