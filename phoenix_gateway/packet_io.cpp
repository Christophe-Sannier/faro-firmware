#include "packet_io.h"
#include <stdio.h>

void packetToJson(const PhoenixPacket& pkt, char* out, size_t outSize) {
  // Contrat fige avec Marius (LOT 6) :
  //   node = origin_node_id, alert = alert_level, hops = hop_count, batt = tension
  // snprintf : pas besoin d'une lib JSON pour 4 champs fixes, et borne la taille.
  snprintf(out, outSize,
    "{\"node\":%u,\"alert\":%u,\"hops\":%u,\"batt\":%.2f}",
    (unsigned)pkt.origin_node_id, (unsigned)pkt.alert_level,
    (unsigned)pkt.hop_count, pkt.battery_voltage);
}
