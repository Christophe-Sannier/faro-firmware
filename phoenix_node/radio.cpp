#include "radio.h"
#include <SPI.h>
#include <RadioLib.h>
#include "radio_config.h"

// --- BUS SPI DÉDIÉ RADIO (séparé du bus écran LCD) ---
static SPIClass spiRadio(HSPI);
static SX1262 radio = new Module(PHX_LORA_NSS, PHX_LORA_DIO1, PHX_LORA_RST,
                                 PHX_LORA_BUSY, spiRadio);

static uint8_t monNodeId = 0; // initialisé par radioInit()

// --- ÉTAT ET ASYNCHRONISME ---
static volatile bool radioActionFlag = false;
static bool radioBusy = false;
static uint16_t monCompteurMessage = 0;
static const uint8_t MAX_HOPS = 5;

// --- DUTY CYCLE (fenêtre glissante 1h) ---
static const unsigned long UNE_HEURE_MS             = 3600000;
static const unsigned long AIRTIME_MAX_PAR_HEURE_MS = 36000;
static const unsigned long TOA_PAQUET_MS            = 42;

static const uint8_t TAILLE_HISTORIQUE_TX = 100;
static unsigned long historiqueTransmissions[TAILLE_HISTORIQUE_TX];
static uint8_t indexHistoriqueTx = 0;

static unsigned long calculerAirtimeCumule(unsigned long tempsActuel) {
  unsigned long airtimeTotal = 0;
  for (uint8_t i = 0; i < TAILLE_HISTORIQUE_TX; i++) {
    if (historiqueTransmissions[i] != 0 &&
        (tempsActuel - historiqueTransmissions[i] < UNE_HEURE_MS)) {
      airtimeTotal += TOA_PAQUET_MS;
    }
  }
  return airtimeTotal;
}

static void enregistrerEmission(unsigned long tempsActuel) {
  historiqueTransmissions[indexHistoriqueTx] = tempsActuel;
  indexHistoriqueTx = (indexHistoriqueTx + 1) % TAILLE_HISTORIQUE_TX;
}

// --- TABLE ANTI-DOUBLON (16 entrées, rétention 60s) ---
struct MessageVu {
  uint16_t message_id;
  uint8_t  origin_node_id;
  unsigned long timestamp;
};

static const uint8_t TAILLE_TABLE_DOUBLON  = 16;
static const unsigned long DELAI_RETENTION_MS = 60000;
static MessageVu tableAntiDoublon[TAILLE_TABLE_DOUBLON];
static uint8_t indexTableDoublon = 0;

// --- ISR ULTRA-LÉGÈRE ---
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
static void setFlag(void) {
  radioActionFlag = true;
}

// ====================================================================
// radioInit — appelée depuis setup() du noeud terrain
// ====================================================================
bool radioInit(uint8_t nodeId) {
  monNodeId = nodeId; // mémorisé pour le filtrage d'écho

  // RXEN : broche d'activation RX, configurée avant tout
  pinMode(PHX_LORA_RXEN, OUTPUT);
  digitalWrite(PHX_LORA_RXEN, HIGH); // mode réception par défaut

  spiRadio.begin(PHX_LORA_SCK, PHX_LORA_MISO, PHX_LORA_MOSI, PHX_LORA_NSS);

  int state = radio.begin(
    PHX_FREQ_MHZ, PHX_BW_KHZ, PHX_SF, PHX_CR,
    PHX_SYNC_WORD, PHX_TX_POWER_DBM, PHX_PREAMBLE_LEN,
    PHX_TCXO_VOLTAGE
  );

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[RADIO] ECHEC init, code : ")); Serial.println(state);
    return false;
  }

  radio.setDio2AsRfSwitch(true);
  radio.setPacketSentAction(setFlag);
  radio.setPacketReceivedAction(setFlag);

  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("[RADIO] Echec startReceive."));
    return false;
  }

  // Nettoyage des structures
  for (uint8_t i = 0; i < TAILLE_TABLE_DOUBLON; i++) {
    tableAntiDoublon[i] = {0, 0, 0};
  }
  for (uint8_t i = 0; i < TAILLE_HISTORIQUE_TX; i++) {
    historiqueTransmissions[i] = 0;
  }

  Serial.println(F("[RADIO] SX1262 initialise, en ecoute."));
  return true;
}

// ====================================================================
// broadcastAlert — émission broadcast non-bloquante
// ====================================================================
void broadcastAlert(PhoenixPacket& pkt) {
  unsigned long tempsActuel = millis();
  unsigned long airtimeActuel = calculerAirtimeCumule(tempsActuel);

  if (airtimeActuel + TOA_PAQUET_MS > AIRTIME_MAX_PAR_HEURE_MS) {
    Serial.print(F("[DUTY CYCLE] Blocage emission. Airtime (ms) : "));
    Serial.println(airtimeActuel);
    return;
  }

  Serial.print(F("[TX] Msg=")); Serial.print(pkt.message_id);
  Serial.print(F(" Origin=")); Serial.print(pkt.origin_node_id);
  Serial.print(F(" Level=")); Serial.println(pkt.alert_level);

  digitalWrite(PHX_LORA_RXEN, LOW); // bascule en émission
  radioBusy = true;
  enregistrerEmission(tempsActuel);

  int state = radio.startTransmit((uint8_t*)&pkt, sizeof(PhoenixPacket));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[RADIO] Erreur startTransmit : ")); Serial.println(state);
    radioBusy = false;
    digitalWrite(PHX_LORA_RXEN, HIGH);
    radio.startReceive();
  }
  // Si succès : RXEN repasse HIGH dans handleRadioReception() au TX_DONE.
}

// ====================================================================
// handleRadioReception — appelée EN PREMIER dans loop(), avant la FSM
// ====================================================================
void handleRadioReception() {
  if (radioActionFlag == false) return; // rien à traiter, sortie immédiate

  radioActionFlag = false;
  unsigned long tempsActuel = millis();

  uint16_t irqFlags = radio.getIrqFlags();

  // --- CAS A : émission terminée ---
  if (irqFlags & RADIOLIB_SX126X_IRQ_TX_DONE) {
    radioBusy = false;
    digitalWrite(PHX_LORA_RXEN, HIGH); // retour en réception
    radio.startReceive();
    Serial.println(F("[RADIO] TX termine. Retour ecoute."));
  }

  // --- CAS B : paquet reçu ---
  else if (irqFlags & RADIOLIB_SX126X_IRQ_RX_DONE) {
    PhoenixPacket paquetRecu;
    int state = radio.readData((uint8_t*)&paquetRecu, sizeof(PhoenixPacket));

    if (state != RADIOLIB_ERR_NONE) {
      Serial.println(F("[RADIO] Paquet corrompu (CRC). Rejete."));
      radio.startReceive();
      return;
    }

    Serial.println(F("\n--- PAQUET RECU ---"));
    Serial.print(F("  Origin=")); Serial.print(paquetRecu.origin_node_id);
    Serial.print(F(" Msg=")); Serial.print(paquetRecu.message_id);
    Serial.print(F(" Level=")); Serial.print(paquetRecu.alert_level);
    Serial.print(F(" Hops=")); Serial.println(paquetRecu.hop_count);

    // 1. Filtrage écho
    if (paquetRecu.origin_node_id == monNodeId) {
      Serial.println(F("[RELAIS] Echo local. Rejete."));
      radio.startReceive();
      return;
    }

    // 2. Anti-boucle
    if (paquetRecu.hop_count >= MAX_HOPS) {
      Serial.println(F("[RELAIS] MAX_HOPS atteint. Detruit."));
      radio.startReceive();
      return;
    }

    // 3. Anti-doublon (rétention 60s)
    bool estUnDoublonActif = false;
    for (uint8_t i = 0; i < TAILLE_TABLE_DOUBLON; i++) {
      if (tableAntiDoublon[i].message_id     == paquetRecu.message_id &&
          tableAntiDoublon[i].origin_node_id == paquetRecu.origin_node_id) {
        if (tempsActuel - tableAntiDoublon[i].timestamp < DELAI_RETENTION_MS) {
          estUnDoublonActif = true;
          break;
        }
      }
    }

    if (estUnDoublonActif) {
      Serial.println(F("[RELAIS] Doublon actif. Rejete."));
      radio.startReceive();
      return;
    }

    // Enregistrement de la signature
    tableAntiDoublon[indexTableDoublon].message_id     = paquetRecu.message_id;
    tableAntiDoublon[indexTableDoublon].origin_node_id = paquetRecu.origin_node_id;
    tableAntiDoublon[indexTableDoublon].timestamp      = tempsActuel;
    indexTableDoublon = (indexTableDoublon + 1) % TAILLE_TABLE_DOUBLON;

    // 4. Relais conditionnel (seules les alertes >= SUSPICION sont relayées)
    if (paquetRecu.alert_level >= ALERT_SUSPICION) {
      if (!radioBusy) {
        Serial.println(F("[RELAIS] Relais du paquet."));
        paquetRecu.hop_count++;           // seul champ modifié (contrat)
        broadcastAlert(paquetRecu);
      }
    } else {
      Serial.println(F("[RELAIS] Niveau NONE. Pas de relais."));
      radio.startReceive();
    }
  }
}