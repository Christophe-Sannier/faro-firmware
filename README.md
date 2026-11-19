# FARO – Réseau de capteurs LoRa pour la détection précoce des feux de forêt

Firmware d'un réseau maillé (*mesh*) de capteurs autonomes : chaque nœud mesure son environnement, détecte une anomalie grâce à un petit modèle d'IA exécuté sur la carte, et relaie l'alerte de proche en proche par radio LoRa jusqu'à une passerelle reliée à un PC.

🏆 **Prix de l'Environnement – Journée des Projets ESIEE Paris**
Projet d'équipe de 6 élèves ingénieurs (anciennement nommé *PHOENIX*, d'où le préfixe `phoenix_` dans le code).

<!-- TODO : ajouter une photo d'un nœud et/ou de la démo -->
<!-- ![Nœud FARO](docs/images/noeud.jpg) -->

---

## Fonctionnement

```
 Nœud 1 ─┐                                         ┌──────────────┐
 Nœud 2 ─┼──► mesh LoRa 868 MHz ──► Passerelle ──USB/JSON──► │ Interface PC │
 Nœud 3 ─┘   (relais par inondation)                         └──────────────┘
```

Chaque **nœud terrain** (ESP32-S3 + radio LoRa SX1262 + capteur BME688 + écran) :

1. mesure température, humidité, pression et résistance de gaz toutes les 5 s ;
2. garde les 8 dernières mesures dans une fenêtre glissante ;
3. passe cette fenêtre dans un modèle **TensorFlow Lite Micro** quantifié en INT8, qui tente de la reconstruire. L'écart entre l'entrée et la sortie (erreur quadratique moyenne, MSE) est comparé à un seuil : plus il est grand, plus la situation est anormale ;
4. classe la situation en 3 niveaux : **RAS / Suspicion / Feu**, avec une confirmation sur 2 cycles consécutifs pour éliminer les fausses alertes ;
5. en cas d'alerte, diffuse un paquet radio de 9 octets et l'affiche sur son écran.

Les nœuds **relaient les alertes des autres** (inondation), avec filtrage des échos, limite du nombre de sauts et table anti-doublons.

La **passerelle** écoute en continu, vérifie chaque paquet et le convertit en JSON sur le port USB pour l'interface PC :

```json
{"node":1,"alert":2,"hops":1,"batt":3.85}
```

---

## Ma contribution

- **Chef de projet** de l'équipe.
- **Responsable du firmware (Lot 2)** :
  - machine à états non bloquante du nœud terrain (`phoenix_node.ino`) ;
  - pilote du capteur BME688 en I2C (`sensor.cpp`), affichage sur l'écran (`screen.cpp`), mesure de la batterie, logique de décision (`decision.cpp`) ;
  - **intégration** dans le firmware du modèle d'IA de Jean et du protocole de relais par inondation de Léon ;
  - firmware de la **passerelle** (`phoenix_gateway/`) ;
  - définition des **contrats d'interface** entre les lots : format binaire du paquet radio (`phoenix_packet.h`), configuration radio commune (`radio_config.h`), format JSON pour l'interface PC.

### Crédits

| Partie | Auteur |
|---|---|
| Firmware nœud, passerelle, intégration | Christophe Sannier |
| Principe de relais par inondation (base de `radio.cpp`) | Léon Rostand |
| Modèle d'IA et son entraînement (base de `inference.cpp`, `faro_model*.h`) | Jean Catherine |
| Interface PC (lecture du JSON) | Marius Velluet |
| Modélisation 3D du boîtier | Benjamin Sannier |
<!-- TODO : vérifier les noms et ajouter les autres membres de l'équipe et leur rôle -->

---

## Démarche

- **Développement avec des bouchons** : la machine à états a d'abord été écrite avec de fausses fonctions provisoires à la place de la radio et de l'IA, pour avancer sans attendre que ces modules soient prêts, puis les vrais modules ont été branchés un par un.
- **Contrats d'interface figés** entre les lots (paquet de 9 octets vérifié à la compilation, configuration radio unique, format JSON) pour que chacun puisse travailler en parallèle.
- **Sketch d'auto-test de la passerelle** (`tools/phoenix_gateway_selftest`) : génère de faux paquets au bon format pour que l'interface PC puisse être développée avant que la radio fonctionne.

---

## Points techniques

| Sujet | Choix |
|---|---|
| Radio | LoRa 868 MHz, SF7, BW 125 kHz, CR 4/5, mot de synchronisation privé |
| Réglementation | Limite d'émission de 36 s par heure glissante (1 % de *duty cycle*) gérée dans le firmware |
| Paquet | Structure *packed* de 9 octets : id du message, nœud d'origine, nombre de sauts, niveau d'alerte, tension batterie |
| Temps réel | Boucle non bloquante cadencée par `millis()` ; interruptions radio réduites à la levée d'un drapeau |
| IA embarquée | TensorFlow Lite Micro, modèle INT8, arène mémoire de 32 Ko |

---

## Matériel

| Composant | Rôle | Qté | Prix unitaire |
|---|---|---|---|
| Carte ESP32-S3 LoRa DFR1195 (DFRobot) | Microcontrôleur + radio LoRa SX1262 | 4 (3 nœuds + 1 passerelle) | 22,00 € |
| [Capteur BME688 SEN0629](https://www.gotronic.fr/art-capteur-ai-env--bme688-ferm--47926.htm) | Température, humidité, pression, gaz | 3 | 17,90 € |
| [Batteries LiPo 3,7 V 3000 mAh (lot de 4)](https://www.amazon.fr/dp/B0F99KLK2Y) | Alimentation des nœuds | 1 lot | 24,99 € |
| [Module SunFlower DFR0559](https://www.gotronic.fr/art-module-sunflower-5-vcc-1-a-dfr0559-28266.htm) | Gestion de la recharge solaire | 1 | 8,65 € |
| [Cellule solaire SOL3W](https://www.gotronic.fr/art-cellule-solaire-sol3w-18996.htm) | Recharge | 1 | 19,00 € |
| **Total** | | | **≈ 194 €** |

---

## Organisation du dépôt

```
faro-firmware/
├── phoenix_node/        Firmware du nœud terrain
├── phoenix_gateway/     Firmware de la passerelle
├── tools/
│   ├── phoenix_gateway_selftest/   Faux paquets JSON, sans radio
│   └── screen_diagnostic/          Test de l'écran
└── docs/images/         Photos
```

## Compiler

Environnement : **Arduino IDE** avec le paquet de cartes **ESP32** (Espressif), carte ESP32-S3.

Bibliothèques : `RadioLib`, `DFRobot_BME68x`, `Adafruit GFX`, `Adafruit ST7735`, `Chirale_TensorFlowLite`.

Ouvrir `phoenix_node/phoenix_node.ino` (ou `phoenix_gateway/phoenix_gateway.ino`), puis **modifier `NODE_ID` (1, 2 ou 3) avant de flasher chaque nœud**.

---

## Limites et pistes d'amélioration

- Un seul nœud équipé de l'alimentation solaire (prototype).
- Pas encore de tests unitaires automatisés dans ce dépôt.
- Modèle entraîné sur des données limitées : seuil d'alerte à valider sur le terrain.
