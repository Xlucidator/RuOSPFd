#include "ospf_packet.h"
#include "lsdb.h"
#include "interface.h"
#include "common.h"

uint16_t crc_checksum(const void* data, size_t len) {
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
    #ifdef DEBUG
        printf("...try to use [sendPackets]: data_len - %d, type - %d...\n", data_len, type);
    #endif
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

    char* packet = (char*)malloc(1500);
    size_t packet_len = OSPFHDR_LEN + data_len;
    /* OSPF Header */
    OSPFHeader* ospf_header = (OSPFHeader*)packet;
    ospf_header->version = 2;
    ospf_header->type = type;
    ospf_header->packet_length = htons(packet_len);
    ospf_header->router_id = htonl(myconfigs::router_id);
    ospf_header->area_id   = htonl(interface->area_id);
    ospf_header->checksum  = 0;
    ospf_header->autype = 0;
    ospf_header->authentication[0] = 0;
    ospf_header->authentication[1] = 0;

    /* OSPF Data */
    memcpy(packet + OSPFHDR_LEN, ospf_data, data_len);

    // calculte checksum
    ospf_header->checksum = crc_checksum(ospf_header, packet_len);

    /* Send OSPF Packet */
    if (sendto(socket_fd, packet, packet_len, 0, (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
            perror("SendPacket: sendto");
    } 
#ifdef DEBUG
    else {
        printf("SendPacket: type %d send success, len %d\n", type, packet_len);
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
        ospf_header->checksum = 0;
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
        ospf_header->checksum = crc_checksum(ospf_header, packet_real_len);

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
        sleep(neighbor->host_interface->rxmt_interval); // normally 5
    }
    
}

// TODO: in fact, we should add it to retransmitter and clear it in receiving LSAcks
void* threadSendLSRPackets(void* nbr) {
    Neighbor* neighbor = (Neighbor*)nbr;
    
    char* lsr_packet = (char*)malloc(1024);
    int cnt = 0;
    // TODO: check number of link_state_req_list
    OSPFLSR* lsr_data = (OSPFLSR*) lsr_packet;
    for (auto& req_lsa_header: neighbor->link_state_req_list) {
        lsr_data->state_id = htonl(req_lsa_header.link_state_id);
        lsr_data->adverising_router = htonl(req_lsa_header.advertising_router);
        lsr_data->type = htonl(req_lsa_header.ls_type);

        lsr_data += 1;
        cnt += 1;
    }
    sendPackets(lsr_packet, sizeof(OSPFLSR)*cnt, T_LSR, neighbor->ip, neighbor->host_interface);
#ifdef DEBUG
    printf("[Thread]SendLSRPacket: send success\n");
#endif
    neighbor->link_state_req_list.clear();
    neighbor->link_state_req_list.shrink_to_fit();
}

void* threadRecvPackets(void *intf) {
    Interface *interface = (Interface*) intf;
    int socket_fd;
    if ((socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("[Thread]RecvPacket: socket_fd init");
    }

    /* Bind sockets to certain Network Interface : seems useless */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, myconfigs::nic_name);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]RecvPacket: setsockopt - bind to device");
    }

#ifdef DEBUG
    printf("[Thread]RecvPacket init\n");
#endif

    struct iphdr *ip_header;
    struct in_addr src, dst;
#define RECV_LEN 1514
    char* frame_rcv = (char*)malloc(RECV_LEN);
    char* packet_rcv = frame_rcv + sizeof(struct ethhdr);
    while (true) {
        memset(frame_rcv, 0, RECV_LEN);
        int recv_size = recv(socket_fd, frame_rcv, RECV_LEN, 0);
        
        /* check IP Header : filter  */
        ip_header = (struct iphdr*)packet_rcv;
        // 1. not OSPF packet
        if (ip_header->protocol != 89) {
            continue;
        }
        // 2. src_ip or dst_ip don't fit
        in_addr_t src_ip = ntohl(*(uint32_t*)(packet_rcv + IPHDR_SRCIP));
        in_addr_t dst_ip = ntohl(*(uint32_t*)(packet_rcv + IPHDR_DSTIP));
        if ((dst_ip != interface->ip && dst_ip != ntohl(inet_addr("224.0.0.5"))) ||
            src_ip == interface->ip) {
            continue;
        }

        #ifdef DEBUG
            printf("[Thread]RecvPacket: recv one");
            src.s_addr = src_ip;
            dst.s_addr = dst_ip;
            printf(" src:%s,", inet_ntoa(src)); // inet_ntoa() -> char* is static
            printf(" dst:%s\n", inet_ntoa(dst));
        #endif

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
                        // receive dd: 1 1 1 bigger_id
                        /* confirm neighbor is master */
                        #ifdef DEBUG
                            printf("neighbor %x declare master, ", neighbor->id);
                            printf("interface %x agree => ", interface->ip);
                            printf("neighbor is master\n");
                        #endif
                        neighbor->is_master = true;
                        neighbor->dd_seq_num = seq_num;
                        seq_num += 1;
                    } else
                    if (!ospf_dd->b_I && !ospf_dd->b_MS
                        && ospf_dd->sequence_number == seq_num
                        && neighbor->id < myconfigs::router_id) {
                        // receive dd: x 0 0 smaller_id
                        /* confirm neighbor is slave */
                        #ifdef DEBUG
                            printf("neighbor %x agree slave, ", neighbor->id);
                            printf("interface %x confirm\n => ", interface->ip);
                            printf("self is master\n");
                        #endif
                        neighbor->is_master = false;
                    } else {
                        /* ignore the packet */
                        #ifdef DEBUG
                            printf("don't agree, ignore\n");
                        #endif
                        continue; 
                    }
                    neighbor->eventNegotiationDone();
                    // received dd may contain lsa_header now, need parse again in S_EXCHANGE
                    goto dd_switch; 
                    break;
                }
                case NeighborState::S_EXCHANGE: {
                    if (is_dup) {
                        #ifdef DEBUG
                            printf("duplicated dd packet: ignore\n");
                        #endif
                        if (neighbor->is_master) {
                            // neighbor is master -> host interface is slave
                            sendPackets(neighbor->last_dd_data, neighbor->last_dd_data_len, T_DD, neighbor->ip, neighbor->host_interface); 
                        }
                        continue; // ignore the packet
                    }
                    if (ospf_dd->b_I || (ospf_dd->b_MS ^ neighbor->is_master)) {
                        // negotiation is done, b_I should not be 1 ; b_MS should fit negotiation result
                        #ifdef DEBUG
                            printf("mismatch: b_I should not be 1 or b_MS should fit negotiation result\n");
                        #endif
                        neighbor->eventSeqNumberMismatch(); // TODO
                        continue;
                    }
                    if ((neighbor->is_master && seq_num == neighbor->dd_seq_num + 1) ||
                        (!neighbor->is_master && seq_num == neighbor->dd_seq_num)) {
                        // dd packet's seq_num is legal: so we can recv it and analysis (it may has lsa_header)
                        #ifdef DEBUG
                            printf("accepted dd packet, analysis the possible lsa_header\n");
                        #endif
                        is_accepted = true;
                    } else {
                        #ifdef DEBUG
                            printf("mismatch: dd packet's seq_num is illegal\n");
                        #endif
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
                            sendPackets(neighbor->last_dd_data, neighbor->last_dd_data_len, T_DD, neighbor->ip, neighbor->host_interface); 
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
                /* 1. Receive DD packet and update link_state_req_list */
                LSAHeader* lsa_header_rcv = (LSAHeader*)(packet_rcv + IPHDR_LEN + OSPF_DD_LEN);
                LSAHeader* lsa_header_end = (LSAHeader*)(packet_rcv + IPHDR_LEN + ospf_header->packet_length);
                while (lsa_header_rcv != lsa_header_end) {
                    LSAHeader lsa_header;
                    lsa_header.advertising_router = ntohl(lsa_header_rcv->advertising_router);
                    lsa_header.link_state_id = ntohl(lsa_header_rcv->link_state_id);
                    lsa_header.ls_sequence_number = ntohl(lsa_header_rcv->ls_sequence_number);
                    lsa_header.ls_type = lsa_header_rcv->ls_type;
                    /* add to link_state_req_list if lsdb do not have the lsa */
                    if (lsa_header.ls_type == LSA_ROUTER) {
                        if (lsdb.getRouterLSA(lsa_header.link_state_id, lsa_header.advertising_router) == nullptr) {
                            neighbor->link_state_req_list.push_back(lsa_header);
                        }
                    } else if (lsa_header.ls_type == LSA_NETWORK) {
                        if (lsdb.getNetworkLSA(lsa_header.link_state_id, lsa_header.advertising_router) == nullptr) {
                            neighbor->link_state_req_list.push_back(lsa_header);
                        }
                    }

                    lsa_header_rcv += 1;
                }

                /* 2. Reply to the DD packet received */
                char* data_ack = (char*)malloc(1024);
                size_t data_len = 0;

                OSPFDD* dd_ack = (OSPFDD*)data_ack;
                memset(dd_ack, 0, sizeof(OSPFDD));
                data_len += sizeof(OSPFDD);
                dd_ack->options = 0x02;
                dd_ack->interface_MTU = htons(neighbor->host_interface->mtu);
                dd_ack->b_MS = neighbor->is_master ? 0 : 1;
                // operation is different between master and slave
                if (neighbor->is_master) {
                    /* interface is slave */
                    neighbor->dd_seq_num = seq_num;
                    dd_ack->sequence_number = htonl(seq_num);
                    // add lsa_header
                    LSAHeader* write_lsa_header = (LSAHeader*)(data_ack + sizeof(OSPFDD));
                    int lsa_cnt = 0;
                    while (neighbor->db_summary_list.size() > 0) {
                        if (lsa_cnt >= 10) break;   // simply limit to 10 lsa (in fact may it depend on MTU, up to 100)
                        
                        LSAHeader& lsa_h = neighbor->db_summary_list.front();
                        lsa_h.host2net();
                        memcpy(write_lsa_header, &lsa_h, LSAHDR_LEN);
                        
                        write_lsa_header += 1;
                        lsa_cnt += 1;
                        data_len += LSAHDR_LEN;
                        neighbor->db_summary_list.pop_front();
                    }
                    dd_ack->b_M = (neighbor->db_summary_list.size() == 0) ? 0 : 1;
                    // send ack packet
                    sendPackets(data_ack, data_len, T_DD, neighbor->ip, neighbor->host_interface);
                    memcpy(neighbor->last_dd_data, data_ack, data_len);
                    neighbor->last_dd_data_len = data_len;
                    // evoke event if there's no more dd packet
                    if (dd_ack->b_M == 0 && ospf_dd->b_M == 0) {
                        neighbor->eventExchangeDone();
                    }
                    free(data_ack);
                } else {
                    /* interface is master */
                    printf("[i'm here]\n");
                    // receive ack of last dd, stop rxmt of this packet
                    if (neighbor->link_state_rxmt_map.count(neighbor->dd_seq_num) > 0) {
                        // check for safety, although there's no need to check here
                        interface->rxmtter.delPacketData(neighbor->link_state_rxmt_map[neighbor->dd_seq_num]);
                    }
                    neighbor->dd_seq_num += 1;

                    // evoke event if there's no more dd packet
                    // [Attention]: Master would always evoke this event after Slave  
                    if (neighbor->db_summary_list.size() == 0 && ospf_dd->b_M == 0) {
                        neighbor->eventExchangeDone();
                        free(data_ack);
                        continue;
                    }
                    // add lsa_header
                    LSAHeader* write_lsa_header = (LSAHeader*)(data_ack + sizeof(OSPFDD));
                    int lsa_cnt = 0;
                    while (neighbor->db_summary_list.size() > 0) {
                        if (lsa_cnt >= 10) break;   // simply limit to 10 lsa (in fact may it depend on MTU, up to 100)

                        LSAHeader& lsa_h = neighbor->db_summary_list.front();
                        lsa_h.host2net();
                        memcpy(write_lsa_header, &lsa_h, LSAHDR_LEN);

                        write_lsa_header += 1;
                        lsa_cnt += 1;
                        data_len += LSAHDR_LEN;
                        neighbor->db_summary_list.pop_front();
                    }
                    dd_ack->b_M = (neighbor->db_summary_list.size() == 0) ? 0 : 1;
                    // send ack packet
                    printf("[i'm here]\n");
                    sendPackets(data_ack, data_len, T_DD, neighbor->ip, neighbor->host_interface);
                    uint32_t pdata_id = interface->rxmtter.addPacketData(
                        PacketData(data_ack, data_len, T_DD, neighbor->ip, interface->rxmt_interval)
                    );
                    neighbor->link_state_rxmt_map[neighbor->dd_seq_num] = pdata_id;
                }
                
            }
        }
        else
        if (ospf_header->type == T_LSR) {
        #ifdef DEBUG
            printf("[Thread]RecvPacket: LSR packet\n");
        #endif
            OSPFLSR* lsr_rcv = (OSPFLSR*)(packet_rcv + IPHDR_LEN + OSPFHDR_LEN);
            OSPFLSR* lsr_end = (OSPFLSR*)(packet_rcv + IPHDR_LEN + ospf_header->packet_length);
            Neighbor* neighbor = interface->getNeighbor(src_ip);

            char* lsu_data_ack = (char*)malloc(2048); /* lsu_data_ack = lsu_body + lsu_attached */
            // OSPFLSU body ： size
            OSPFLSU* lsu_body = (OSPFLSU*)lsu_data_ack;
            memset(lsu_body, 0, sizeof(OSPFLSU));
            lsu_body->num = 0;   // redundent, just for consistency
            // attached : lsa
            char* lsu_attached = lsu_data_ack + sizeof(OSPFLSU);
            int lsa_cnt = 0;
            while (lsr_rcv != lsr_end) {
                lsr_rcv->net2host();

                if (lsr_rcv->type == LSA_ROUTER) {
                    LSARouter* router_lsa = lsdb.getRouterLSA(lsr_rcv->state_id, lsr_rcv->adverising_router);
                    if (router_lsa == nullptr) {
                        neighbor->evnetBadLSReq();
                        free(lsu_data_ack);
                        goto after_dealing; // TODO: just an expedient
                    } else {
                        char* lsa_packet = router_lsa->toRouterLSA();
                        memcpy(lsu_attached, lsa_packet, router_lsa->size());
                        delete[] lsa_packet;   // release "new char[size()]" from toRouterLSA()
                        lsu_attached += router_lsa->size();
                    }
                } else if (lsr_rcv->type == LSA_NETWORK) {
                    LSANetwork* network_lsa = lsdb.getNetworkLSA(lsr_rcv->state_id, lsr_rcv->adverising_router);
                    if (network_lsa == nullptr) {
                        neighbor->evnetBadLSReq();
                        free(lsu_data_ack);
                        goto after_dealing; // TODO: just an expedient
                    } else {
                        char* lsa_packet = network_lsa->toNetworkLSA();
                        memcpy(lsu_attached, lsa_packet, network_lsa->size());
                        delete[] lsa_packet;
                        lsu_attached += network_lsa->size();
                    }
                }

                lsr_rcv += 1;
                lsa_cnt += 1;
            }
            lsu_body->num = htonl(lsa_cnt);
            sendPackets(lsu_data_ack,(lsu_attached - lsu_data_ack), T_LSU, src_ip, interface);
        }
        else
        if (ospf_header->type == T_LSU) {
        #ifdef DEBUG
            printf("[Thread]RecvPacket: LSU packet\n");
        #endif
            OSPFLSU* ospf_lsu = (OSPFLSU*)(packet_rcv + IPHDR_LEN + OSPFHDR_LEN);
            int lsa_num = ntohl(ospf_lsu->num);
        
            char* lsa_ptr = (char*)(ospf_lsu) + sizeof(OSPFLSU); // +4
            for (int i = 0; i < lsa_num; ++i) {
                // receive lsa form ospf_lsu
                LSAHeader* lsa_header = (LSAHeader*)lsa_ptr;
                lsdb.addLSA(lsa_ptr);
                lsa_ptr += lsa_header->length;
                sendPackets((char*)lsa_header, LSAHDR_LEN, T_LSAck, ntohl(inet_addr("225.0.0.5")), interface);
            }
        }

        after_dealing:;
    }

    free(packet_rcv);
#undef RECV_LEN
}