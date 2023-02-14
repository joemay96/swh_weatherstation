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

#define TFT_CS    D3//10
#define TFT_RST   D4//8  
#define TFT_DC    D6//9 //Weiß ich nicht genau D2?

#define TFT_SCLK D5//13   
#define TFT_MOSI D7//11   

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup(void) {
  Serial.begin(9600);

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
  
}

void loop() {

  tft.fillScreen(ST7735_WHITE); 
  delay(1000);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(0);
  tft.setCursor(30,80);
  tft.println("Hey you! You got it!");  

  delay(500);
}
