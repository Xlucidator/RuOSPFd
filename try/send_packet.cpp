#include "../common.h"
#include "../ospf_packet.h"

void setDstAddr(sockaddr_in& dst_addr, const char* dst);

int main(int argc, char **argv) {
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket");
        return 1;
    }
    
    /* 允许IP_HDRINCL: 自定义更改IP报文头 */
    int opt_on = 1;
    if (setsockopt(socket_fd, IPPROTO_IP, IP_HDRINCL, &opt_on, sizeof(opt_on)) < 0) {
        perror("setsockopt");
        return 1;
    }

    /* 设置socket目标地址 */
    struct sockaddr_in dst_addr;
    setDstAddr(dst_addr, "10.128.54.200");


    /* 创建报文 */
    ip ip_header;   // IP header
    CLEAR_STRUCT(ip_header);
    ip_header.ip_hl = 5;
    ip_header.ip_v = 4;
    ip_header.ip_tos = 0;
    ip_header.ip_len = htons(sizeof(ip_header) + sizeof(OSPFHeader) + sizeof(OSPFHello));
    ip_header.ip_id = 0;
    ip_header.ip_off = 0;
    ip_header.ip_ttl = 1;
    ip_header.ip_p = IPPROTO_OSPF; // IPPROTO_OSPF
    ip_header.ip_sum = 0;
    ip_header.ip_src.s_addr = inet_addr("192.128.75.128");
    ip_header.ip_dst.s_addr = inet_addr("10.128.54.200");

    OSPFHeader ospf_header; // OSPF header
    CLEAR_STRUCT(ospf_header);
    ospf_header.version = 2;
    ospf_header.type = 1; // Hello报文类型
    ospf_header.packet_length = htons(sizeof(OSPFHeader) + sizeof(OSPFHello));
    ospf_header.router_id = htonl(0x01010101);
    ospf_header.area_id = 0;
    ospf_header.checksum = 0;
    ospf_header.autype = 0;
    ospf_header.authentication[0] = 0;
    ospf_header.authentication[1] = 0;

    OSPFHello ospf_hello;   // OSPF Hello
    CLEAR_STRUCT(ospf_hello);
    ospf_hello.network_mask = htonl(0xffffff00);
    ospf_hello.hello_interval = htons(10);
    ospf_hello.options = 0;
    ospf_hello.rtr_pri = 1;
    ospf_hello.router_dead_interval = htonl(40);
    ospf_hello.designated_router = 0;
    ospf_hello.backup_designated_router = 0;

    // 组装packet
    size_t packet_size = sizeof(ip_header) + sizeof(OSPFHeader) + sizeof(OSPFHello);
    char packet[packet_size];
    memcpy(packet, &ip_header, sizeof(ip_header));
    memcpy(packet + sizeof(ip_header), &ospf_header, sizeof(ospf_header));
    memcpy(packet + sizeof(ip_header) + sizeof(ospf_header), &ospf_hello, sizeof(ospf_hello));

    /* 发送报文 */
    if (sendto(socket_fd, packet, sizeof(packet), 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr)) < 0) {
        perror("sendto");
        return 1;
    }

    close(socket_fd);

    return 0;
}

void setDstAddr(sockaddr_in& dst_addr, const char* dst) {
    CLEAR_STRUCT(dst_addr);
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = inet_addr(dst);
}