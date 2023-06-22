#include "lsa_manage.h"
#include "ospf_packet.h"
#include "lsdb.h"
#include "interface.h"
#include "setting.h"

#include <vector>


void onGeneratingRouterLSA() {
    LSARouter* router_lsa = genRouterLSA(myconfigs::interfaces);

    LSARouter* lsa_find = lsdb.getRouterLSA(myconfigs::router_id, myconfigs::router_id);
    // to compare
    if (lsa_find == nullptr || !(*lsa_find == *router_lsa)) {
        /* lsdb do not contain this lsa: add the lsa */
        pthread_mutex_lock(&lsdb.router_lock);
        lsdb.router_lsas.push_back(router_lsa);
        pthread_mutex_unlock(&lsdb.router_lock);
    }
    // TODO: consider -- so why not use <set>
}


LSARouter* genRouterLSA(std::vector<Interface*>& sel_interfaces) {
    LSARouter* router_lsa = new LSARouter();

    router_lsa->lsa_header.ls_type = 1;
    router_lsa->lsa_header.advertising_router = myconfigs::router_id;
    router_lsa->lsa_header.link_state_id = myconfigs::router_id;

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

    network_lsa->lsa_header.ls_type = 2;
    network_lsa->lsa_header.advertising_router = myconfigs::router_id;
    network_lsa->lsa_header.link_state_id = interface->ip;

    for (auto& p_neighbor: interface->neighbor_list) {
        if (p_neighbor->state == NeighborState::S_FULL) {
            network_lsa->attached_routers.push_back(p_neighbor->ip);
        }
    }

    return network_lsa;
}