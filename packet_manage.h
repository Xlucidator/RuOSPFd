#ifndef PACKET_MANAGE_H
#define PACKET_MANAGE_H

#include "interface.h"
#include <stdint.h>

uint16_t crc_checksum(const void* data, size_t len);
void sendPackets(const char* ospf_data, int data_len, uint8_t type, uint32_t dst_ip, Interface* interface);

/* Send */
void* threadSendHelloPackets(void* intf);
void* threadSendEmptyDDPackets(void* nbr);
void* threadSendLSRPackets(void* nbr);
/* Recv */
void* threadRecvPackets(void *intf);



#endif // PACKET_MANAGE_H
