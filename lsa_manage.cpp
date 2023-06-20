#include "lsa_manage.h"
#include "ospf_packet.h"
#include "interface.h"

#include <vector>

LSARouter genRouterLSA(std::vector<Interface*>& sel_interfaces) {
    LSARouter router_lsa;

    for (auto& interface: sel_interfaces) {
        if (interface->state == InterfaceState::S_DOWN) {
            continue;
        }
        LSARouterLink link;
    }
}