#include "lsa_manage.h"

#include "lsdb.h"
#include "interface.h"
#include "setting.h"

#include <vector>


// TODO: should leave lsdb.xxx_lock in the LSDB class itself!
void onGeneratingRouterLSA() {
    #ifdef DEBUG
        printf("...generating a router lsa...\n");
    #endif
    LSARouter* router_lsa = genRouterLSA(myconfigs::interfaces);

    LSARouter* lsa_find = lsdb.getRouterLSA(myconfigs::router_id, myconfigs::router_id);
    bool is_added = false;
    // assure its comparible
    if (lsa_find == nullptr) {
        /* lsdb do not contain this lsa: add the lsa */
        #ifdef DEBUG
            printf("it is a new router lsa, insert to lsdb\n");
        #endif
        pthread_mutex_lock(&lsdb.router_lock);
        lsdb.router_lsas.push_back(router_lsa);
        pthread_mutex_unlock(&lsdb.router_lock);
        // is_added = true;
    } else if (*router_lsa > *lsa_find) {
        #ifdef DEBUG
            printf("it is a newer router lsa, insert to lsdb, remove old one\n");
        #endif
        lsdb.delLSA(lsa_find->lsa_header.link_state_id, lsa_find->lsa_header.advertising_router, LSA_ROUTER);
        pthread_mutex_lock(&lsdb.router_lock);
        lsdb.router_lsas.push_back(router_lsa);
        pthread_mutex_unlock(&lsdb.router_lock);
        is_added = true;
    }
    // consider -- so why not use <set>
    if (is_added) {
    #ifdef DEBUG
        printf("...try to flood lsa\n");
    #endif
        lsdb.floodLSA(router_lsa, myconfigs::interfaces);
    }
}

void onGeneratingNetworkLSA(Interface* interface) {
    #ifdef DEBUG
        printf("...generating a network lsa...\n");
    #endif
    LSANetwork* network_lsa = genNetworkLSA(interface);

    LSANetwork* lsa_find = lsdb.getNetworkLSA(myconfigs::router_id, interface->ip);
    bool is_added = false;
    if (lsa_find == nullptr || !(*lsa_find == *network_lsa)) {
        /* lsdb do not contain this lsa: add the lsa */
        #ifdef DEBUG
            printf("it is a new network lsa, insert to lsdb\n");
        #endif
        pthread_mutex_lock(&lsdb.network_lock);
        lsdb.network_lsas.push_back(network_lsa);
        pthread_mutex_unlock(&lsdb.network_lock);
    } else if (*network_lsa > *lsa_find) {
        #ifdef DEBUG
            printf("it is a newer network lsa, insert to lsdb, remove old one\n");
        #endif
        lsdb.delLSA(lsa_find->lsa_header.link_state_id, lsa_find->lsa_header.advertising_router, LSA_NETWORK);
        pthread_mutex_lock(&lsdb.network_lock);
        lsdb.network_lsas.push_back(network_lsa);
        pthread_mutex_unlock(&lsdb.network_lock);
        is_added = true;
    }

    if (is_added) {
    #ifdef DEBUG
        printf("...try to flood lsa\n");
    #endif
        lsdb.floodLSA(network_lsa, myconfigs::interfaces);
    }
}

LSARouter* genRouterLSA(std::vector<Interface*>& sel_interfaces) {
    LSARouter* router_lsa = new LSARouter();

    router_lsa->lsa_header.ls_type = 1; // unnecessary now
    router_lsa->lsa_header.advertising_router = myconfigs::router_id;
    router_lsa->lsa_header.link_state_id = myconfigs::router_id;
    pthread_mutex_lock(&lsa_seq_lock);
        router_lsa->lsa_header.ls_sequence_number += lsa_seq_cnt;
        lsa_seq_cnt += 1;
    pthread_mutex_unlock(&lsa_seq_lock);

    for (auto& interface: sel_interfaces) {
        if (interface->state == InterfaceState::S_DOWN) {
            continue;   // pass down interface
        }

        LSARouterLink link;
        link.metric = interface->cost;
        link.tos_num = 0;
        if ((interface->state != InterfaceState::S_WAITING) &&
            (interface->dr == interface->ip || interface->getNeighbor(interface->dr)->state == NeighborState::S_FULL)
            ) {
            link.type = L_TRANSIT;
            link.link_id = interface->dr;
            link.link_data = interface->ip;
        } else {
            link.type = L_STUB;
            link.link_id = interface->ip & interface->mask;
            link.link_data = interface->mask;
        }
        router_lsa->links.push_back(link);
    }
    router_lsa->link_num = router_lsa->links.size();
    
    return router_lsa;
}

LSANetwork* genNetworkLSA(Interface *interface) {
    LSANetwork* network_lsa = new LSANetwork();

    network_lsa->lsa_header.ls_type = 2; // unnecessary now
    network_lsa->lsa_header.advertising_router = myconfigs::router_id;
    network_lsa->lsa_header.link_state_id = interface->ip;
    pthread_mutex_lock(&lsa_seq_lock);
        network_lsa->lsa_header.ls_sequence_number += lsa_seq_cnt;
        lsa_seq_cnt += 1;
    pthread_mutex_unlock(&lsa_seq_lock);

    for (auto& p_neighbor: interface->neighbor_list) {
        if (p_neighbor->state == NeighborState::S_FULL) {
            network_lsa->attached_routers.push_back(p_neighbor->ip);
        }
    }

    return network_lsa;
}