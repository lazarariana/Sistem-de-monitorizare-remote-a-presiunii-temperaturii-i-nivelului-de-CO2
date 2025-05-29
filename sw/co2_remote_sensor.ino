#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <settings.h>
#include <sunrise.h>
#include <utils.h>

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 3 /* rotation 180* */); /* Landscape with communication connector on the right side */
long lastDataUpdate = millis();
long lastScreenUpdate = millis();
const int UPDATE_INTERVAL_SECS = 15;        // Update every 15 seconds
const int SCREEN_UPDATE_INTERVAL_SECS = 1;  // Update every 15 seconds
AsyncWebServer server(80);
uint8_t displayNr = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32-C3!");

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(FAN_CTRL, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  reInitI2C(); /* Initialize I2C and use default pins defined for the board */

  // Initialize Little Flash Filesystem
  initLittleFS();

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

  gfx->setCursor(10, 10);
  gfx->setTextColor(RGB565_GREEN);
  gfx->setTextSize(2);
  gfx->println("Waiting......");

  /* Read the sensor's configs */
  Serial.println("Sensor Measurement Configurations:");
  read_sensor_config(SUNRISE_ADDR);
  Serial.println();

#if (DISABLE_ABC == 1)
  setABC(SUNRISE_ADDR, false);
#endif

  /* Change measurement mode if single */
  change_measurement_mode(SUNRISE_ADDR);

  connectWifi(gfx);

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP180 sensor, check wiring!");
    while (1) {}
  }

  screenNeedsUpdate = true;

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(bmpTemperature).c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(bmpPressure / 100.0F).c_str());
  });
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(CO2Val).c_str());
  });

  server.begin();
}

void loop()
{
  static uint8_t lastBtnState = HIGH;
  uint8_t state = digitalRead(BTN_PIN);

  if (state != lastBtnState) {
    lastBtnState = state;
    if (state == LOW) {
      displayNr++;
      if (displayNr >= 4) {
        displayNr = 0;
      }
      screenNeedsUpdate = true;
    }
  }

  // Check if we should update data from sensors
  if (millis() - lastDataUpdate > 1000 * UPDATE_INTERVAL_SECS) {
    lastDataUpdate = millis();
    screenNeedsUpdate = true;

    bmp_update();

    /**
     * @brief  The main function loop. Reads the sensor's current
     *         CO2 value and error status and prints them to the 
     *         Serial Monitor.
     * 
     * @retval None
     */
    CO2Val = read_sensor_measurements(SUNRISE_ADDR);

    /* Delay between readings */
    Serial.print("\nWaiting... ");
    Serial.print(readPeriodMs);
    Serial.println("MS\n");

    Serial.println("**************************************************************");
  }

  // Check if we should update weather information
  if (millis() - lastScreenUpdate > 1000 * SCREEN_UPDATE_INTERVAL_SECS) {
    lastScreenUpdate = millis();

    if (screenNeedsUpdate) {
      screenNeedsUpdate = false;
      lcd_update(gfx, displayNr);
    }
  }

  delay(500);

  /* Handle LED output and the Fan control*/
  if (CO2Val < 700) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(FAN_CTRL, LOW);
  } else if (CO2Val >= 700 && CO2Val < 1000) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(FAN_CTRL, HIGH);
  } else {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(FAN_CTRL, HIGH);
  }
}
