#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

// ============================================================
//  Config radio PARTAGEE : noeuds terrain (LOT 3) ET passerelle (4.4.1).
//  CES VALEURS DOIVENT ETRE STRICTEMENT IDENTIQUES DES DEUX COTES.
//  Une seule valeur qui differe = zero reception.
//  -> Source unique de verite a partager avec Leon (5.1.1).
// ============================================================

// --- Parametres protocole (spec LOT 3 / tache 5.1.1) ---
#define PHX_FREQ_MHZ      868.0   // bande libre EU
#define PHX_BW_KHZ        125.0
#define PHX_SF            7       // SF7 : faible latence pour la demo
#define PHX_CR            5       // coding rate 4/5
#define PHX_SYNC_WORD     0x12    // reseau PRIVE (PAS 0x34 = LoRaWAN public !)
#define PHX_PREAMBLE_LEN  8
#define PHX_TX_POWER_DBM  14      // <= +14 dBm (ETSI). Ignore en reception pure.

// --- Brochage SX1262 propre a la DFR1195 (source : wiki DFRobot officiel) ---
#define PHX_LORA_MISO     5
#define PHX_LORA_MOSI     6
#define PHX_LORA_SCK      7
#define PHX_LORA_NSS      10
#define PHX_LORA_DIO1     4
#define PHX_LORA_BUSY     40
#define PHX_LORA_RST      41
#define PHX_LORA_RXEN     42      // active le chemin RX (propre a cette carte)

// --- Pieges materiels DFR1195 ---
#define PHX_TCXO_VOLTAGE  1.8     // TCXO 1.8V : OBLIGATOIRE, sinon l'init echoue
// (et DIO2 = commutateur RF interne -> radio.setDio2AsRfSwitch(true) dans le code)

#endif
