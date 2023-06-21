#include "ospf_packet.h"

LSAHeader::LSAHeader() {
    ls_age = 100;
    options = 0x2;
}

LSARouter::LSARouter() {
    zero1 = 0;
    b_B = b_E = b_V = 0;
    zero2 = 0;
    link_num = 0;
}

char* LSARouter::toRouterLSA() {
    link_num = links.size();

}