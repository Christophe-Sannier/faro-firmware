#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

// Contrat de donnees capteur - partage avec le firmware de collecte de Jean.
// L'ORDRE et les UNITES sont figes : toute divergence entre l'inference en
// direct et la collecte d'entrainement provoque un decalage silencieux.
struct SensorReading {
  float temperature;    // degres Celsius
  float humidity;       // % humidite relative
  float pressure;       // hPa
  float gasResistance;  // kOhm
};

bool sensorInit();          // true si le BME688 repond
SensorReading readSensor(); // une lecture des 4 grandeurs

#endif
