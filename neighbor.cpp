#include "neighbor.h"

#include "common.h"

void Neighbor::helloBeenReceived() {
    printf("Neighbor %d received HelloReceived ", this->id);
    if (state == NeighborState::S_DOWN) {
        state = NeighborState::S_INIT;
        printf("and its state from DOWN -> INIT.\n");
    }
}