#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // memset()
#include <stdint.h>
#include <iostream>
#include <assert.h>

#include <unistd.h>     // close()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

/* socket */ 
#include <sys/socket.h>
#include <netinet/in.h> // ntoh hton ...
#include <netinet/ip.h>
#include <netinet/if_ether.h> // ETH_P_IP ... ethernet frame

/* kernel */
#include <sys/ioctl.h>  // ioctl()
#include <net/if.h>     // ifr
#include <arpa/inet.h>  // inet_addr inet_ntoa()

/* setting */
#include "setting.h"

#define CLEAR_STRUCT(S) memset(&S, 0, sizeof(S))

#endif //COMMON_H