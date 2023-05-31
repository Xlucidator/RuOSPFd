#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* socket */ 
#include <sys/socket.h>
#include <netinet/in.h> // ntoh hton ...
#include <netinet/ip.h>

/* nic */
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>  // inet_addr

#define CLEAR_STRUCT(S) memset(&S, 0, sizeof(S))

#endif //COMMON_H