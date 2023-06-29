#include "common.h"
#include "interface.h"
#include "lsdb.h"
#include "lsa_manage.h"

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

void* waitTimer(void* intf) {
    Interface* interface = (Interface*)intf;
    sleep(40);  // time not sure
    interface->eventWaitTimer();
}


Interface::~Interface() {
    for (auto& neighbor: neighbor_list) {
        delete neighbor;
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

    candidates.push_back(&self); // add self
    // TODO: do we need a lock?
    for (auto& p_neighbor: neighbor_list) { // add other neighbors
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
    /* 1. Elect BDR */
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

    /* 2. Elect DR */
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
        // TODO: select again
    }

    /* detect changes : Interface became DR now */
    if (dr->ip == this->ip && this->dr != this->ip) {
        onGeneratingNetworkLSA(this);
    }

    this->dr = dr->ip;
    this->bdr = bdr->ip;

#ifdef DEBUG
    printf("\tnew dr: %x\n", this->dr);
    printf("\tnew bdr: %x\n", this->bdr);
    printf("\tfinished\n");
#endif
}


void Interface::eventInterfaceUp() {
    printf("Interface %x received BackUpSeen ", this->ip);
    if (state == InterfaceState::S_DOWN) {
        // TODO: NBMA/BROADCAST: WAITING; P2P/P2MP/VIRTUAL: POINT-2-POINT
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_t timer_thread;
        pthread_t rxmt_thread;
        pthread_create(&timer_thread, &attr, waitTimer, this);
        pthread_create(&rxmt_thread, &attr, Retransmitter::run, (void*)&(this->rxmtter));

        state = InterfaceState::S_WAITING;
        printf("and its state from DOWN -> WAITING.\n");
    } else {
        printf("and reject.\n");
    }
}

void Interface::eventWaitTimer() {
    printf("Interface %x received WaitTimer ", this->ip);
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

        onGeneratingRouterLSA();
    } else {
        printf("and reject.\n");
    }
}

void Interface::eventBackUpSeen() {
    printf("Interface %x received BackUpSeen ", this->ip);
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

        onGeneratingRouterLSA();
    } else {
        printf("and reject.\n");
    }
}

void Interface::eventNeighborChange() {
    printf("Interface %x received NeighborChange ", this->ip);

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

        onGeneratingRouterLSA();
    } else {
        printf("and reject.\n");
    }
}