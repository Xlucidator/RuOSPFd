
#include "common.h"

int main() {
    struct sockaddr_in *sin;
    struct ifreq ifr;

    FILE *dns;
    FILE *gw;
    char *ip = new char(16);
    char *netmask = new char(16);
    char *broadcast = new char(16);
    char *mac = new  char(32);

    char *iface = "ens33";

    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket ");
        exit(1);
    }

    /* 获取MAC地址 */
    memset(&ifr, 0, sizeof(ifr)); // 初始化ifr
    strcpy(ifr.ifr_name, iface);
    ioctl(socket_fd, SIOCGIFHWADDR, &ifr);
    printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (unsigned char)ifr.ifr_hwaddr.sa_data[0],
           (unsigned char)ifr.ifr_hwaddr.sa_data[1],
           (unsigned char)ifr.ifr_hwaddr.sa_data[2],
           (unsigned char)ifr.ifr_hwaddr.sa_data[3], 
           (unsigned char)ifr.ifr_hwaddr.sa_data[4],
           (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

    /* 获取ip地址 */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, iface);
    ioctl(socket_fd, SIOCGIFADDR, &ifr);
    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    printf("IP Address: %s\n", inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN));

    return 0;
}