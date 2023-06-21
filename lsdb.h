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

    std::vector<LSANetwork> network_lsas;
    pthread_mutex_t network_lock;

    LSDB();
};

#endif // LSDB_H
