#include "ospf_packet.h"
#include <netinet/in.h>

/* ============ LSA Header ============ */
LSAHeader::LSAHeader() {
    ls_age = 100;
    options = 0x2;
    ls_sequence_number = 0x80000000; // init
}

void LSAHeader::host2net() {
    ls_age = htons(ls_age);
    link_state_id = htons(link_state_id);
    advertising_router = htons(advertising_router);
    ls_checksum = htons(ls_checksum);
    length = htons(length);
    ls_sequence_number = htonl(ls_sequence_number);
}

void LSAHeader::net2host() {
    ls_age = ntohs(ls_age);
    link_state_id = ntohs(link_state_id);
    advertising_router = ntohs(advertising_router);
    ls_checksum = ntohs(ls_checksum);
    length = ntohs(length);
    ls_sequence_number = ntohl(ls_sequence_number);
}

/* ============ Router Link ============ */
bool LSARouterLink::operator==(const LSARouterLink& other) {
    return link_id   == other.link_id && 
           link_data == other.link_data && 
           type      == other.type &&
           tos_num   == other.tos_num &&
           metric    == other.metric;
}


/* ============ Router LSA ============ */
LSARouter::LSARouter() {
    zero1 = 0;
    b_B = b_E = b_V = 0;
    zero2 = 0;
    link_num = 0;
}

char* LSARouter::toRouterLSA() {
    link_num = links.size();
    // TODO
}

size_t LSARouter::size() {
    return LSAHDR_LEN + 4 + sizeof(LSARouterLink) * links.size();
}

bool LSARouter::operator==(const LSARouter& other) {    // TODO: lsa update? i don't think it's necessary to compare all
    if (lsa_header.link_state_id      != other.lsa_header.link_state_id ||
        lsa_header.advertising_router != other.lsa_header.advertising_router ||
        lsa_header.length             != other.lsa_header.length ||
        link_num != other.link_num) return false;
    for (int i = 0; i < link_num; ++i) {
        if (!(links[i] == other.links[i])) 
            return false;
    }
    return true;
}


/* ============ Netword LSA ============ */
LSANetwork::LSANetwork() {
    network_mask = 0xffffff00;  // not good
}

char* LSANetwork::toNetworkLSA() {
    
    // TODO
}

size_t LSANetwork::size() {
    return LSAHDR_LEN + 4 + sizeof(uint32_t) * attached_routers.size();
}

bool LSANetwork::operator==(const LSANetwork& other) {
    if (lsa_header.link_state_id      != other.lsa_header.link_state_id ||
        lsa_header.advertising_router != other.lsa_header.advertising_router ||
        lsa_header.length             != other.lsa_header.length ||
        network_mask != other.network_mask ||
        attached_routers.size() != other.attached_routers.size()) return false;
    for (int i = 0; i < attached_routers.size(); ++i) {
        if (!(attached_routers[i] == other.attached_routers[i])) 
            return false;
    }
    return true;
}