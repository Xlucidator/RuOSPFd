#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// OSPF报文结构
struct ospf_header {
    uint8_t version;
    uint8_t type;
    uint16_t length;
    uint32_t router_id;
    uint32_t area_id;
    uint16_t checksum;
    uint16_t autype;
    uint64_t authentication[2];
};

struct ospf_hello {
    uint32_t network_mask;
    uint16_t hello_interval;
    uint8_t options;
    uint8_t priority;
    uint32_t dead_interval;
    uint32_t designated_router;
    uint32_t backup_designated_router;
};

int main() {
    // 创建原始套接字
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // 设置套接字选项，允许IP_HDRINCL
    int optval = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        return 1;
    }

    // 目标地址
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("192.0.2.1");

    // IP报文
    struct ip ip_header;
    memset(&ip_header, 0, sizeof(ip_header));
    ip_header.ip_hl = 5;
    ip_header.ip_v = 4;
    ip_header.ip_tos = 0;
    ip_header.ip_len = htons(sizeof(ip_header) + sizeof(ospf_header) + sizeof(ospf_hello));
    ip_header.ip_id = 0;
    ip_header.ip_off = 0;
    ip_header.ip_ttl = 1;
    ip_header.ip_p = 89; // IPPROTO_OSPF
    ip_header.ip_sum = 0;
    ip_header.ip_src.s_addr = inet_addr("192.0.2.2");
    ip_header.ip_dst.s_addr = inet_addr("192.0.2.1");

    // OSPF报文
    struct ospf_header ospf_hdr;
    memset(&ospf_hdr, 0, sizeof(ospf_hdr));
    ospf_hdr.version = 2;
    ospf_hdr.type = 1; // Hello报文类型
    ospf_hdr.length = htons(sizeof(ospf_header) + sizeof(ospf_hello));
    ospf_hdr.router_id = htonl(0x01010101);
    ospf_hdr.area_id = 0;
    ospf_hdr.checksum = 0;
    ospf_hdr.autype = 0;
    ospf_hdr.authentication[0] = 0;
    ospf_hdr.authentication[1] = 0;

    // OSPF Hello报文
    struct ospf_hello hello;
    memset(&hello, 0, sizeof(hello));
    hello.network_mask = htonl(0xffffff00);
    hello.hello_interval = htons(10);
    hello.options = 0;
    hello.priority = 1;
    hello.dead_interval = htonl(40);
    hello.designated_router = 0;
    hello.backup_designated_router = 0;

    // 组装报文
    char packet[sizeof(ip_header) + sizeof(ospf_header) + sizeof(ospf_hello)];
    memcpy(packet, &ip_header, sizeof(ip_header));
    memcpy(packet + sizeof(ip_header), &ospf_hdr, sizeof(ospf_header));
    memcpy(packet + sizeof(ip_header) + sizeof(ospf_header), &hello, sizeof(ospf_hello));

    // 发送报文
    if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto");
        return 1;
    }

    close(sockfd);
    return 0;
}