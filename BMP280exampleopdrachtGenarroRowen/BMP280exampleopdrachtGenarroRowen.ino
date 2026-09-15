/*
 * BMP280_example.ino
 * Voorbeeldgebruik van de BMP280-library (I2C)
 *
 * Bedrading (GY-BMP280 module):
 *   VIN -> 3.3V of 5V (afhankelijk van je module)
 *   GND -> GND
 *   SCL -> A5 (Uno) / SCL pin
 *   SDA -> A4 (Uno) / SDA pin
 */

#include <Wire.h>
#include "BMP280.h"

// Standaard I2C-adres is 0x76; gebruik 0x77 als jouw module SDO naar VCC heeft
BMP280 bmp(BMP280_I2C_ADDR_PRIMARY);

void setup() {
    Serial.begin(9600);
    while (!Serial) { }

    if (!bmp.begin()) {
        Serial.println("BMP280 niet gevonden! Check bedrading en I2C-adres.");
        while (1) { delay(10); }
    }

    Serial.println("BMP280 gevonden, sensor geinitialiseerd.");
}

void loop() {
    Serial.print("Temperatuur: ");
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    Serial.print("Druk: ");
    Serial.print(bmp.readPressure() / 100.0f);
    Serial.println(" hPa");

    Serial.print("Geschatte hoogte: ");
    Serial.print(bmp.readAltitude(1013.25f)); // pas aan naar lokale zeeniveau-druk
    Serial.println(" m");

    Serial.println();
    delay(1000);
}
