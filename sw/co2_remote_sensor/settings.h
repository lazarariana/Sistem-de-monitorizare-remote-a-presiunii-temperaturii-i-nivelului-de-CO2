
// Setup
#define WIFI_SSID "Samsung-S24"
#define WIFI_PASS "Test1234"

#define LED_GREEN 3
#define LED_YELLOW 18
#define LED_RED 19
#define FAN_CTRL 8
#define BTN_PIN 9

/* SPI Bus */
/* ESP32-C3 various dev board  : CS:  7, DC:  2, RST:  1, BL:  3, SCK:  4, MOSI:  6, MISO: nil */
#define TFT_CS 10   // Chip select pin
#define TFT_RST 2   // Reset pin
#define TFT_DC 4    // Data/Command pin
#define TFT_MOSI 0  // SPI MOSI pin
#define TFT_SCLK 1  // SPI SCLK pin

/* Sunrise communication address, both for Modbus and I2C */
#define SUNRISE_ADDR 0x68

#define CO2_ENABLE 5

/* I2C Bus */
#define I2C_SDA 6
#define I2C_SCL 7

/***************************
 * End Settings
 **************************/