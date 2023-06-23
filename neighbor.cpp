#include "common.h"
#include "interface.h"
#include "lsdb.h"
#include "packet_manage.h"


Neighbor::Neighbor(in_addr_t ip, Interface* intf):ip(ip) {
    state = NeighborState::S_DOWN;
    is_master = false;
    dd_seq_num = 0;
    last_dd_seq_num = 0;
    last_dd_len = 0;
    priority = 1;
    host_interface = intf;
}

void Neighbor::initDBSummaryList() {
    pthread_mutex_lock(&lsdb.router_lock);

    for (auto& p_lsa: lsdb.router_lsas) 
        db_summary_list.push_back(p_lsa->lsa_header);
    for (auto& p_lsa: lsdb.network_lsas)
        db_summary_list.push_back(p_lsa->lsa_header);

    pthread_mutex_unlock(&lsdb.router_lock);
}

void Neighbor::eventHelloReceived() {
    printf("Neighbor %d received HelloReceived ", this->id);
    if (state == NeighborState::S_DOWN) {
        state = NeighborState::S_INIT;
        printf("and its state from DOWN -> INIT.\n");
    }
}

void Neighbor::event2WayReceived() {
    printf("Neighbor %d received 2WayReceived ", this->id);
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
    }
}

void Neighbor::event1WayReceived() {
    printf("Neighbor %d received 1WayReceived ", this->id);
    if (state >= NeighborState::S_2WAY) { // above 2WAY
        state = NeighborState::S_INIT;
        printf("and its state from 2-WAY -> INIT.\n");
    }
}

void Neighbor::eventNegotiationDone() {
    printf("Neighbor %d received 1WayReceived ", this->id);
    if (state == NeighborState::S_EXSTART) {
        state = NeighborState::S_EXCHANGE;
        printf("and its state from EXSTART -> EXCHANGE.\n");
        initDBSummaryList();
    }
}

void Neighbor::eventSeqNumberMismatch() {
    printf("Neighbor %d received SeqNumberMismatch ", this->id);
}