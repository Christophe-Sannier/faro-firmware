#include "inference.h"
#include "faro_model.h"
#include "faro_model_params.h"
#include <Arduino.h>
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

static uint8_t tensor_arena[32 * 1024];
static tflite::MicroInterpreter* interpreter = nullptr;
static bool modelReady = false;

static const int kWindow   = 8;
static const int kFeatures = 4;
static const int kTensorLen = kWindow * kFeatures;  // 32

bool inferenceInit() {
  static tflite::MicroMutableOpResolver<2> resolver;
  resolver.AddFullyConnected();
  resolver.AddLogistic();

  const tflite::Model* model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("[IA] Erreur version schema TFLite.");
    return false;
  }

  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, sizeof(tensor_arena));
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("[IA] Erreur AllocateTensors (arene trop petite ?).");
    return false;
  }

  TfLiteTensor* in  = interpreter->input(0);
  TfLiteTensor* out = interpreter->output(0);
  int in_len  = 1; for (int d = 0; d < in->dims->size;  d++) in_len  *= in->dims->data[d];
  int out_len = 1; for (int d = 0; d < out->dims->size; d++) out_len *= out->dims->data[d];
  if (in_len != kTensorLen || out_len != kTensorLen) {
    Serial.printf("[IA] Dimensions inattendues : in=%d out=%d (attendu %d).\n",
                  in_len, out_len, kTensorLen);
    return false;
  }

  Serial.printf("[IA] TFLite Micro OK. Arena utilisee : %u / %u octets\n",
                (unsigned)interpreter->arena_used_bytes(), (unsigned)sizeof(tensor_arena));
  Serial.printf("[IA] Entree type=%d scale=%.8f zp=%d\n",
                in->type, in->params.scale, (int)in->params.zero_point);
  // Adresses des buffers : si in et out pointent au meme endroit, c'est la preuve du recyclage.
  Serial.printf("[IA] Adresse in=%p  out=%p\n",
                (void*)in->data.int8, (void*)out->data.int8);
  modelReady = true;
  return true;
}

float runInference(SensorReading* win) {
  if (!modelReady || !interpreter) return -1.0f;

  TfLiteTensor* input_tensor  = interpreter->input(0);
  TfLiteTensor* output_tensor = interpreter->output(0);

  // SAUVEGARDES de l'entree (AVANT Invoke, car l'arene est recyclee pendant Invoke)
  int8_t in_saved_q[kTensorLen];      // version quantifiee (pour affichage)
  float  in_saved_real[kTensorLen];   // version reelle normalisee (pour le MSE)

  // 1. Normalisation + remplissage du tenseur d'entree
  for (int i = 0; i < kWindow; i++) {
    float vals[4] = {
      win[i].temperature,
      win[i].humidity,
      win[i].pressure,
      win[i].gasResistance
    };
    for (int j = 0; j < kFeatures; j++) {
      int idx = i * kFeatures + j;
      float range      = NORM_MAX[j] - NORM_MIN[j];
      float normalized = (range > 0.0f) ? (vals[j] - NORM_MIN[j]) / range : 0.0f;

      if (input_tensor->type == kTfLiteInt8) {
        float   scale   = input_tensor->params.scale;
        int32_t zero_pt = input_tensor->params.zero_point;
        int32_t q = lroundf(normalized / scale) + zero_pt;
        if (q < -128) q = -128;
        if (q >  127) q =  127;
        input_tensor->data.int8[idx] = (int8_t)q;
        in_saved_q[idx]    = (int8_t)q;                    // <-- sauvegarde avant Invoke
        in_saved_real[idx] = (q - zero_pt) * scale;        // <-- valeur reelle sauvegardee
      } else {
        input_tensor->data.f[idx] = normalized;
        in_saved_q[idx]    = 0;
        in_saved_real[idx] = normalized;
      }
    }
  }

  // 2. Inference
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("[IA] Erreur Invoke.");
    return -1.0f;
  }

  // 3. MSE : on compare la SAUVEGARDE (avant Invoke) a la sortie (apres Invoke)
  float mse = 0.0f;
  for (int k = 0; k < kTensorLen; k++) {
    float in_val = in_saved_real[k];   // <-- valeur sauvegardee, PAS relue du tenseur
    float out_val;
    if (output_tensor->type == kTfLiteInt8) {
      float   s_out = output_tensor->params.scale;
      int32_t z_out = output_tensor->params.zero_point;
      out_val = (output_tensor->data.int8[k] - z_out) * s_out;
    } else {
      out_val = output_tensor->data.f[k];
    }
    float diff = in_val - out_val;
    mse += diff * diff;
  }
  mse /= (float)kTensorLen;

  // === DIAGNOSTIC V4 — montre l'entree sauvegardee, le tenseur APRES Invoke, et la sortie ===
  Serial.print("[DIAG-V4] in_saved q: ");
  for (int k = 0; k < 4; k++) Serial.printf("%d ", in_saved_q[k]);
  Serial.print(" | input_tensor APRES invoke: ");
  for (int k = 0; k < 4; k++) Serial.printf("%d ", input_tensor->data.int8[k]);
  Serial.print(" | out q: ");
  for (int k = 0; k < 4; k++) Serial.printf("%d ", output_tensor->data.int8[k]);
  Serial.printf(" | MSE=%.6f\n", mse);
  // === FIN DIAGNOSTIC ===

  return mse;
}