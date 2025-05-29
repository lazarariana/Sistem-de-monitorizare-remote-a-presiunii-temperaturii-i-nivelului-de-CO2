#include <Adafruit_BMP085.h>
#include <settings.h>

#define BM180_ADDR 0x77

Adafruit_BMP085 bmp;
int32_t sealevelPressure;
int32_t bmpPressure;
float bmpTemperature;
float bmpAltitude;
float bmpRealAltitude;
uint16_t CO2Val;
bool screenNeedsUpdate;

void bmp_update(void)
{
  bmpTemperature = bmp.readTemperature();
  Serial.print("Temperature = ");
  Serial.print(bmpTemperature);
  Serial.println(" C");

  Serial.println("**************************************************************");

  bmpPressure = bmp.readPressure();
  Serial.print("Pressure = ");
  Serial.print(bmpPressure);
  Serial.println(" Pa");

  Serial.println("--------------------------------------------------------------");

  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  bmpAltitude = bmp.readAltitude();
  Serial.print("Altitude = ");
  Serial.print(bmpAltitude);
  Serial.println(" meters");

  Serial.println("--------------------------------------------------------------");

  sealevelPressure = bmp.readSealevelPressure();
  Serial.print("Pressure (sealevel/calculated) = ");
  Serial.print(sealevelPressure);
  Serial.println(" Pa");

  Serial.println("--------------------------------------------------------------");
  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
  bmpRealAltitude = bmp.readAltitude(101500);
  Serial.print("Real altitude = ");
  Serial.print(bmpRealAltitude);
  Serial.println(" meters");

  Serial.println();

  Serial.println("**************************************************************");
}

void lcd_update(Arduino_GFX *gfx, uint8_t screenNr)
{
  /*----------------------------------------------------------------------------------*/
  // clear part of the screen
  gfx->setCursor(0, 0);
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_GREEN);

  /*----------------------------------------------------------------------------------*/
  /*----------------------------------------------------------------------------------*/
  /*----------------------------------------------------------------------------------*/

  gfx->setTextSize(2);
  if (screenNr == 0) {
    gfx->println("**************************");

    gfx->print("Temperature = ");
    gfx->print(bmpTemperature);
    gfx->println(" 'C");

    gfx->println("**************************");
  } else if (screenNr == 1) {
    gfx->println("--------------------------");
    gfx->print("Pressure = ");
    gfx->print(bmpPressure);
    gfx->println(" Pa");

    gfx->println("--------------------------");

    gfx->print("Sealevel = ");
    gfx->print(sealevelPressure);
    gfx->println(" Pa");

    gfx->println("--------------------------");
  } else if (screenNr == 2) {
    gfx->println("--------------------------");
    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    gfx->print("Altitude = ");
    gfx->print(bmpAltitude);
    gfx->println("m");

    gfx->println("--------------------------");

    gfx->print("Altitude (real) = ");
    gfx->print(bmpRealAltitude);
    gfx->println("m");
    gfx->println("--------------------------");
  } else if (screenNr == 3) {
    gfx->println("**************************");

    gfx->setTextSize(3);
    gfx->print("CO2: ");
    gfx->print(CO2Val);
    gfx->println("ppm");
    gfx->setTextSize(2);
    gfx->println("**************************");
  }
}

void drawProgress(Arduino_GFX *display, uint8_t percentage, String text)
{
  // clear part of the screen
  display->setCursor(0, 0);
  display->fillScreen(RGB565_BLACK);
  display->setTextColor(RGB565_GREEN);
  display->println(text);
  display->drawRect(10, 168, 240 - 20, 15, RGB565_WHITE);
  display->fillRect(12, 170, 216 * percentage / 100, 11, RGB565_GREEN);
}

void connectWifi(Arduino_GFX *display)
{
  WiFi.disconnect();
  WiFi.mode( WIFI_STA );
  if (WiFi.status() == WL_CONNECTED) return;
  //Manual Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (i > 80) i = 0;
    drawProgress(display, i, "Connecting to WiFi...");
    i += 10;
    Serial.print(".");
  }

  Serial.println("IP: ");
  Serial.println(WiFi.localIP());

  display->setCursor(0, 0);
  display->fillScreen(RGB565_BLACK);
  display->setTextColor(RGB565_GREEN);

  display->setTextSize(3);
  display->setCursor(00, 50);
  display->println("IP:");
  display->println(WiFi.localIP());

  delay(10000);
}

// Initialize LittleFS
void initLittleFS()
{
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFSmounted successfully");
}
