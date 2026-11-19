// ============================================================
//  PHOENIX - Firmware NOEUD TERRAIN - tache 4.3.2
//  Machine a etats finie (FSM) non-bloquante du noeud terrain.
//
//  Etats : INIT -> SENSING -> INFERENCE -> IDLE / ALERT_EMIT -> SENSING
//  Cycle nominal : 5 s, temporise par millis() (JAMAIS delay()).
//
//  Cohabite avec le relais mesh (LOT 3) : handleRadioReception()
//  est appele EN PREMIER a chaque tour de loop(), avant la FSM,
//  pour que le relais reste reactif.
// ============================================================

#include "phoenix_packet.h"   // struct PhoenixPacket (9 octets) + enum AlertLevel
#include "sensor.h"           // SensorReading, sensorInit(), readSensor()  (4.2.1)
#include "screen.h"           // ecranInit/ecranAfficherBoot/ecranAfficherEtat (4.3.4)
#include "decision.h"         // classifyMSE(), PHOENIX_MSE_THRESHOLD          (4.3.3)
#include "radio.h"            // broadcastAlert(), handleRadioReception(), radioInit() (LOT 3)
#include "inference.h"

// --- ID du noeud — A CHANGER avant de flasher chaque carte ---
#define NODE_ID 1             // 1, 2 ou 3

// --- Etats de la FSM ---
enum NodeState { STATE_INIT, STATE_SENSING, STATE_INFERENCE, STATE_IDLE, STATE_ALERT_EMIT };
NodeState currentState = STATE_INIT;

// --- Timing non bloquant ---
const unsigned long CYCLE_PERIOD_MS = 5000;
unsigned long lastCycleTime = 0;

// --- Fenetre glissante (contrat ML : N=8) ---
const int WINDOW_SIZE = 8;
SensorReading window[WINDOW_SIZE];
int windowHead = 0, windowCount = 0;

// --- Confirmation 2 cycles (ecrase les faux positifs) ---
const uint8_t CONFIRM_CYCLES = 2;
uint8_t consecutiveAlertCount = 0;

// --- Variables courantes ---
float      currentMSE   = 0.0f;
AlertLevel currentAlert = ALERT_NONE;
uint16_t   nextMessageId = 0;

// ===== lireBatterie — lecture ADC réelle, pont diviseur 1:2 (R3/R19 = 1MΩ/1MΩ) =====
float lireBatterie() {
  analogReadResolution(12);              // 12 bits = 0..4095
  int raw = analogRead(1);               // GPIO1 = BAT_ADC
  float vAdc = raw * (3.3f / 4095.0f);  // tension lue sur la broche
  float vBatt = vAdc * 2.0f;            // ×2 : pont diviseur exact 1:2
  return vBatt;
}

// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(300);   // stabilisation alimentation (critique sur batterie : 
                // laisse la LiPo encaisser le pic d'init radio sans brownout)

  if (!radioInit(NODE_ID)) {
    Serial.println("[FSM] Radio KO");
  }
  if (!inferenceInit()) {
    Serial.println("[FSM] IA KO — inferences desactivees");
  }
}

void loop() {
  // (1) Job radio/relais — non bloquant, AVANT la FSM, a CHAQUE tour (LOT 3)
  handleRadioReception();

  // (2) Machine a etats du noeud
  switch (currentState) {

    case STATE_INIT:
    if (sensorInit()) {
      ecranInit();
      ecranAfficherBoot(NODE_ID);
      lastCycleTime = millis();
      currentState = STATE_SENSING;
      Serial.println("[FSM] INIT -> SENSING");
    } else {
      Serial.println("[FSM] INIT : sensorInit() ECHEC — retry...");
      delay(500);  // attente avant retry
    }
      break;

    case STATE_SENSING:
      if (millis() - lastCycleTime >= CYCLE_PERIOD_MS) {
        lastCycleTime = millis();
        SensorReading r = readSensor();
        window[windowHead] = r;
        windowHead = (windowHead + 1) % WINDOW_SIZE;
        if (windowCount < WINDOW_SIZE) windowCount++;
        Serial.printf("[FSM] SENSING T=%.1f H=%.1f P=%.1f G=%.1f (buffer %d/%d)\n",
                      r.temperature, r.humidity, r.pressure, r.gasResistance,
                      windowCount, WINDOW_SIZE);
        currentState = STATE_INFERENCE;
      }
      break;

    case STATE_INFERENCE:
      if (windowCount < WINDOW_SIZE) {
        Serial.println("[FSM] INFERENCE : fenetre pas encore pleine -> IDLE");
        currentState = STATE_IDLE;
      } else {
        currentMSE = runInference(window);
        AlertLevel raw = classifyMSE(currentMSE);

        if (raw >= ALERT_SUSPICION) {
          if (consecutiveAlertCount < 255) consecutiveAlertCount++;
        } else {
          consecutiveAlertCount = 0;
        }

        currentAlert = (consecutiveAlertCount >= CONFIRM_CYCLES) ? raw : ALERT_NONE;
        Serial.printf("[FSM] INFERENCE MSE=%.4f raw=%u confirm=%u/%u -> niveau %u\n",
                      currentMSE, raw, consecutiveAlertCount, CONFIRM_CYCLES,
                      currentAlert);
        currentState = (currentAlert >= ALERT_SUSPICION) ? STATE_ALERT_EMIT : STATE_IDLE;
      }
      break;

    case STATE_IDLE:
      ecranAfficherEtat(NODE_ID, currentMSE, currentAlert, lireBatterie());
      currentState = STATE_SENSING;
      break;

    case STATE_ALERT_EMIT: {
      PhoenixPacket pkt;
      pkt.message_id      = nextMessageId++;
      pkt.origin_node_id  = NODE_ID;
      pkt.hop_count       = 0;
      pkt.alert_level     = currentAlert;
      pkt.battery_voltage = lireBatterie();
      broadcastAlert(pkt);
      ecranAfficherEtat(NODE_ID, currentMSE, currentAlert, pkt.battery_voltage);
      currentState = STATE_SENSING;
      break;
    }
  }
}