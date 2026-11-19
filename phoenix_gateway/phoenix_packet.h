#ifndef PHOENIX_PACKET_H
#define PHOENIX_PACKET_H

#include <stdint.h>

// Niveau d'alerte. enum type uint8_t -> directement assignable au champ
// alert_level du paquet, et utilisable comme valeur numerique dans les tests.
enum AlertLevel : uint8_t {
  ALERT_NONE      = 0,   // RAS
  ALERT_SUSPICION = 1,   // Suspicion
  ALERT_FIRE      = 2    // Feu avere
};

// Format binaire des messages LoRa. STRICTEMENT identique cote noeud terrain
// et cote passerelle. __packed__ : aucun octet de bourrage -> 9 octets pile.
struct __attribute__((__packed__)) PhoenixPacket {
  uint16_t message_id;       // compteur incremental PROPRE au noeud d'origine
  uint8_t  origin_node_id;   // 1, 2 ou 3 : noeud ayant CREE l'alerte
  uint8_t  hop_count;        // 0 a l'origine, +1 a chaque relais (LOT 3)
  uint8_t  alert_level;      // voir enum AlertLevel
  float    battery_voltage;  // tension LiPo du noeud d'origine, en Volts
};

// Garde-fou : si le compilateur ne fait pas 9 octets, l'erreur est levee
// a la compilation (et non a la demo). Corrige le "10" errone du WBS.
static_assert(sizeof(PhoenixPacket) == 9,
  "PhoenixPacket doit faire exactement 9 octets - verifier __packed__");

#endif
