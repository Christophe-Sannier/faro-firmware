#include "decision.h"
#include "faro_model_params.h"   // FARO_MSE_THRESHOLD vient d'ici
static const float PHOENIX_MSE_THRESHOLD = FARO_MSE_THRESHOLD;
const float PHOENIX_SUSPICION_RATIO = 0.85f;

AlertLevel classifyMSE(float mse) {
  if (mse >= PHOENIX_MSE_THRESHOLD) {
    return ALERT_FIRE;        // niveau 2
  }
  if (mse >= PHOENIX_SUSPICION_RATIO * PHOENIX_MSE_THRESHOLD) {
    return ALERT_SUSPICION;   // niveau 1
  }
  return ALERT_NONE;          // niveau 0
}