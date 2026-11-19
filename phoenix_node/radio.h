#ifndef RADIO_H
#define RADIO_H

#include "phoenix_packet.h"

// === API publique du LOT 3 (Léon) ===

// À appeler UNE FOIS dans le setup() du noeud terrain.
// Initialise le bus SPI radio, le SX1262, les interruptions et l'écoute.
// Retourne true si l'init radio a réussi, false sinon.
bool radioInit(uint8_t nodeId);

// Émission broadcast d'un paquet (gère le duty cycle 1% et le RXEN).
// Appelée par la FSM dans l'état ALERT_EMIT.
void broadcastAlert(PhoenixPacket& pkt);

// Traite les interruptions radio (TX terminé / paquet reçu + relais mesh).
// À appeler EN PREMIER, à chaque tour de loop(), AVANT la FSM.
void handleRadioReception();

#endif