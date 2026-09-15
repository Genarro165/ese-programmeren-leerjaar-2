/*
 * BMP280.h
 * Arduino library for the Bosch BMP280 temperature/pressure sensor (I2C)
 *
 * Author: <your name>
 * v1.0
 */

#ifndef BMP280_H
#define BMP280_H

#include <Arduino.h>
#include <Wire.h>

// Default I2C addresses (depends on SDO pin: GND = 0x76, VCC = 0x77)
#define BMP280_I2C_ADDR_PRIMARY    0x76
#define BMP280_I2C_ADDR_SECONDARY  0x77

// Register addresses (see BMP280 datasheet, chapter 5.3)
#define BMP280_REG_ID              0xD0
#define BMP280_REG_RESET           0xE0
#define BMP280_REG_STATUS          0xF3
#define BMP280_REG_CTRL_MEAS       0xF4
#define BMP280_REG_CONFIG          0xF5
#define BMP280_REG_PRESS_MSB       0xF7
#define BMP280_REG_TEMP_MSB        0xFA
#define BMP280_REG_CALIB_START     0x88

#define BMP280_CHIP_ID             0x58   // expected value of BMP280_REG_ID
#define BMP280_RESET_VALUE         0xB6

// Oversampling settings for temperature/pressure
typedef enum {
    BMP280_SKIPPED      = 0x00,
    BMP280_OVERSAMP_1X   = 0x01,
    BMP280_OVERSAMP_2X   = 0x02,
    BMP280_OVERSAMP_4X   = 0x03,
    BMP280_OVERSAMP_8X   = 0x04,
    BMP280_OVERSAMP_16X  = 0x05
} bmp280_oversampling_t;

// Power modes
typedef enum {
    BMP280_MODE_SLEEP  = 0x00,
    BMP280_MODE_FORCED = 0x01,
    BMP280_MODE_NORMAL = 0x03
} bmp280_mode_t;

class BMP280 {
public:
    BMP280(uint8_t i2c_addr = BMP280_I2C_ADDR_PRIMARY, TwoWire *wire = &Wire);

    // Initialise the sensor. Returns false if the chip ID does not match.
    bool begin();

    // Configure oversampling and power mode
    void setSampling(bmp280_mode_t mode = BMP280_MODE_NORMAL,
                      bmp280_oversampling_t tempSampling = BMP280_OVERSAMP_2X,
                      bmp280_oversampling_t pressSampling = BMP280_OVERSAMP_16X);

    // Trigger a one-shot (forced) measurement
    void takeForcedMeasurement();

    // Reads
    float readTemperature();   // degrees Celsius
    float readPressure();      // Pa
    float readAltitude(float seaLevelhPa = 1013.25f); // meters

    uint8_t getChipID();

private:
    uint8_t  _i2c_addr;
    TwoWire *_wire;
    int32_t  _t_fine;

    // Calibration coefficients (see datasheet chapter 3.11.2)
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

    void     readCalibrationData();
    void     writeRegister(uint8_t reg, uint8_t value);
    uint8_t  readRegister8(uint8_t reg);
    uint16_t readRegister16LE(uint8_t reg); // little-endian unsigned
    int16_t  readRegister16LE_signed(uint8_t reg);
    int32_t  readRawTemperature();
    int32_t  readRawPressure();
};

#endif // BMP280_H
