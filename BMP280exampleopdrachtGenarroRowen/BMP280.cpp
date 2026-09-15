/*
 * BMP280.cpp
 * Arduino library for the Bosch BMP280 temperature/pressure sensor (I2C)
 */

#include "BMP280.h"

BMP280::BMP280(uint8_t i2c_addr, TwoWire *wire) {
    _i2c_addr = i2c_addr;
    _wire = wire;
    _t_fine = 0;
}

// --- Low level I2C helpers ---------------------------------------------

void BMP280::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->write(value);
    _wire->endTransmission();
}

uint8_t BMP280::readRegister8(uint8_t reg) {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->endTransmission(false); // repeated start
    _wire->requestFrom(_i2c_addr, (uint8_t)1);
    return _wire->read();
}

// Reads two bytes starting at 'reg' as little-endian, returns unsigned
uint16_t BMP280::readRegister16LE(uint8_t reg) {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(_i2c_addr, (uint8_t)2);
    uint8_t lsb = _wire->read();
    uint8_t msb = _wire->read();
    return (uint16_t)(msb << 8) | lsb;
}

int16_t BMP280::readRegister16LE_signed(uint8_t reg) {
    return (int16_t)readRegister16LE(reg);
}

// --- Setup ---------------------------------------------------------------

bool BMP280::begin() {
    _wire->begin();

    uint8_t id = readRegister8(BMP280_REG_ID);
    if (id != BMP280_CHIP_ID) {
        return false; // not a BMP280 (or wrong address / wiring)
    }

    // Soft reset
    writeRegister(BMP280_REG_RESET, BMP280_RESET_VALUE);
    delay(10);

    readCalibrationData();

    // Sensible defaults: normal mode, temp x2, pressure x16 oversampling
    setSampling(BMP280_MODE_NORMAL, BMP280_OVERSAMP_2X, BMP280_OVERSAMP_16X);

    return true;
}

uint8_t BMP280::getChipID() {
    return readRegister8(BMP280_REG_ID);
}

void BMP280::readCalibrationData() {
    dig_T1 = readRegister16LE(BMP280_REG_CALIB_START + 0);
    dig_T2 = readRegister16LE_signed(BMP280_REG_CALIB_START + 2);
    dig_T3 = readRegister16LE_signed(BMP280_REG_CALIB_START + 4);

    dig_P1 = readRegister16LE(BMP280_REG_CALIB_START + 6);
    dig_P2 = readRegister16LE_signed(BMP280_REG_CALIB_START + 8);
    dig_P3 = readRegister16LE_signed(BMP280_REG_CALIB_START + 10);
    dig_P4 = readRegister16LE_signed(BMP280_REG_CALIB_START + 12);
    dig_P5 = readRegister16LE_signed(BMP280_REG_CALIB_START + 14);
    dig_P6 = readRegister16LE_signed(BMP280_REG_CALIB_START + 16);
    dig_P7 = readRegister16LE_signed(BMP280_REG_CALIB_START + 18);
    dig_P8 = readRegister16LE_signed(BMP280_REG_CALIB_START + 20);
    dig_P9 = readRegister16LE_signed(BMP280_REG_CALIB_START + 22);
}

void BMP280::setSampling(bmp280_mode_t mode,
                          bmp280_oversampling_t tempSampling,
                          bmp280_oversampling_t pressSampling) {
    // ctrl_meas register: osrs_t[2:0] | osrs_p[2:0] | mode[1:0]
    uint8_t ctrl_meas = (tempSampling << 5) | (pressSampling << 2) | mode;
    writeRegister(BMP280_REG_CTRL_MEAS, ctrl_meas);
}

void BMP280::takeForcedMeasurement() {
    // Re-write ctrl_meas with FORCED mode to trigger one measurement,
    // then poll the status register until the measurement is done.
    uint8_t ctrl_meas = readRegister8(BMP280_REG_CTRL_MEAS);
    ctrl_meas = (ctrl_meas & 0xFC) | BMP280_MODE_FORCED;
    writeRegister(BMP280_REG_CTRL_MEAS, ctrl_meas);

    // Bit 3 of the status register (0xF3) is set while a conversion is running
    while (readRegister8(BMP280_REG_STATUS) & 0x08) {
        delay(1);
    }
}

// --- Raw ADC reads --------------------------------------------------------

int32_t BMP280::readRawTemperature() {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(BMP280_REG_TEMP_MSB);
    _wire->endTransmission(false);
    _wire->requestFrom(_i2c_addr, (uint8_t)3);

    uint32_t msb  = _wire->read();
    uint32_t lsb  = _wire->read();
    uint32_t xlsb = _wire->read();

    return (int32_t)((msb << 12) | (lsb << 4) | (xlsb >> 4));
}

int32_t BMP280::readRawPressure() {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(BMP280_REG_PRESS_MSB);
    _wire->endTransmission(false);
    _wire->requestFrom(_i2c_addr, (uint8_t)3);

    uint32_t msb  = _wire->read();
    uint32_t lsb  = _wire->read();
    uint32_t xlsb = _wire->read();

    return (int32_t)((msb << 12) | (lsb << 4) | (xlsb >> 4));
}

// --- Compensation (fixed-point, as specified in the BMP280 datasheet) ----

float BMP280::readTemperature() {
    int32_t adc_T = readRawTemperature();

    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12)
            * ((int32_t)dig_T3)) >> 14;

    _t_fine = var1 + var2;

    int32_t T = (_t_fine * 5 + 128) >> 8; // in 0.01 degC
    return T / 100.0f;
}

float BMP280::readPressure() {
    // t_fine must be up to date: call readTemperature() first (or here)
    readTemperature();

    int64_t var1, var2, p;
    int32_t adc_P = readRawPressure();

    var1 = (int64_t)_t_fine - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0) {
        return 0; // avoid division by zero
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);

    return (float)p / 256.0f; // Pa
}

float BMP280::readAltitude(float seaLevelhPa) {
    float pressure_hPa = readPressure() / 100.0f;
    // Standard international barometric formula
    return 44330.0f * (1.0f - pow(pressure_hPa / seaLevelhPa, 0.1903f));
}
