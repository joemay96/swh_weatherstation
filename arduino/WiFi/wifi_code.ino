/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-http-get-post-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  
  Code compatible with ESP8266 Boards Version 3.0.0 or above 
  (see in Tools > Boards > Boards Manager > ESP8266)
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

const char* ssid = "iPhone von Josef";
const char* password = "pw1234567890";

//Your Domain name with URL path or IP address with path
const char* username = "ww";
const char* apiKey = "secPW123456VerySecureWeatherStationPassword";
const char* serverName = "https://weatherstation.sharky.live/api/v1/weather";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// sende request alle 10 Sekunden -> später kann das auf 600.000 gesetzt werden -> schickt Daten ale 10 Minuten
unsigned long timerDelay = 10000;

void setup() {
  Serial.begin(115200);
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

void loop() {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
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
