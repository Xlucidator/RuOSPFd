#ifndef INTERFACE_H
#define INTERFACE_H

#include "common.h"

#include <list>
#include "neighbor.h"

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

class Interface {
public:
    NetworkType    type  = NetworkType::T_P2P;
    InterfaceState state = InterfaceState::S_DOWN;
    in_addr_t   ip;
    uint32_t    mask = 0xffffff00;
    uint32_t    area_id = 0;

    uint32_t    hello_intervel = 10;
    uint32_t    router_dead_interval = 40;
    uint32_t    intf_trans_delay;

    // TODO: Timer

    uint32_t    cost = 0;
    uint32_t    mtu = 1500;

    uint32_t    dr = 0;
    uint32_t    bdr = 0;
    std::list<Neighbor*> neighbor_list;
    
    Interface()=default;
    Neighbor* getNeighbor(in_addr_t ip);
    Neighbor* addNeighbor(in_addr_t ip);
};

#endif // INTERFACE_H