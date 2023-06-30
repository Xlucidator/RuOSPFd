#include "common.h"
#include "interface.h"  // contains neighbor.h
#include "lsdb.h"
#include "packet_manage.h"
#include "lsa_manage.h"

Neighbor::Neighbor(in_addr_t ip, Interface* intf):ip(ip) {
    state = NeighborState::S_DOWN;
    is_master = false;
    dd_seq_num = 0;
    last_dd_seq_num = 0;
    last_dd_data_len = 0;
    priority = 1;
    host_interface = intf;
    pthread_mutex_init(&link_state_req_list_lock, NULL);
}

Neighbor::~Neighbor() {
    // remour has it that deque will not release memory (doge)
    link_state_req_list.shrink_to_fit();
    db_summary_list.shrink_to_fit();
}

void Neighbor::initDBSummaryList() {
#ifdef DEBUG
    printf("...start to init db_summary_list...\n");
#endif
    pthread_mutex_lock(&lsdb.router_lock);
    for (auto& p_lsa: lsdb.router_lsas) {
        char* full_rlsa_packet = p_lsa->toRouterLSA();
        db_summary_list.push_back(*((LSAHeader*)full_rlsa_packet));
        delete full_rlsa_packet;
    }
    pthread_mutex_unlock(&lsdb.router_lock);

    pthread_mutex_lock(&lsdb.network_lock);
    for (auto& p_lsa: lsdb.network_lsas) {
        char* full_nlsa_packet = p_lsa->toNetworkLSA();
        db_summary_list.push_back(*((LSAHeader*)full_nlsa_packet));
        delete full_nlsa_packet;
    }
    pthread_mutex_unlock(&lsdb.network_lock);
}

void Neighbor::reqListRemove(uint32_t link_state_id, uint32_t advertise_rtr_id) {
    pthread_mutex_lock(&link_state_req_list_lock);
    for (auto it = link_state_req_list.begin(); it != link_state_req_list.end(); ++it) {
        if (it->link_state_id == link_state_id &&
            it->advertising_router == advertise_rtr_id) {
            #ifdef DEBUG
                printf("link_state_req_list: remove one successfully.\n");
            #endif
            link_state_req_list.erase(it);
            break;
        }
    }
    link_state_req_list.shrink_to_fit();
    pthread_mutex_unlock(&link_state_req_list_lock);
}

void Neighbor::eventHelloReceived() {
    printf("Neighbor %x received HelloReceived ", this->id);
    if (state == NeighborState::S_DOWN) {
        state = NeighborState::S_INIT;
        printf("and its state from DOWN -> INIT.\n");
    } else if (state >= NeighborState::S_INIT) {
        printf("and neighbor updated.\n");
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::event2WayReceived() {
    printf("Neighbor %x received 2WayReceived ", this->id);
    if (state == NeighborState::S_INIT) {      

        Interface* h_intf = this->host_interface;
        NetworkType intf_type = h_intf->type;

        switch (intf_type) {
            case NetworkType::T_BROADCAST: 
            case NetworkType::T_NBMA: {
                if (this->id != this->ndr && this->id != this->nbdr &&
                    myconfigs::router_id != this->ndr && 
                    myconfigs::router_id != this->nbdr) {
                    /* do not forms adjacency */
                    state = NeighborState::S_2WAY;
                    printf("and its state from INIT -> 2-WAY.\n");
                    break;
                }
                // let it fall through 
            }

            default: {
                /* forms adjacency */
                state = NeighborState::S_EXSTART;
                printf("and its state from INIT -> EXSTART.\n");
                /* start to send DD empty packet: prepare master/slave */
                dd_seq_num = 0;
                is_master = true;
                pthread_create(&empty_dd_send_thread, &myconfigs::thread_attr, threadSendEmptyDDPackets, (void*)this);
                break;
            }
        }
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::event1WayReceived() {
    printf("Neighbor %x received 1WayReceived ", this->id);
    if (state >= NeighborState::S_2WAY) { // above 2WAY
        state = NeighborState::S_INIT;
        printf("and its state from 2-WAY -> INIT.\n");
        // TODO: clear link_state_rxmt_map(and interface->rxmtter), db_summary_list, link_state_req_list
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::eventNegotiationDone() {
    printf("Neighbor %x received NegotiationDone ", this->id);
    if (state == NeighborState::S_EXSTART) {
        state = NeighborState::S_EXCHANGE;
        printf("and its state from EXSTART -> EXCHANGE.\n");
        initDBSummaryList();
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::eventSeqNumberMismatch() {
    printf("Neighbor %x received SeqNumberMismatch ", this->id);
    if (state >= NeighborState::S_EXCHANGE) {
        NeighborState prev_state = state;
        // TODO: clear link_state_rxmt_map(and interface->rxmtter), db_summary_list, link_state_req_list
        // init sending empty dd packet (M/I/MS = 1) again
        state = NeighborState::S_EXSTART;
        printf("and its state from XXX -> EXSTART.\n");
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::eventExchangeDone() {
    printf("Neighbor %x received ExchangeDone ", this->id);
    if (state == NeighborState::S_EXCHANGE) {
        if (link_state_req_list.size() == 0) {
            /* has all lsas: no need to request */
            state = NeighborState::S_FULL;
            printf("and its state from EXCHANGE -> FULL.\n");
            onGeneratingRouterLSA();
            if (host_interface->dr == host_interface->ip) {
                onGeneratingNetworkLSA(host_interface);
            }
        } else {
            state = NeighborState::S_LOADING;
            printf("and its state from EXCHANGE -> LOADING.\n");
            pthread_create(&lsr_send_thread, &myconfigs::thread_attr, threadSendLSRPackets, (void*)this);
        }
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::evnetBadLSReq() {
    printf("Neighbor %x received BadLSReq ", this->id);
    // TODO: similar to event SeqNumberMismatch, need to negotiate a new adjacent relationship
    if (state >= NeighborState::S_EXCHANGE) {
        NeighborState prev_state = state;
        // TODO: clear three list/deque/map
        // init sending empty dd packet (M/I/MS = 1) again
        state = NeighborState::S_EXSTART;
        printf("and its state from XXX -> EXSTART.\n");
    } else {
        printf("and reject.\n");
    }
}

void Neighbor::eventLoadDone() {
    printf("Neighbor %x received LoadDone ", this->id);
    if (state == NeighborState::S_LOADING) {
        state = NeighborState::S_FULL;
        printf("and its state from LOADING -> FULL.\n");
        onGeneratingRouterLSA();
    } else {
        printf("and reject.\n");
    }
}