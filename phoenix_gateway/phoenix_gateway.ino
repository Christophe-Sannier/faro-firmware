// ============================================================
//  PHOENIX - Firmware PASSERELLE (Noeud Mere) - tache 4.4.1
//  Recoit les PhoenixPacket en LoRa -> sort du JSON sur USB.
//  Ne transmet jamais, ne relaie jamais (collecte pure).
//
//  Chaine : ecoute LoRa continue -> paquet recu (IRQ DIO1)
//           -> memcpy 9 octets -> PhoenixPacket -> JSON sur USB.
//  Le JSON est le contrat avec le pyserial de Marius (LOT 6).
// ============================================================
#include <RadioLib.h>
#include "phoenix_packet.h"   // MEME header partage que les noeuds terrain
#include "radio_config.h"     // MEME config radio que les noeuds terrain
#include "packet_io.h"        // serialisation JSON partagee (testee en 4.5.2)

// SX1262 avec le brochage DFR1195 : Module(NSS, DIO1, RST, BUSY)
SX1262 radio = new Module(PHX_LORA_NSS, PHX_LORA_DIO1, PHX_LORA_RST, PHX_LORA_BUSY);

// Drapeau leve par l'interruption "paquet recu". volatile : modifie en ISR.
volatile bool packetFlag = false;
void IRAM_ATTR onPacket() { packetFlag = true; }   // ISR : juste lever le drapeau

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }                 // attendre l'USB CDC (ESP32-S3)

  // Chemin RX toujours actif : la passerelle ne fait QUE recevoir.
  pinMode(PHX_LORA_RXEN, OUTPUT);
  digitalWrite(PHX_LORA_RXEN, HIGH);

  // SPI sur les broches LoRa de la DFR1195 (bus distinct de l'ecran).
  SPI.begin(PHX_LORA_SCK, PHX_LORA_MISO, PHX_LORA_MOSI, PHX_LORA_NSS);

  // Init radio. Le DERNIER parametre (TCXO 1.8V) est VITAL sur cette carte :
  // sans lui, l'init echoue et la radio reste muette.
  int state = radio.begin(PHX_FREQ_MHZ, PHX_BW_KHZ, PHX_SF, PHX_CR,
                          PHX_SYNC_WORD, PHX_TX_POWER_DBM,
                          PHX_PREAMBLE_LEN, PHX_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("{\"error\":\"radio init echec, code %d\"}\n", state);
    while (true) { delay(1000); }       // sans radio, rien ne marchera
  }

  radio.setDio2AsRfSwitch(true);        // commutateur RF interne (piege DFR1195)

  // RadioLib >= 6.0 : setPacketReceivedAction.
  // Si vous etes en RadioLib 5.x, remplacer par : radio.setDio1Action(onPacket);
  radio.setPacketReceivedAction(onPacket);
  radio.startReceive();                 // ecoute continue
  Serial.println("{\"status\":\"gateway ready\"}");
}

void loop() {
  if (!packetFlag) return;              // rien recu -> on rend la main
  packetFlag = false;

  uint8_t buf[32];
  size_t len = radio.getPacketLength();
  if (len > sizeof(buf)) len = sizeof(buf);     // securite anti-debordement
  int state = radio.readData(buf, len);

  // On n'accepte QUE les paquets de la bonne taille ET dont le CRC est bon.
  // (RadioLib verifie le CRC materiel LoRa automatiquement.)
  if (state == RADIOLIB_ERR_NONE && len == sizeof(PhoenixPacket)) {
    PhoenixPacket pkt;
    memcpy(&pkt, buf, sizeof(PhoenixPacket));   // 9 octets -> struct (meme layout S3<->S3)

    char json[96];
    packetToJson(pkt, json, sizeof(json));      // module partage avec les tests
    Serial.println(json);
  }
  // sinon : paquet etranger ou corrompu -> ignore silencieusement

  radio.startReceive();                 // re-armer l'ecoute
}
