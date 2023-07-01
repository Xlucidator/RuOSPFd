#ifndef LSDB_H
#define LSDB_H

#include "ospf_packet.h"
#include "interface.h"

#include <vector>
#include <stdint.h>
#include <pthread.h>

class LSDB { // TODO: LSA Aging
public:
    std::vector<LSARouter*> router_lsas;
    pthread_mutex_t router_lock;

    std::vector<LSANetwork*> network_lsas;
    pthread_mutex_t network_lock;

    uint16_t    max_age;        // max time an lsa can survive, default 3600s
    uint16_t    max_age_diff;   // max time an lsa flood the AS, default 900s

    LSDB();
    ~LSDB();
    LSARouter* getRouterLSA(uint32_t link_state_id, uint32_t advertise_rtr_id);
    LSARouter* getRouterLSA(uint32_t link_state_id);
    LSANetwork* getNetworkLSA(uint32_t link_state_id, uint32_t advertise_rtr_id);
    LSANetwork* getNetworkLSA(uint32_t link_state_id);
    void addLSA(char* net_ptr); // add LSA from netptr from LSU, for all types
    void delLSA(uint32_t link_state_id, uint32_t advertise_rtr_id, uint8_t type);
    void floodLSA(LSA* lsa, std::vector<Interface*>& sel_interfaces); // on lsdb updated
};


extern LSDB lsdb;

#endif // LSDB_H
