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

static const int numReadings = 5;
float readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int readCount = 0;
float total = 0;                  // the running total
float average = 0;                // the average

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
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
    server.send(200, "text/plain", F("Temperature sender for Gree I-Feel devices."));
  });

  ElegantOTA.begin(&server);
  server.begin();

  heatpumpIR = new GreeYACHeatpumpIR();
  Serial.println(F("Starting IR Sensor"));

  delayStart = 0;
  readCount = 0;
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
  {
    readings[thisReading] = 0;
  }
}

void loop()
{
  MDNS.update();
  server.handleClient();

  if ((millis() - delayStart) >= DELAY_TIME)
  {
    if (sht30.get() == 0)
    {
      delayStart += DELAY_TIME; // this prevents drift in the delays

      Serial.print("Temperature in Celsius : ");
      Serial.println(sht30.cTemp);
      Serial.print("Relative Humidity : ");
      Serial.println(sht30.humidity);

      total = total - readings[readIndex];
      readings[readIndex] = sht30.cTemp;
      total = total + readings[readIndex];
      readIndex = (readIndex + 1) % numReadings;
      int count = min(++readCount, numReadings);
      readCount = count;

      // calculate the average:
      average = total / (float)readCount;

      uint8_t rounded = round(average);
      Serial.print(F("Sending IR for current temperature : "));
      Serial.println(rounded);
      heatpumpIR->send(irSender, rounded);
    }
  }

  delay(10);
}