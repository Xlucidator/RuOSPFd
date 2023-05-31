#ifndef SETTINGS_H
#define SETTINGS_H

#include <arpa/inet.h> // contains <netinet/in.h> 

#define DEBUG 1

namespace myconfigs 
{
    
    const char* nic_name = "ens33";
    uint32_t router_id = ntohl(inet_addr("192.168.75.128"));
    uint32_t area_id = 0;

} // namespace Configs

#endif // SETTINGS_H
