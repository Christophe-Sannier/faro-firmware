// ============================================================
//  PHOENIX - MINI-SKETCH AUTO-TEST PASSERELLE (sans radio)
//  Fabrique de faux PhoenixPacket et les crache en JSON sur l'USB,
//  au MEME format que la vraie passerelle (4.4.1).
//
//  But : debloquer Marius (LOT 6) AVANT que la radio fonctionne.
//  Il peut developper et valider son parser pyserial des maintenant.
//  A flasher sur n'importe quel ESP32 (radio non requise).
// ============================================================

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
}

void loop() {
  static uint16_t n = 0;
  uint8_t niveaux[3] = {0, 1, 2};               // RAS / Suspicion / Feu, en rotation
  char json[96];
  snprintf(json, sizeof(json),
    "{\"node\":%u,\"alert\":%u,\"hops\":%u,\"batt\":%.2f}",
    (unsigned)(1 + n % 3), (unsigned)niveaux[n % 3], 1u, 3.85);
  Serial.println(json);
  n++;
  delay(3000);                                  // un faux paquet toutes les 3 s
}
