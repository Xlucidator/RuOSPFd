#include "lsdb.h"


LSDB::LSDB() {
    pthread_mutex_init(&router_lock, NULL);
    pthread_mutex_init(&network_lock, NULL);

}

LSDB::~LSDB() {
    for (auto& p_lsa : router_lsas) {
        delete p_lsa;
    }
    for (auto& p_lsa : network_lsas) {
        delete p_lsa;
    }
}

LSARouter* LSDB::getRouterLSA(uint32_t link_state_id, uint32_t advertise_rtr_id) {
    for (auto& p_lsa: router_lsas) {
        if (p_lsa->lsa_header.link_state_id == link_state_id &&
            p_lsa->lsa_header.advertising_router == advertise_rtr_id) {
            return p_lsa;
        }
    }
    return nullptr;
}

LSARouter* LSDB::getRouterLSA(uint32_t link_state_id) {
    for (auto& p_lsa: router_lsas) {
        if (p_lsa->lsa_header.link_state_id == link_state_id) {
            return p_lsa;
        }
    }
    return nullptr;
}