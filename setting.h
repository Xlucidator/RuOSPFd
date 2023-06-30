#ifndef SETTINGS_H
#define SETTINGS_H

#define DEBUG 1

#include <stdint.h>
#include <vector>
#include <pthread.h>
#include "interface.h"

namespace myconfigs {
    extern const char* nic_name;
    extern uint32_t router_id;
    extern std::vector<Interface*> interfaces;
    extern pthread_attr_t thread_attr;
} // namespace Configs

extern struct in_addr ipaddr_tmp;

extern int lsa_seq_cnt;
extern pthread_mutex_t lsa_seq_lock;

#endif // SETTINGS_H
