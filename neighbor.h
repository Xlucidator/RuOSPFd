#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include "common.h"

enum struct NeighborState : uint8_t {
    S_DOWN = 0,
    S_ATTEMPT,
    S_INIT,
    S_2WAY,
    S_EXSTART,
    S_EXCHANGE,
    S_LOADING,
    S_FULL
};

enum struct NeighborEvent : uint8_t {
    E_HELLORECV = 0,
    E_START,
    E_2WAYRECV,
    E_NEGOTIATIONDONE,
    E_EXCHANGEDONE,
    E_BADLSREQ,
    E_LOADINGDONE,
    E_ADJOK,
    E_SEQNUMMISMATCH,
    E_1WAY,
    E_KILLNBR,
    E_INACTTIMER,
    E_LLDOWN
};

class Neighbor {
public:
    uint32_t id;    
    uint32_t ip;
    uint32_t ndr;
    uint32_t nbdr;
    uint8_t state;
};

#endif // NEIGHBOR_H