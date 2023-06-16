#include "interface.h"


Neighbor* Interface::getNeighbor(in_addr_t ip) {
    for (auto& neighbor: neighbor_list) {
        if (neighbor->ip == ip) {
            return neighbor;
        }
    }
    return nullptr;
}

Neighbor* Interface::addNeighbor(in_addr_t ip) {
    Neighbor new_neighbor(ip, this);
    // new_neighbor.host_interface = this;
    neighbor_list.push_back(&new_neighbor);
    return &new_neighbor;
}