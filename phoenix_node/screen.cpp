#include "screen.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Fonts/FreeMono9pt7b.h>   // police fournie avec Adafruit_GFX
#include <SPI.h>

#define TFT_CS   17
#define TFT_DC   14
#define TFT_RST  15
#define TFT_BL   16    // retroeclairage (actif HAUT)
#define TFT_PWR  48    // alim dalle (actif BAS !) - indispensable, sinon ecran noir
#define TFT_SCLK 12
#define TFT_MOSI 11

Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Memoire du dernier niveau affiche : on ne repeint le bandeau couleur QUE
// lorsqu'il change (anti-clignotement). -1 = "rien encore affiche".
static int lastAlertLevel = -1;

static uint16_t couleurNiveau(AlertLevel level) {
  if (level == ALERT_FIRE)      return ST77XX_RED;
  if (level == ALERT_SUSPICION) return ST77XX_YELLOW;
  return ST77XX_GREEN;
}

void ecranInit() {
  // 1. Alimenter la dalle : GPIO48 a l'etat BAS (c'etait LA cause de l'ecran noir)
  pinMode(TFT_PWR, OUTPUT);
  digitalWrite(TFT_PWR, LOW);

  // 2. Retroeclairage a l'etat HAUT
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 3. SPI ecran (GPIO11/12) + init dalle
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);                 // rotation validee sur la carte
  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeMono9pt7b);        // avec une police GFX, Y = ligne de base

  lastAlertLevel = -1;
}

void ecranAfficherBoot(uint8_t nodeId) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(0, 18);
  tft.print("PHOENIX");
  tft.setCursor(0, 40);
  tft.printf("NODE %u", (unsigned)nodeId);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 62);
  tft.print("Init...");
  lastAlertLevel = -1;   // force un rendu complet au premier ecranAfficherEtat()
}

void ecranAfficherEtat(uint8_t nodeId, float mse, AlertLevel alertLevel, float battV) {
  // 1) Premier rendu : nettoie l'ecran de boot + en-tete fixe
  if (lastAlertLevel == -1) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(0, 14);
    tft.printf("NODE %u", (unsigned)nodeId);
  }

  // 2) Bandeau couleur + libelle : seulement si le niveau a change
  if ((int)alertLevel != lastAlertLevel) {
    uint16_t c = couleurNiveau(alertLevel);
    tft.fillRect(0, 20, 160, 22, c);
    tft.setTextColor(ST77XX_BLACK, c);
    tft.setCursor(4, 36);
    if      (alertLevel == ALERT_NONE)      tft.print("RAS       ");
    else if (alertLevel == ALERT_SUSPICION) tft.print("SUSPICION ");
    else                                    tft.print("FEU AVERE!");
    lastAlertLevel = (int)alertLevel;
  }

  tft.fillRect(0, 46, 160, 34, ST77XX_BLACK);   // zone des 2 lignes MSE + Bat
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 58);
  tft.printf("MSE :%.4f", mse);
  tft.setCursor(0, 74);
  tft.printf("Bat :%.2fV", battV);
}
