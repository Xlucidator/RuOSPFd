#include "common.h"
#include "interface.h"

#include <list>
#include <vector>
#include <algorithm>

/* priority first, router id second */
bool compareNeighbor(Neighbor* a, Neighbor* b) {
    if (a->priority != b->priority) {
        return a->priority > b->priority;
    } else {
        return a->id >= b->id;
    }
}

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
    Neighbor* dr  = nullptr;
    Neighbor* bdr = nullptr;
    /* Elect BDR */
    std::vector<Neighbor*> bdr_candidates_lv1; // most fittest
    std::vector<Neighbor*> bdr_candidates_lv2; // second fittest
    for (auto& p_neighbor: candidates) {
        if (p_neighbor->ndr != p_neighbor->id) {
            /* only who didn't declare dr could run for bdr */ 
            bdr_candidates_lv2.push_back(p_neighbor);
            if (p_neighbor->nbdr == p_neighbor->id) {
                /* one should also declare bdr */
                bdr_candidates_lv1.push_back(p_neighbor);
            }
        }
    }
    if (bdr_candidates_lv1.size() != 0) {
        std::sort(bdr_candidates_lv1.begin(), bdr_candidates_lv1.end(), compareNeighbor);
        bdr = bdr_candidates_lv1[0];
    } else if (bdr_candidates_lv2.size() != 0) {
        std::sort(bdr_candidates_lv2.begin(), bdr_candidates_lv2.end(), compareNeighbor);
        bdr = bdr_candidates_lv2[0];
    } // assume bdr_candidates_lv2 isn't empty

    /* Elect DR */
    std::vector<Neighbor*> dr_candidates;
    for (auto& p_neighbor : candidates) {
        if (p_neighbor->ndr == p_neighbor->id) {
            /* one should declare dr */
            dr_candidates.push_back(p_neighbor);
        }
    }
    if (dr_candidates.size() != 0) {
        std::sort(dr_candidates.begin(), dr_candidates.end(), compareNeighbor);
        dr = dr_candidates[0];
    } else {
        dr = bdr;   // ? so what about bdr, it can't be equal to bdr right?
    }

    if ( // (now is dr/bdr)    ^ (former is dr/bdr)
        ((dr->ip  == this->ip) ^ (this->dr  == this->ip)) ||
        ((bdr->ip == this->ip) ^ (this->bdr == this->ip))
        ) {
        // select again
        // TODO
    }

    /* detect changes : Interface became DR now */
    if (dr->ip == this->ip && this->dr != this->ip) {
        // TODO: generate Network LSA
    }

    this->dr = dr->ip;
    this->bdr = bdr->ip;

#ifdef DEBUG
    std::cout << "\tnew dr: "  << this->dr  << std::endl;
    std::cout << "\tnew bdr: " << this->bdr << std::endl;
    printf("\tfinished\n");
#endif
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

        // TODO: generate Router LSA
    }
}

void Interface::eventNeighborChange() {
    printf("Interface %d received NeighborChange ", this->ip);

    if (state == InterfaceState::S_DR ||
        state == InterfaceState::S_BACKUP ||
        state == InterfaceState::S_DROTHER
        ) {
        electDesignedRouter();
        if (ip == dr) {
            state = InterfaceState::S_DR;
            printf("and its state from XX -> DR.\n");
        } else if (ip == bdr) {
            state = InterfaceState::S_BACKUP;
            printf("and its state from XX -> BACKUP.\n");
        } else {
            state = InterfaceState::S_DROTHER;
            printf("and its state from XX -> DROTHER.\n");
        }
    
        // TODO: generate Router LSA
    }
}