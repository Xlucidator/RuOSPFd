#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <deque>
#include <map>
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

    /* last received DD data (of packet) from the neighbor
     *      [means]: host_interface last send, and only data oart of a dd packet for convenience
     */
    uint32_t    last_dd_seq_num;
    uint32_t    last_dd_data_len;
    char        last_dd_data[1024];

    /* neighbor details */
    uint32_t    id;  
    uint32_t    priority;  
    uint32_t    ip;
    uint32_t    ndr;    // neighbor's designated router
    uint32_t    nbdr;   // neighbor's backup designated router
    Interface*  host_interface;

    /* LSA information */
    std::map<uint32_t, uint32_t> link_state_rxmt_map;  // {dd_seq_num:id}
    std::deque<LSAHeader> db_summary_list; // already in net order
    std::deque<LSAHeader> link_state_req_list;
    pthread_mutex_t link_state_req_list_lock;
    pthread_t empty_dd_send_thread; // for negotiation of master/slave
    pthread_t lsr_send_thread;

    Neighbor(in_addr_t ip, Interface* intf);
    ~Neighbor();
    void reqListRemove(uint32_t link_state_id, uint32_t advertise_rtr_id);

    void eventHelloReceived();  // neighbor's hello has been received
    void event2WayReceived();
    void event1WayReceived();
    void eventNegotiationDone();
    void eventSeqNumberMismatch();
    void eventExchangeDone();
    void evnetBadLSReq();
    void eventLoadDone();

private:
    void initDBSummaryList();
};


#endif // NEIGHBOR_H