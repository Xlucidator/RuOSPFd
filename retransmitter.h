#ifndef RETRANSMITTER_H
#define RETRANSMITTER_H

#include <vector>
#include <stdint.h>
#include <pthread.h>
#include "ospf_packet.h"

/* Packet Data is the data part of an OSPF packet */
struct PacketData {
    /* packet info */
    const char* data;   // must comes from malloc
    size_t data_len;
    uint8_t type;
    uint32_t dst_ip;
    /* control part */
    uint32_t id;
    uint32_t age;
    uint32_t duration;

    PacketData(const char* data, size_t data_len, uint8_t type, uint32_t dst_ip, uint32_t duration);
    ~PacketData();
};

class Interface;

class Retransmitter {
public:
    Interface* host_interface;
    std::vector<PacketData> packet_list;
    pthread_mutex_t list_lock;
    uint32_t alloc_id;

    Retransmitter(Interface* interface);
    uint32_t addPacketData(PacketData packet_data);
    void     delPacketData(uint32_t id);
    static void* run(void* retransmitter);
};



#endif // RETRANSMITTER_H