#include "common.h"
#include "interface.h"

#include <list>

Neighbor* Interface::getNeighbor(in_addr_t ip) {
    for (auto& neighbor: neighbor_list) {
        if (neighbor->ip == ip) {
            return neighbor;
        }
    }
    return nullptr;
}

Neighbor* Interface::addNeighbor(in_addr_t ip) {
    Neighbor* new_neighbor = new Neighbor(ip, this);
    neighbor_list.push_back(new_neighbor);
    return new_neighbor;
}

void Interface::eventBackUpSeen() {
    printf("Interface %d received BackUpSeen ", this->ip);
    if (state == InterfaceState::S_WAITING) {
        electDesignedRouter();
        if (ip == dr) {
            state = InterfaceState::S_DR;
            printf("and its state from WAITING -> DR.\n");
        } else if (ip == bdr) {
            state = InterfaceState::S_BACKUP;
            printf("and its state from WAITING -> BACKUP.\n");
        } else {
            state = InterfaceState::S_DROTHER;
            printf("and its state from WAITING -> DROTHER.\n");
        }
    }
}

void Interface::electDesignedRouter() {
#ifdef DEBUG
    printf("\n\tstart electing DR/BDR...\n");
#endif

    std::list<Neighbor*> candidates;

    /* Select Candidates */
    Neighbor self(this->ip, this);
    self.id   = myconfigs::router_id;
    self.ndr  = this->dr;
    self.nbdr = this->bdr;
    self.priority = this->router_priority;

    candidates.push_back(&self);
    for (auto& p_neighbor: neighbor_list) {
        if (static_cast<uint8_t>(p_neighbor->state) >= 
            static_cast<uint8_t>(NeighborState::S_2WAY) && 
            p_neighbor->priority != 0) {
            /* select to be candidates */
            candidates.push_back(p_neighbor);
        }
    }

    /* Launch Election */


#ifdef DEBUG
    printf("\tfinished\n");
#endif
}