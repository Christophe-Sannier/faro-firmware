#ifndef INFERENCE_H
#define INFERENCE_H

#include "sensor.h"  // pour SensorReading

// Initialise l'interpréteur TFLite Micro.
// À appeler UNE FOIS dans setup(). Retourne true si succès.
bool inferenceInit();

// Fait tourner l'autoencodeur sur la fenêtre glissante (8 lectures, ordre
// chronologique : [0] = plus ancien ... [7] = plus récent).
// Retourne le MSE (>= 0) entre entrée normalisée et reconstruction,
// ou -1.0f si l'IA est indisponible (init échouée).
float runInference(SensorReading* win);

#endif
