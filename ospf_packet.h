#ifndef OSPF_PACKET_H
#define OSPF_PACKET_H

#include <stdint.h>
#include <vector>

#define IPPROTO_OSPF    (89)

#define IPHDR_LEN       (20)
#define IPHDR_SRCIP     (12)

#define OSPFHDR_LEN     (sizeof(OSPFHeader))
#define OSPF_HELLO_LEN  (OSPFHDR_LEN + sizeof(OSPFHello))   // exclude attached
#define OSPF_DD_LEN     (OSPFHDR_LEN + sizeof(OSPFDD))      // exclude attached

enum OSPFType: uint8_t {
    T_HELLO = 1,
    T_DD,
    T_LSR,
    T_LSU,
    T_LSAck
};

uint16_t fletcher_checksum(const void* data, size_t len);

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
    uint32_t    network_mask;
    uint16_t    hello_interval;
    uint8_t     options;
    uint8_t     rtr_pri;
    uint32_t    router_dead_interval;
    uint32_t    designated_router;       // DR
    uint32_t    backup_designated_router; // BDR
    uint32_t    neighbor;
    /* attached : neighbors (router id) */
};

struct OSPFDD {
    uint16_t    interface_MTU;
    uint8_t     options;
    uint8_t     b_MS: 1;
    uint8_t     b_M : 1;
    uint8_t     b_I : 1;
    uint8_t     b_other: 5;
    uint32_t    sequence_number;
    /* attached : lsa headers */
};

struct OSPFLSR {
    uint32_t    type;
    uint32_t    state_id;
    uint32_t    adverising_router;

    void net2host();    // Attention: be careful, no check before transition
};

struct OSPFLSU {
    uint32_t    num;
    /* attached : full lsa */
};

struct OSPFLSAck {
    /* attached : lsa headers */
};

/* LSA related */
#define LSAHDR_LEN      (sizeof(LSAHeader))

enum LSAType : uint8_t {
    LSA_ROUTER = 1,
    LSA_NETWORK,
    LSA_SUMNET,
    LSA_SUMASB,
    LSA_ASEXTERNAL,
};

enum LinkType : uint8_t {
    L_P2P = 1,
    L_TRANSIT,
    L_STUB,
    L_VIRTUAL,
};

/* LSA Header */
struct LSAHeader {
    uint16_t    ls_age;     // init + in transition
    uint8_t     options;    // init
    uint8_t     ls_type;            // assign
    uint32_t    link_state_id;      // assign + in transition
    uint32_t    advertising_router; // assign + in transition
    uint32_t    ls_sequence_number; // in transition
    uint16_t    ls_checksum;        // in transition
    uint16_t    length;             // in transition

    LSAHeader();
    void host2net();    // be careful！ has no check before transition
    void net2host();    // be careful！ has no check before transition
};

/* LSA Data */
struct LSARouterLink {
    uint32_t    link_id;
    uint32_t    link_data;
    uint8_t     type;
    uint8_t     tos_num = 0;    // we assume 0 for convenience
    uint16_t    metric;
    /* each tos */

    LSARouterLink();
    LSARouterLink(char* net_ptr);
    bool operator==(const LSARouterLink& other);
};

struct LSARouter {
    LSAHeader   lsa_header;
    /* data part */
    uint8_t     zero1 : 5;
        uint8_t     b_V : 1;    // Virtual : is virtual channel
        uint8_t     b_E : 1;    // External: is ASBR
        uint8_t     b_B : 1;    // Board   : is ABR
    uint8_t     zero2 = 0;
    uint16_t    link_num;
    std::vector<LSARouterLink> links;

    LSARouter();
    LSARouter(char* net_ptr);
    char* toRouterLSA();  // [attention]: need to be del
    size_t size();
    bool operator==(const LSARouter& other);
};

struct LSANetwork {
    LSAHeader   lsa_header;
    /* data part */
    uint32_t    network_mask;   // init
    std::vector<uint32_t> attached_routers; // assign: ip list

    LSANetwork();
    LSANetwork(char* net_ptr);
    char* toNetworkLSA(); // [attention]: need to be del
    size_t size();
    bool operator==(const LSANetwork& other);
};

#endif // OSPF_PACKET_H

