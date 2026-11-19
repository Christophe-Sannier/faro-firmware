#ifndef SCREEN_H
#define SCREEN_H

#include "phoenix_packet.h"   // pour AlertLevel

void ecranInit();
void ecranAfficherBoot(uint8_t nodeId);
void ecranAfficherEtat(uint8_t nodeId, float mse, AlertLevel alertLevel, float battV);

#endif
