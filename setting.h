#ifndef SETTINGS_H
#define SETTINGS_H

#define DEBUG 1

#include <stdint.h>
#include <vector>
#include "interface.h"

namespace myconfigs {
    extern const char* nic_name;
    extern uint32_t router_id;
    extern uint32_t area_id;
    extern std::vector<Interface*> interfaces;
} // namespace Configs

#endif // SETTINGS_H
