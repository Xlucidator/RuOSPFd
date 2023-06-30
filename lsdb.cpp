#include "lsdb.h"
#include "common.h"


LSDB::LSDB() {
    pthread_mutex_init(&router_lock, NULL);
    pthread_mutex_init(&network_lock, NULL);

    max_age = 3600;
    max_age_diff = 900;
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

void LSDB::addLSA(char* net_ptr) {
    LSAHeader* lsa_header = (LSAHeader*) net_ptr;
    
    if (lsa_header->ls_type == LSA_ROUTER) {
        LSARouter* rlsa = new LSARouter(net_ptr);
        
        // checksum
        LSARouter test_rlsa = *rlsa;
        printf("recv checksum: %x\n", rlsa->lsa_header.ls_checksum);
        printf("self checksum:\n");
        test_rlsa.toRouterLSA();

        LSARouter* lsa_check = getRouterLSA(rlsa->lsa_header.link_state_id, rlsa->lsa_header.advertising_router);
        pthread_mutex_lock(&router_lock);
        if (lsa_check != nullptr) { // arbitrary : delete the old in the router_lsa;
            for (auto it = router_lsas.begin(); it != router_lsas.end(); ++it) {
                if (*it == lsa_check) {
                    router_lsas.erase(it);
                    delete lsa_check;
                    break;
                }
            }
        }
        router_lsas.push_back(rlsa);
        pthread_mutex_unlock(&router_lock);
    } else if (lsa_header->ls_type == LSA_NETWORK) {
        LSANetwork* nlsa = new LSANetwork(net_ptr);
        LSANetwork* lsa_check = getNetworkLSA(nlsa->lsa_header.link_state_id, nlsa->lsa_header.advertising_router);
        pthread_mutex_lock(&network_lock);
        if (lsa_check != nullptr) {
            for (auto it = network_lsas.begin(); it != network_lsas.end(); ++it) {
                if (*it == lsa_check) {
                    network_lsas.erase(it);
                    delete lsa_check;
                    break;
                }
            }
        }
        network_lsas.push_back(nlsa);
        pthread_mutex_unlock(&network_lock);
    }
}

// TODO: if router_lsas and network_lsas has a father class, it could be more elegant
void LSDB::delLSA(uint32_t link_state_id, uint32_t advertise_rtr_id, uint8_t type) {
    if (type == LSA_ROUTER) {
        pthread_mutex_lock(&router_lock);
        for (auto it = router_lsas.begin(); it != router_lsas.end(); ++it) {
            LSARouter* lsa = *it;
            if (lsa->lsa_header.link_state_id == link_state_id && 
                lsa->lsa_header.advertising_router == advertise_rtr_id) {
                router_lsas.erase(it);
                delete lsa;
                break;
            }
        }
        pthread_mutex_unlock(&router_lock);
    } else if (type == LSA_NETWORK) {
        pthread_mutex_lock(&network_lock);
        for (auto it = network_lsas.begin(); it != network_lsas.end(); ++it) {
            LSANetwork* lsa = *it;
            if (lsa->lsa_header.link_state_id == link_state_id && 
                lsa->lsa_header.advertising_router == advertise_rtr_id) {
                network_lsas.erase(it);
                delete lsa;
                break;
            }
        }
        pthread_mutex_unlock(&network_lock);
    }
}