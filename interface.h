#ifndef INTERFACE_H
#define INTERFACE_H

#include <list>
#include <stdint.h>
#include "neighbor.h"
#include "retransmitter.h"

enum struct NetworkType : uint8_t {
    T_P2P = 1,
    T_BROADCAST,
    T_NBMA,
    T_P2MP,
    T_VIRTUAL,
};

enum struct InterfaceState : uint8_t {
    S_DOWN = 0,
    S_LOOPBACK,
    S_WAITING,
    S_POINT2POINT,
    S_DROTHER,
    S_BACKUP,
    S_DR,
};

enum struct InterfaceEvent : uint8_t {
    E_INTERFACEUP = 0,
    E_WAITTIMER,
    E_BACKUPSEEN,
    E_NEIGHBORCHANGE,
    E_LOOPIND,
    E_UNLOOPIND,
    E_INTERFACEDOWN,
};

void* waitTimer(void* intf);

class Interface {
public:
    NetworkType    type  = NetworkType::T_BROADCAST;  // default broadcast in ensp
    InterfaceState state = InterfaceState::S_DOWN;
    in_addr_t   ip;
    uint32_t    mask = 0xffffff00;
    uint32_t    area_id = 0;

    uint32_t    hello_intervel = 10;
    uint32_t    router_dead_interval = 40;  // 4 times of hello_intervel
    uint32_t    rxmt_interval  = 5;
    uint32_t    intf_trans_delay = 1;
    uint32_t    router_priority = 1;

    Retransmitter rxmtter = Retransmitter(this);  // use for rxmt

    uint32_t    cost = 0;
    uint32_t    mtu = 1500;

    uint32_t    dr = 0;
    uint32_t    bdr = 0;
    std::list<Neighbor*> neighbor_list;
    
    Interface()=default;
    ~Interface();
    Neighbor* getNeighbor(in_addr_t ip);
    Neighbor* addNeighbor(in_addr_t ip);
    /* event */
    void eventInterfaceUp();
    void eventWaitTimer();
    void eventBackUpSeen();
    void eventNeighborChange();
private:
    void electDesignedRouter();
};

#endif // INTERFACE_H