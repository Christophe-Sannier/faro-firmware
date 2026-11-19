#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS    17
#define TFT_DC    14
#define TFT_RST   15
#define TFT_BL    16
#define TFT_SCLK  12
#define TFT_MOSI  11

Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

int currentTest = 0;
unsigned long lastChange = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n=== TEST VARIANTES D'INIT ===\n");
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  delay(100);
  
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, -1); // On ne force pas le CS ici
  delay(100);
  
  testNextVariant();
}

void testNextVariant() {
  Serial.printf("\n--- TEST %d ---\n", currentTest);
  
  // Réinitialiser l'écran à chaque test
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  switch(currentTest) {
    case 0:
      Serial.println("Variante: INITR_MINI160x80");
      tft.initR(INITR_MINI160x80);
      break;
    case 1:
      Serial.println("Variante: INITR_MINI160x80 + invertDisplay");
      tft.initR(INITR_MINI160x80);
      tft.invertDisplay(true);
      break;
    case 2:
      Serial.println("Variante: INITR_MINI160x80_PLUGIN");
      tft.initR(INITR_MINI160x80_PLUGIN);
      break;
    case 3:
      Serial.println("Variante: INITR_MINI160x80_PLUGIN + invertDisplay");
      tft.initR(INITR_MINI160x80_PLUGIN);
      tft.invertDisplay(true);
      break;
    case 4:
      Serial.println("Variante: INITR_BLACKTAB");
      tft.initR(INITR_BLACKTAB);
      break;
    case 5:
      Serial.println("Variante: INITR_BLACKTAB + invertDisplay");
      tft.initR(INITR_BLACKTAB);
      tft.invertDisplay(true);
      break;
    case 6:
      Serial.println("Variante: INITR_REDTAB");
      tft.initR(INITR_REDTAB);
      break;
    case 7:
      Serial.println("Variante: INITR_REDTAB + invertDisplay");
      tft.initR(INITR_REDTAB);
      tft.invertDisplay(true);
      break;
    default:
      Serial.println("FIN DES TESTS");
      return;
  }
  
  tft.setRotation(1);
  
  // Test visuel : remplir en rouge
  tft.fillScreen(ST77XX_RED);
  Serial.println("✓ Affichage RED pendant 3 sec...");
  
  currentTest++;
  lastChange = millis();
}

void loop() {
  // Changer de variante toutes les 3 secondes
  if (millis() - lastChange >= 3000) {
    testNextVariant();
  }
  
  delay(100);
}