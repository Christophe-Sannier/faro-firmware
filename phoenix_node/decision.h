#ifndef DECISION_H
#define DECISION_H

#include "phoenix_packet.h"   // pour AlertLevel

// Largeur de la bande de suspicion (0.5 * seuil, point du contrat IA).
extern const float PHOENIX_SUSPICION_RATIO;

// Fonction PURE : meme entree -> meme sortie, aucun effet de bord.
// Pure -> testable sans carte (ideal pour 4.5.2).
AlertLevel classifyMSE(float mse);

#endif
