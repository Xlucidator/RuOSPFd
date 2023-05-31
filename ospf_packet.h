#ifndef OSPF_PACKET_H
#define OSPF_PACKET_H

#include <stdint.h>

#define IPPROTO_OSPF (89)

/* Packet Header */
struct OSPFHeader {
    uint8_t     version = 2;
    uint8_t     type;
    uint16_t    packet_length;
    uint32_t    router_id;
    uint32_t    area_id;
    uint16_t    checksum;
    uint16_t    autype;
    uint32_t    authentication[2];
};


/* Packet Data */
struct OSPFHello {
    /* struct OSPFHeader header; */
    uint32_t    network_mask;
    uint16_t    hello_interval;
    uint8_t     options;
    uint8_t     rtr_pri;
    uint32_t    router_dead_interval;
    uint32_t    designated_router;       // DR
    uint32_t    backup_designated_router; // BDR
    uint32_t    neighbor;
};

struct OSPFDD {
    uint16_t    interface_MTU;
    uint8_t     options;
    uint8_t     b_MS: 1;
    uint8_t     b_M : 1;
    uint8_t     b_I : 1;
    uint8_t     b_other: 5;
    uint32_t    sequence_number;
};

struct OSPFLSR {
    uint32_t    type;
    uint32_t    state_id;
    uint32_t    adverising_router;
};

struct OSPFLSU {
    uint32_t    num;
};

/* LSA related */



#endif // OSPF_PACKET_H

