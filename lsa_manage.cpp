#include "lsa_manage.h"
#include "ospf_packet.h"
#include "interface.h"
#include "setting.h"

#include <vector>

LSARouter genRouterLSA(std::vector<Interface*>& sel_interfaces) {
    LSARouter router_lsa;

    router_lsa.lsa_header.ls_type = 1;
    router_lsa.lsa_header.advertising_router = myconfigs::router_id;
    router_lsa.lsa_header.link_state_id = myconfigs::router_id;

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
        router_lsa.links.push_back(link);
    }
    router_lsa.link_num = router_lsa.links.size();
    
    return router_lsa;
}