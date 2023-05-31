#include "ospf_packet.h"
#include "setting.h"

#include "common.h"
#include "interface.h"

uint16_t packet_checksum(const void* data, size_t len) {
    uint32_t sum = 0;
    const uint16_t* ptr = static_cast<const uint16_t*>(data);

    // accumulate per 16 bits (2 Bytes)
    for (size_t i = 0; i < len / 2; ++i) {
        sum += *ptr++;
    }
    if (len & 1) {  // if odd len
        sum += static_cast<const uint8_t*>(data)[len - 1];
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

void threadSendPackets() {

}

void* threadSendHelloPackets(void* intf) {
    Interface *interface = (Interface*) intf;
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_OSPF)) < 0) {
        perror("[Thread]SendHelloPacket: socket_fd init");
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, myconfigs::nic_name);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]SendHelloPacket: setsockopt");
    }

    /* Set Address : broadcast */
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = inet_addr("224.0.0.5");

    char* packet = (char*)malloc(65535);
    while (true) {
        // add neighbor
        size_t packet_real_len = sizeof(OSPFHeader) + sizeof(OSPFHello) + 4 * interface->neighbor_list.size();

        /* OSPF Header */
        OSPFHeader* ospf_header = (OSPFHeader*)packet;
        ospf_header->version = 2;
        ospf_header->type = 1; // Hello Type
        ospf_header->packet_length = htons(packet_real_len);
        ospf_header->router_id = htonl(myconfigs::router_id);
        ospf_header->area_id = htonl(myconfigs::area_id);
        ospf_header->autype = 0;
        ospf_header->authentication[0] = 0;
        ospf_header->authentication[1] = 0;

        /* OSPF Hello */
        OSPFHello* ospf_hello = (OSPFHello*)(packet + sizeof(OSPFHeader)); 
        ospf_hello->network_mask = htonl(0xffffff00);
        ospf_hello->hello_interval = htons(10);
        ospf_hello->options = 0x02;
        ospf_hello->rtr_pri = 1;
        ospf_hello->router_dead_interval = htonl(40);
        ospf_hello->designated_router = htonl(interface->dr);
        ospf_hello->backup_designated_router = htonl(interface->bdr);

        /* OSPF Attach */
        uint32_t* ospf_attach = (uint32_t*)(packet + sizeof(OSPFHeader) + sizeof(OSPFHello));
        for (auto& nbr: interface->neighbor_list) {
            *ospf_attach++ = htonl(nbr->id);
        }
        // calculte checksum
        ospf_header->checksum = packet_checksum(ospf_header, packet_real_len);

        /* Send Packet */
        if (sendto(socket_fd, packet, packet_real_len, 0, (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
            perror("[Thread]SendHelloPacket: sendto");
        } 
        #ifdef DEBUG
            else {
                printf("[Thread]SendHelloPacket: send success\n");
            }
        #endif
        sleep(10);
    }
}