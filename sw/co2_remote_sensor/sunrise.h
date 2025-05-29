#include <Arduino.h>
#include <Wire.h>
#include <settings.h>

/* WARINING!!!
  Some wire driver implementations do not corectly implement Wire.endTransmission(false) function,
  so please check this before disable WIRE_WORKAROUND!
  For example, known "bad" implementations are: Nucleo STM32
 */
#define WIRE_WORKAROUND (0)
/* It could be necessary to disable ABC if sensor will be tested with CO2 concentrations below 400ppm. */
#define DISABLE_ABC (0)

const int ATTEMPTS = 5; /* Amount of wakeup attempts before time-out */
const int EEPROM_UPDATE_DELAY_MS = 25; /* It takes 25ms to write one EE register */

/* Register Addresses */
const uint8_t ERROR_STATUS = 0x01;
const uint8_t MEASUREMENT_MODE = 0x95;
const uint8_t METER_CONTROL = 0xA5;

/* Measurement modes */
const uint16_t CONTINUOUS = 0x0000;
const uint16_t SINGLE = 0x0001;
int readPeriodMs = 4000; /* Reading period, in milliseconds. Default is 4 seconds */

void reInitI2C()
{
  /* Initialize I2C and use default pins defined for the board */
  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin();

  /* Setup I2C clock to 100kHz */
  Wire.setClock(100000);
}

/* Workaround regarding BAD implementations of Wire.endTransmission(false) function */
int WireRequestFrom(uint8_t dev_addr, uint8_t bytes_numbers, uint8_t offset_to_read, bool stop)
{
  int error;

#if (WIRE_WORKAROUND == 1)
  error = Wire.requestFrom((uint8_t)dev_addr, (uint8_t)bytes_numbers, /* how many bytes */
                           (uint32_t)offset_to_read,                  /* from address*/
                           (uint8_t)1,                                /* Address size - 1 byte*/
                           stop);                                     /* STOP*/
#else
  Wire.beginTransmission(dev_addr);
  Wire.write(offset_to_read);  //starting register address, from which read data
  Wire.endTransmission(false);
  error = Wire.requestFrom((uint8_t)dev_addr, (uint8_t)bytes_numbers, (uint8_t)stop);
#endif

  return error;
}

/**
 * @brief  Wakes up the sensor by initializing a write operation
 *         with no data.
 *
 * @param  target:      The sensor's communication address
 * @note   This example shows a simple way to wake up the sensor.
 * @retval true if successful, false if failed
 */
bool _wakeup(uint8_t target)
{
  int attemps = ATTEMPTS;
  int error;

  /*
    * 0 - success
    * 1 - BUG in STM32 library
    * 2 - Received NACK on transmit of address
  */
  do {
    uint8_t byte_0;

    Wire.beginTransmission(target);
    error = Wire.endTransmission(true);
  } while (((error != 0) && (error != 2) && (error != 1)) && (--attemps > 0));

  /* STM32 driver can stack under some conditions */
  if (error == 4) {
    reInitI2C();
    return false;
  }

  return (attemps > 0);
}

/**
 * @brief  Enable or disable ABC.
 *
 * @param  target: The sensor's communication address
 * @param  enable: Set true to enable or false to disable ABC
 * @note   This example shows a simple way to enable/disable ABC.
 * It could be necessary to disable ABC if sensor will be tested with CO2 concentrations below 400ppm.
 * @retval None
 */
void setABC(uint8_t target, bool enable) {
  /* Wakeup */
  if (!(_wakeup(target))) {
    Serial.print("Failed to wake up sensor.");
    return;
  }

  /* Request values */
  int error = WireRequestFrom(target, 1, METER_CONTROL /* from address*/, true /* STOP*/);

  if (error != 1) {
    Serial.print("Failed to write to target. Error code : ");
    Serial.println(error);
    return;
  }

  uint8_t meterControl = Wire.read();
  if (enable) {
    Serial.println("Enabling ABC...");
    meterControl &= (uint8_t)~0x02U;
  } else {
    Serial.println("Disabling ABC...");
    meterControl |= 0x02U;
  }

  /* Wakeup */
  if (!(_wakeup(target))) {
    Serial.print("Failed to wake up sensor.");
    return;
  }

  Wire.beginTransmission(target);
  Wire.write(METER_CONTROL);
  Wire.write(meterControl);
  error = Wire.endTransmission(true);
  delay(EEPROM_UPDATE_DELAY_MS);

  if (error != 0) {
    Serial.print("Failed to send request. Error code: ");
    Serial.println(error);
    /* FATAL ERROR */
    while (true)
      ;
  }
}

/**
 * @brief  Reads and prints the sensor's current measurement mode,
 *         measurement period and number of samples.
 *
 * @param  target: The sensor's communication address
 * @note   This example shows a simple way to read the sensor's
 *         measurement configurations.
 * @retval None
 */
void read_sensor_config(uint8_t target) {
  /* Function variables */
  int error;
  int numBytes = 7;

  /* Wakeup */
  if (!(_wakeup(target))) {
    Serial.print("Failed to wake up sensor.");
    return;
  }

  /* Request values */
  error = WireRequestFrom(target, numBytes, MEASUREMENT_MODE /* from address*/, true /* STOP*/);
  if (error != numBytes) {
    Serial.print("Failed to write to target. Error code : ");
    Serial.println(error);
    return;
  }

  /* Read values */
  uint8_t measMode = Wire.read(); /* Measurement mode */
  uint8_t byteHi = Wire.read();   /* Measurement period */
  uint8_t byteLo = Wire.read();
  uint16_t measPeriod = ((int16_t)(int8_t)byteHi << 8) | (uint16_t)byteLo;

  /* Number of samples */
  byteHi = Wire.read();
  byteLo = Wire.read();
  uint16_t numSamples = ((int16_t)(int8_t)byteHi << 8) | (uint16_t)byteLo;

  /* ABCPeriod */
  byteHi = Wire.read();
  byteLo = Wire.read();
  uint16_t abcPeriod = ((int16_t)(int8_t)byteHi << 8) | (uint16_t)byteLo;

  /* Probabely the sensor will not go into sleep mode */
  /* Wakeup */
  if (!(_wakeup(target))) {
    Serial.print("Failed to wake up sensor.");
    return;
  }

  /* Request values */
  error = WireRequestFrom(target, 1, METER_CONTROL /* from address*/, true /* STOP*/);
  if (error != 1) {
    Serial.print("Failed to write to target. Error code : ");
    Serial.println(error);
    return;
  }

  uint8_t meterControl = Wire.read();

  Serial.print("Measurement Mode: ");
  Serial.println(measMode);

  readPeriodMs = measPeriod * 1000;

  Serial.print("Measurement Period, sec: ");
  Serial.println(measPeriod);

  Serial.print("Number of Samples: ");
  Serial.println(numSamples);

  if ((0U == abcPeriod) || (0xFFFFU == abcPeriod) || (meterControl & 0x02U)) {
    Serial.println("ABCPeriod: disabled");
  } else {
    Serial.print("ABCPeriod, hours: ");
    Serial.println(abcPeriod);
  }

  Serial.print("MeterControl: ");
  Serial.println(meterControl, HEX);

#if (DISABLE_ABC == 0)
  /* Automatic Baseline Calibration if co2 level drops below 400ppm */
  if ((meterControl & 0x02U) != 0) {
    setABC(target, true);
  }
#endif
}

/**
 * @brief  Changes the sensor's current measurement mode, if it's
 *         currently in single mode.
 *
 * @param  target: The sensor's communication address
 * @note   This example shows a simple way to change the sensor's
 *         measurement mode. The sensor has to be manually restarted after the
 *         changes.
 * @retval None
 */
void change_measurement_mode(uint8_t target)
{
  /* Function variables */
  int error;
  int numBytes = 1;

  /* Wakeup */
  if (!(_wakeup(target))) {
    Serial.print("Failed to wake up sensor.");
    /* FATAL ERROR */
    while (true);
  }

  /* Read Value */
  error = WireRequestFrom(target, numBytes /* how many bytes */, MEASUREMENT_MODE /* from address*/, true /* STOP*/);
  if (error != numBytes) {
    Serial.print("Failed to read measurement mode. Error code: ");
    Serial.println(error);
    /* FATAL ERROR */
    while (true)
      ;
  }

  /* Change mode if single */
  if (Wire.read() != CONTINUOUS) {
    /* Wakeup */
    if (!(_wakeup(target))) {
      Serial.print("Failed to wake up sensor.");
      /* FATAL ERROR */
      while (true)
        ;
    }

    Serial.println("Changing Measurement Mode to Continuous...");

    Wire.beginTransmission(target);
    Wire.write(MEASUREMENT_MODE);
    Wire.write(CONTINUOUS);
    error = Wire.endTransmission(true);
    delay(EEPROM_UPDATE_DELAY_MS);

    if (error != 0) {
      Serial.print("Failed to send request. Error code: ");
      Serial.println(error);
      /* FATAL ERROR */
      while (true)
        ;
    }
    Serial.println("Sensor restart is required to apply changes");
    while (true)
      ;
  }
}

/**
 * @brief  Reads and prints the sensor's current CO2 value and
 *         error status.
 *
 * @param  target: The sensor's communication address
 * @note   This example shows a simple way to read the sensor's
 *         CO2 measurement and error status.
 * @retval CO2 measurement
 */
uint16_t read_sensor_measurements(uint8_t target) {
  uint16_t co2Val;

  /* Function variables */
  int error;
  int numBytes = 7;

  /* Wakeup */
  if (!(_wakeup(target))) {
    Serial.print("Failed to wake up sensor.");
    return 0;
  }

  /* Request values */
  error = WireRequestFrom(target, numBytes /* how many bytes */, ERROR_STATUS /* from address*/, true /* STOP*/);
  if (error != numBytes) {
    Serial.print("Failed to read values. Error code: ");
    Serial.println(error);
    return 0;
  }

  /* Read values */
  /* Error status */
  uint8_t eStatus = Wire.read();

  /* Reserved */
  uint8_t byteHi = Wire.read();
  uint8_t byteLo = Wire.read();

  byteHi = Wire.read();
  byteLo = Wire.read();

  /* CO2 value */
  byteHi = Wire.read();
  byteLo = Wire.read();
  co2Val = ((int16_t)(int8_t)byteHi << 8) | (uint16_t)byteLo;

  Serial.print("CO2: ");
  Serial.print(co2Val);
  Serial.println(" ppm");

  Serial.print("Error Status: 0x");
  Serial.println(eStatus, HEX);

  return co2Val;
}
