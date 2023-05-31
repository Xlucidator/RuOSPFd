#ifndef PACKET_MANAGE_H
#define PACKET_MANAGE_H

#include "common.h"
#include "interface.h"

uint16_t packet_checksum(const void* data, size_t len);

void* threadSendHelloPackets(void* intf);


#endif // PACKET_MANAGE_H
