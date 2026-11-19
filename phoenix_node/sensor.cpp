#include "sensor.h"
#include <Wire.h>
#include "DFRobot_BME68x.h"

// Adresse I2C 0x76 : confirmee par le schema officiel SEN0629
// (le 0x77 du tout premier code etait incorrect pour ce module).
DFRobot_BME68x_I2C bme(0x76);

bool sensorInit() {
  Wire.begin(8, 9);

  // Reset logiciel du BME688 : force un état propre indépendamment
  // de l'historique d'alimentation (critique sur batterie sans USB).
  // Registre 0xE0 = reset, valeur 0xB6 = commande de reset (datasheet §5.3).
  Wire.beginTransmission(0x76);
  Wire.write(0xE0);   // adresse du registre reset
  Wire.write(0xB6);   // valeur magic de reset
  Wire.endTransmission();
  delay(10);          // le BME688 met ~2 ms pour redémarrer, 10 ms de marge

  if (bme.begin() != 0) return false;
  bme.setGasHeater(320, 100);
  return true;
}

SensorReading readSensor() {
  SensorReading r;
  bme.startConvert();
  delay(150);                   // chauffe du gaz : le seul delay() tolere (~150 ms / 5 s)
  bme.update();
  r.temperature   = bme.readTemperature() /100.0f;
  r.humidity      = bme.readHumidity() /1000.0f;
  r.pressure      = bme.readPressure() / 100.0f;     // Pa -> hPa
  r.gasResistance = bme.readGasResistance() / 1000.0f; // Ohm -> kOhm
  return r;
}
