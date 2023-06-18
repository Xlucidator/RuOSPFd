#include "ospf_packet.h"

#include "interface.h"
#include "common.h"

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
        size_t packet_real_len = OSPFHDR_LEN + sizeof(OSPFHello) + 4 * interface->neighbor_list.size();

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
        OSPFHello* ospf_hello = (OSPFHello*)(packet + OSPFHDR_LEN); 
        ospf_hello->network_mask = htonl(0xffffff00);
        ospf_hello->hello_interval = htons(10);
        ospf_hello->options = 0x02;
        ospf_hello->rtr_pri = 1;
        ospf_hello->router_dead_interval = htonl(40);
        ospf_hello->designated_router = htonl(interface->dr);
        ospf_hello->backup_designated_router = htonl(interface->bdr);

        /* OSPF Attach */
        uint32_t* ospf_attach = (uint32_t*)(packet + OSPF_HELLO_LEN);
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

void* threadRecvPacket(void *intf) {
    Interface *interface = (Interface*) intf;
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_OSPF)) < 0) {
        perror("[Thread]RecvPacket: socket_fd init");
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, myconfigs::nic_name);

#define RECV_LEN 1024
    char* packet_rcv = (char*)malloc(RECV_LEN);
    while (true) {
        memset(packet_rcv, 0, RECV_LEN);
        recv(socket_fd, packet_rcv, RECV_LEN, 0);
        in_addr_t src_ip = ntohl(*(uint32_t*)(packet_rcv + IPHDR_SRCIP));
        if (src_ip == interface->ip) {
            continue; // from self, finish packet process
        }

        OSPFHeader* ospf_header = (OSPFHeader*)(packet_rcv + IPHDR_LEN);
        /* translate : net to host */
        ospf_header->packet_length = ntohs(ospf_header->packet_length);
        ospf_header->router_id     = ntohl(ospf_header->router_id    );
        ospf_header->area_id       = ntohl(ospf_header->area_id      );
        ospf_header->checksum      = ntohl(ospf_header->checksum     );

        if (ospf_header->type == T_HELLO) {
        #ifdef DEBUG
            printf("[Thread]RecvPacket: Hello packet\n");
        #endif
            OSPFHello* ospf_hello = (OSPFHello*)(packet_rcv + IPHDR_LEN + OSPFHDR_LEN);
            Neighbor* neighbor;

            if ((neighbor = interface->getNeighbor(src_ip)) == nullptr) {
                neighbor = interface->addNeighbor(src_ip);
            }
            neighbor->id   = ospf_header->router_id;
            uint32_t prev_ndr   = neighbor->ndr;
            uint32_t prev_nbdr  = neighbor->nbdr;
            neighbor->ndr  = ntohl(ospf_hello->designated_router);
            neighbor->nbdr = ntohl(ospf_hello->backup_designated_router);
            neighbor->priority = ntohl(ospf_hello->rtr_pri);

            neighbor->eventHelloReceived();

            bool to_2way = false;
            /* Decide 1way or 2way: check whether Hello attached-info has self-routerid */ 
            uint32_t* ospf_attach = (uint32_t*)(packet_rcv + IPHDR_LEN + OSPF_HELLO_LEN);
            uint32_t* ospf_end = (uint32_t*)(packet_rcv + IPHDR_LEN + ospf_header->packet_length);
            for (;ospf_attach != ospf_end; ++ospf_attach) {
                if (*ospf_attach == htonl(myconfigs::router_id)) {
                    to_2way = true;
                    neighbor->event2WayReceived();
                    break;
                }
            }
            if (!to_2way) {
                neighbor->event1WayReceived();
                continue; // finish packet process
            }

            /* Decide NeighborChange / BackupSeen Event */
            if (neighbor->ip == neighbor->ndr &&
                neighbor->nbdr == 0x00000000 &&
                interface->state == InterfaceState::S_WAITING) {
                interface->eventBackUpSeen();
            }
        }

        if (ospf_header->type == T_DD) {
        #ifdef DEBUG
            printf("[Thread]RecvPacket: DD packet\n");
        #endif
            OSPFDD* ospf_dd = (OSPFDD*)(packet_rcv + IPHDR_LEN + OSPFHDR_LEN);

        }
    }
#undef RECV_LEN
}