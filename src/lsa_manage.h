#ifndef LSA_MANAGE_H
#define LSA_MANAGE_H

#include "ospf_packet.h"
#include "interface.h"

LSARouter*  genRouterLSA(std::vector<Interface*>& sel_interfaces);
LSANetwork* genNetworkLSA(Interface *interface);

void onGeneratingRouterLSA();
void onGeneratingNetworkLSA(Interface* interface);

#endif // LSA_MANAGE_H