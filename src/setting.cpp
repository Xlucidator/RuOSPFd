#include "setting.h"
#include <arpa/inet.h> // contains <netinet/in.h>

namespace myconfigs {
    
    const char* nic_name = "ens33";
    uint32_t router_id = ntohl(inet_addr("192.168.75.128"));
    std::vector<Interface*> interfaces;
    std::map<uint32_t, Interface*> ip2interface;

    pthread_attr_t thread_attr;
} // namespace Configs

struct in_addr ipaddr_tmp;
int lsa_seq_cnt = 0;
pthread_mutex_t lsa_seq_lock;

bool to_exit = false;