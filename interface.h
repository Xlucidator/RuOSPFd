#ifndef INTERFACE_H
#define INTERFACE_H

#include "common.h"

#include <list>
#include "neighbor.h"

class Interface {
public:

    uint32_t    dr = 0;
    uint32_t    bdr = 0;
    std::list<Neighbor*> neighbor_list;
    Interface()=default;
};

#endif // INTERFACE_H