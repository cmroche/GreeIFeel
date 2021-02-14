#include <Arduino.h>
#include <GreeHeatpumpIR.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ElegantOTA.h>
#include <WEMOS_SHT3X.h>

#include "config.h"

ESP8266WebServer server(80);

SHT3X sht30(0x45);
IRSenderESP8266Alt irSender(D6);
GreeYACHeatpumpIR *heatpumpIR;

unsigned long DELAY_TIME = 60 * 1000; // 1 minute
unsigned long delayStart = 0;         // the time the delay started

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(500);

  Serial.print(F("Setting up WiFi"));
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println(F("Starting MDNS"));
  String clientId = String("TEMPERATURE-") + String(ESP.getChipId(), HEX);
  if (!MDNS.begin(clientId))
  {
    Serial.println(F("Unable to setup MDNS responder"));
  }

  Serial.println(F("Starting HTTP OTA service"));
  server.on("/", []() {
    server.send(200, "text/plain", F("Meow meow meow meow meow meow meow meow meow meow."));
  });

  ElegantOTA.begin(&server);
  server.begin();

  heatpumpIR = new GreeYACHeatpumpIR();
  Serial.println(F("Starting IR Sensor"));

  delayStart = millis();
}

void loop()
{
  MDNS.update();
  server.handleClient();

  if ((millis() - delayStart) >= DELAY_TIME)
  {
    delayStart += DELAY_TIME; // this prevents drift in the delays
    if (sht30.get() == 0)
    {
      Serial.print("Temperature in Celsius : ");
      Serial.println(sht30.cTemp);
      Serial.print("Relative Humidity : ");
      Serial.println(sht30.humidity);
      Serial.println();

      Serial.println(F("Sending IR for current temperature"));
      heatpumpIR->send(irSender, uint8_t(sht30.cTemp - 1.0));
    }
  }

  delay(5000);
}