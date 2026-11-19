#ifndef PACKET_IO_H
#define PACKET_IO_H

#include "phoenix_packet.h"
#include <stddef.h>

// Serialise un PhoenixPacket au format JSON du contrat LOT 6 :
//   {"node":1,"alert":2,"hops":1,"batt":3.85}
// Utilise EN PRODUCTION par la passerelle, et EN TEST par test_packet.cpp.
// Une seule source de verite pour le format -> ce qu'on teste = ce qui tourne.
void packetToJson(const PhoenixPacket& pkt, char* out, size_t outSize);

#endif
