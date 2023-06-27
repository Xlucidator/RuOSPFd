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

LSANetwork* LSDB::getNetworkLSA(uint32_t link_state_id, uint32_t advertise_rtr_id) {
    for (auto& p_lsa: network_lsas) {
        if (p_lsa->lsa_header.link_state_id == link_state_id &&
            p_lsa->lsa_header.advertising_router == advertise_rtr_id) {
            return p_lsa;
        }
    }
    return nullptr;
}

LSANetwork* LSDB::getNetworkLSA(uint32_t link_state_id) {
    for (auto& p_lsa: network_lsas) {
        if (p_lsa->lsa_header.link_state_id == link_state_id) {
            return p_lsa;
        }
    }
    return nullptr;
}

void LSDB::addLSA(char* tar_ptr) {
    LSAHeader* lsa_header = (LSAHeader*) tar_ptr;
    
    if (lsa_header->ls_type == LSA_ROUTER) {
        LSARouter* rlsa = new LSARouter(tar_ptr);
        LSARouter* lsa_check = getRouterLSA(rlsa->lsa_header.link_state_id, rlsa->lsa_header.advertising_router);
        if (lsa_check != nullptr) {

        }
    } else if (lsa_header->ls_type == LSA_NETWORK) {

    }
}

void LSDB::delLSA(uint32_t link_state_id, uint32_t advertise_rtr_id, uint8_t type) {
    if (type == LSA_ROUTER) {
        pthread_mutex_lock(&router_lock);
        for (auto it = router_lsas.begin(); it != router_lsas.end(); ++it) {
            LSARouter* lsa = *it;
            if (lsa->lsa_header.link_state_id == link_state_id && lsa->lsa_header.advertising_router) {
                router_lsas.erase(it);
                delete lsa;
                break;
            }
        }
        pthread_mutex_unlock(&router_lock);
    } else if (type == LSA_NETWORK) {
        pthread_mutex_lock(&network_lock);
        
        pthread_mutex_unlock(&network_lock);
    }
}