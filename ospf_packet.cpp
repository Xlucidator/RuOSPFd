#include "ospf_packet.h"

/* ============ LSA Header ============ */
LSAHeader::LSAHeader() {
    ls_age = 100;
    options = 0x2;
    ls_sequence_number = 0x80000000; // init
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

}

size_t LSARouter::size() {
    return LSAHDR_LEN + 4 + sizeof(LSARouterLink) * links.size();
}

bool LSARouter::operator==(const LSARouter& other) {    // TODO: lsa update? i don't think it's necessary to compare all
    if (lsa_header.link_state_id      != other.lsa_header.link_state_id &&
        lsa_header.advertising_router != other.lsa_header.advertising_router &&
        lsa_header.length             != other.lsa_header.length &&
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