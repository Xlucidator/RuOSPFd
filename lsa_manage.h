#ifndef LSA_MANAGE_H
#define LSA_MANAGE_H

LSARouter*  genRouterLSA(std::vector<Interface*>& sel_interfaces);
LSANetwork* genNetworkLSA(Interface *interface);

void onGeneratingRouterLSA();

#endif // LSA_MANAGE_H