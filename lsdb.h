#ifndef LSDB_H
#define LSDB_H

#include "ospf_packet.h"

#include <vector>
#include <stdint.h>
#include <pthread.h>

class LSDB {
public:
    std::vector<LSARouter*> router_lsas;
    pthread_mutex_t router_lock;

    std::vector<LSANetwork*> network_lsas;
    pthread_mutex_t network_lock;

    LSDB();
    ~LSDB();
    LSARouter* getRouterLSA(uint32_t link_state_id, uint32_t advertise_rtr_id);
    LSARouter* getRouterLSA(uint32_t link_state_id);
    LSANetwork* getNetworkLSA(uint32_t link_state_id, uint32_t advertise_rtr_id);
    LSANetwork* getNetworkLSA(uint32_t link_state_id);
};


extern LSDB lsdb;

#endif // LSDB_H
