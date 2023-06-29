#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <netinet/ip.h> //结构体iphdr的定义
#include <netinet/if_ether.h> //以太网帧以及协议ID的定义
#include <arpa/inet.h> //提供IP地址转换函数inet_ntoa()
#include <net/if.h>

using namespace std;

int main(int argc, char **argv) {
    int sock_fd, recv_size;
    char buffer[2048];
    struct ethhdr *eth; //以太网帧结构体指针
    struct iphdr *iph; //IPv4数据包结构体指针
    struct in_addr addr1, addr2;
    int count = 1; //总接收次数

    //创建接收IP报文的socket连接
    if (0 > (sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP)))) {
        perror("socket");
        exit(1);
    }

    /* Bind sockets to certain Network Interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "ens33");
    if (setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]RecvPacket: setsockopt - bind to device");
    }

    //循环接收
    while (1) {
        recv_size = recv(sock_fd, buffer, 2048, 0);
        
        // eth = (struct ethhdr*)buffer; //将接收的数据转为以太网帧的格式
        // printf("Dest MAC addr : %02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        // printf("Source MAC addr : %02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);

        iph = (struct iphdr*)(buffer + sizeof(struct ethhdr)); //将接收的数据转为IPv4数据包的格式
        memcpy(&addr1, &iph->saddr, 4);  //复制源IP地址
        memcpy(&addr2, &iph->daddr, 4);  //复制目的IP地址

        if (iph->protocol != 89) {
            continue;
        }

        if (strcmp(inet_ntoa(addr1), "127.0.0.1") == 0) {
            continue;
        }

        cout << "---------------- count = " << count++ << " ----------------" << endl;
        cout << recv_size << " bytes read" << endl;

        if (iph->version == 4 && iph->ihl == 5) {
            cout << "Source host : " << inet_ntoa(addr1) << endl;
            cout << "Dest host : " << inet_ntoa(addr2) << endl;
        }
    }
}