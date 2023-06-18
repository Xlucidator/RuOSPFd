#include "common.h"
#include "interface.h"

Neighbor::Neighbor(in_addr_t ip):ip(ip) {
    state = NeighborState::S_DOWN;
    host_interface = nullptr;
}

Neighbor::Neighbor(in_addr_t ip, Interface* intf):ip(ip) {
    state = NeighborState::S_DOWN;
    host_interface = intf;
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
        // TODO: send DD Packet
        dd_seq_num = 0;
        is_master = true;

        state = NeighborState::S_INIT;
        printf("and its state from INIT -> EXSTART.\n");
    }
}