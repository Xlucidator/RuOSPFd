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

void sendPackets(const char* ospf_data, int data_len, uint8_t type, uint32_t dst_ip, Interface* interface) {
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_OSPF)) < 0) {
        perror("SendPacket: socket_fd init");
    }

    /* Bind sockets to certain Network Interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, myconfigs::nic_name);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("SendPacket: setsockopt");
    }

    /* Set Dst Address : certain ip */
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = htonl(dst_ip);

    char* packet = (char*)malloc(65535);
    size_t packet_len = OSPFHDR_LEN + data_len;
    /* OSPF Header */
    OSPFHeader* ospf_header = (OSPFHeader*)packet;
    ospf_header->version = 2;
    ospf_header->type = type;
    ospf_header->packet_length = htons(packet_len);
    ospf_header->router_id = htonl(myconfigs::router_id);
    ospf_header->area_id   = htonl(interface->area_id);
    ospf_header->autype = 0;
    ospf_header->authentication[0] = 0;
    ospf_header->authentication[1] = 0;

    /* OSPF Data */
    memcpy(packet + OSPFHDR_LEN, ospf_data, data_len);

    // calculte checksum
    ospf_header->checksum = packet_checksum(ospf_header, packet_len);

    /* Send OSPF Packet */
    if (sendto(socket_fd, packet, packet_len, 0, (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
            perror("SendPacket: sendto");
    } 
#ifdef DEBUG
    else {
        printf("SendPacket: send success\n");
    }
#endif

    free(packet);
}

void* threadSendHelloPackets(void* intf) {
    Interface *interface = (Interface*) intf;
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_OSPF)) < 0) {
        perror("[Thread]SendHelloPacket: socket_fd init");
    }

    /* Bind sockets to certain Network Interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, myconfigs::nic_name);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]SendHelloPacket: setsockopt");
    }

    /* Set Address : multicast */
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
        ospf_header->area_id = htonl(interface->area_id);
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
        sleep(interface->hello_intervel);
    }

    free(packet);
}

void* threadSendEmptyDDPackets(void* nbr) {
    Neighbor* neighbor = (Neighbor*)nbr;

    while (true) {
        if (neighbor->state != NeighborState::S_EXSTART) {
            break;
        }

        OSPFDD ospf_dd;
        memset(&ospf_dd, 0, sizeof(ospf_dd));
        ospf_dd.interface_MTU = htons(neighbor->host_interface->mtu);
        ospf_dd.options = 0x02; // 8bit: | * | * | DC | EA | N/P | MC | E | * |  
        ospf_dd.sequence_number = neighbor->dd_seq_num;
        ospf_dd.b_I = ospf_dd.b_M = ospf_dd.b_MS = 1;

        sendPackets((char *)&ospf_dd, sizeof(ospf_dd), T_DD, neighbor->ip, neighbor->host_interface);
    #ifdef DEBUG
        printf("[Thread]SendEmptyDDPacket: send success\n");
    #endif
        sleep(5);   // TODO: assure the time interval
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
            /* == about DR == */
            if (neighbor->ndr  == neighbor->ip &&
                neighbor->nbdr == 0x00000000 &&
                interface->state == InterfaceState::S_WAITING
                ) {
                interface->eventBackUpSeen();
            } else 
            if ((prev_ndr == neighbor->ip) ^ (neighbor->ndr == neighbor->ip)) {
                //   former declare dr   ^  now declare dr
                interface->eventNeighborChange();
            }
            /* == about BDR == */
            if (neighbor->nbdr == neighbor->ip &&
                interface->state == InterfaceState::S_WAITING
                ) {
                interface->eventBackUpSeen(); 
            } else
            if ((prev_nbdr == neighbor->ip) ^ (neighbor->nbdr == neighbor->ip)) {
                //   former declare bdr   ^  now declare bdr
                interface->eventNeighborChange();
            }
        }
        else
        if (ospf_header->type == T_DD) {
        #ifdef DEBUG
            printf("[Thread]RecvPacket: DD packet\n");
        #endif
            OSPFDD* ospf_dd = (OSPFDD*)(packet_rcv + IPHDR_LEN + OSPFHDR_LEN);
            Neighbor* neighbor = interface->getNeighbor(src_ip);
            
            bool is_accepted = false;
            bool is_dup = false;
            uint32_t seq_num = ntohl(ospf_dd->sequence_number);
            if (neighbor->last_dd_seq_num == seq_num) {
                is_dup = true;
            #ifdef DEBUG
                printf("[Thread]RecvPacket: DD packet duplicated\n");
            #endif
            } else {
                neighbor->last_dd_seq_num = seq_num;
            }

            /* Dealing DD depending on neighbor's state */
            dd_switch:
            switch (neighbor->state) {
                case NeighborState::S_INIT: {
                    neighbor->event2WayReceived();
                    goto dd_switch; // depend on the result of the event
                    break;
                }
                case NeighborState::S_EXSTART: {
                    if (ospf_dd->b_M && ospf_dd->b_I && ospf_dd->b_MS
                        && neighbor->id > myconfigs::router_id) {
                        /* confirm neighbor is master */
                        neighbor->is_master = true;
                        neighbor->dd_seq_num = seq_num;
                        seq_num += 1;
                        // TODO? set b_M to 0 in ack DD
                    } else
                    if (!ospf_dd->b_I && !ospf_dd->b_MS
                        && ospf_dd->sequence_number == seq_num
                        && neighbor->id < myconfigs::router_id) {
                        /* confirm neighbor is slave */
                        neighbor->is_master = false;
                    } else {
                        /* ignore the packet */
                        continue; 
                    }
                    neighbor->eventNegotiationDone();
                    break;
                }
                case NeighborState::S_EXCHANGE: {
                    if (is_dup) {
                        if (neighbor->is_master) {
                            // neighbor is master -> host interface is slave
                            sendPackets(neighbor->last_dd_packet, neighbor->last_dd_len, T_DD, neighbor->ip, neighbor->host_interface); 
                        }
                        continue; // ignore the packet
                    }
                    if (ospf_dd->b_I || (ospf_dd->b_MS ^ neighbor->is_master)) {
                        neighbor->eventSeqNumberMismatch(); // TODO
                        continue;
                    }
                    if ((neighbor->is_master && seq_num == neighbor->dd_seq_num + 1) &&
                        (!neighbor->is_master && seq_num == neighbor->dd_seq_num)) {
                        is_accepted = true;
                    } else {
                        neighbor->eventSeqNumberMismatch();
                        continue;
                    }
                    break;
                }
                case NeighborState::S_LOADING:
                case NeighborState::S_FULL: {
                    /* finish dd exchange */
                    // only deal with duplicate or mismatch
                    if (is_dup) {
                        if (neighbor->is_master) {
                            // neighbor is master -> host interface is slave
                            sendPackets(neighbor->last_dd_packet, neighbor->last_dd_len, T_DD, neighbor->ip, neighbor->host_interface); 
                        }
                        continue; // ignore the packet
                    }
                    if (ospf_dd->b_I || (ospf_dd->b_MS ^ neighbor->is_master)) {
                        neighbor->eventSeqNumberMismatch(); // TODO
                        continue;
                    }
                    break;
                }
                default: ;
            }

            if (is_accepted) {
                /* Reply to DD packet received */
                LSAHeader* lsa_header_rcv = (LSAHeader*)(packet_rcv + IPHDR_LEN + OSPF_DD_LEN);
                LSAHeader* lsa_header_end = (LSAHeader*)(packet_rcv + IPHDR_LEN + ospf_header->packet_length);
                while (lsa_header_rcv != lsa_header_end) {
                    LSAHeader lsa_header;
                    lsa_header.advertising_router = ntohl(lsa_header_rcv->advertising_router);
                    lsa_header.link_state_id = ntohl(lsa_header_rcv->link_state_id);
                    lsa_header.ls_sequence_number = ntohl(lsa_header_rcv->ls_sequence_number);
                    lsa_header.ls_type = lsa_header_rcv->ls_type;

                    if (lsa_header.ls_type == LSA_ROUTER) {
                        
                    } else if (lsa_header.ls_type == LSA_NETWORK) {
                        
                    }

                    lsa_header_rcv += 1;
                }

                OSPFDD dd_ack;
                dd_ack.b_MS = ~ospf_dd->b_MS;
                
            }
        }
    }

    free(packet_rcv);
#undef RECV_LEN
}