#include "setting.h"
#include <arpa/inet.h> // contains <netinet/in.h>

namespace myconfigs {
    
    const char* nic_name = "ens33";
    uint32_t router_id = ntohl(inet_addr("192.168.75.128"));
    std::vector<Interface*> interfaces;

    pthread_attr_t thread_attr;

} // namespace Configs