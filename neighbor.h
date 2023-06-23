#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "ospf_packet.h"

enum struct NeighborState : uint8_t {
    S_DOWN = 0,
    S_ATTEMPT,
    S_INIT,
    S_2WAY,
    S_EXSTART,
    S_EXCHANGE,
    S_LOADING,
    S_FULL,
};

enum struct NeighborEvent : uint8_t {
    E_HELLORECV = 0,
    E_START,
    E_2WAYRECV,
    E_NEGOTIATIONDONE,
    E_EXCHANGEDONE,
    E_BADLSREQ,
    E_LOADINGDONE,
    E_ADJOK,
    E_SEQNUMMISMATCH,
    E_1WAY,
    E_KILLNBR,
    E_INACTTIMER,
    E_LLDOWN,
};

class Interface;

class Neighbor {
public:
    NeighborState state = NeighborState::S_DOWN;
    bool        is_master;
    uint32_t    dd_seq_num;

    /* last received DD packet from the neighbor */
    uint32_t    last_dd_seq_num;
    uint32_t    last_dd_len;
    char        last_dd_packet[1024];

    /* neighbor details */
    uint32_t    id;  
    uint32_t    priority;  
    uint32_t    ip;
    uint32_t    ndr;    // neighbor's designated router
    uint32_t    nbdr;   // neighbor's backup designated router
    Interface*  host_interface;

    /* LSA information */
    // TODO: link state retransmission list
    std::list<LSAHeader> db_summary_list;
    std::list<LSAHeader> link_state_req_list;
    pthread_t empty_dd_send_thread; // for negotiation of master/slave

    Neighbor(in_addr_t ip, Interface* intf);

    void eventHelloReceived();  // neighbor's hello has been received
    void event2WayReceived();
    void event1WayReceived();
    void eventNegotiationDone();
    void eventSeqNumberMismatch();

private:
    void initDBSummaryList();
};


#endif // NEIGHBOR_H