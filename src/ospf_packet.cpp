#include "common.h"
#include "ospf_packet.h"
#include <netinet/in.h>

uint16_t not_a_fletcher_checksum(const void* data, size_t len) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    uint16_t sum1 = 0, sum2 = 0;
    
    for (size_t i = 0; i < len; ++i) {
        sum1 = (sum1 + ptr[i]) % 255;
        sum2 = (sum2 + sum1)   % 255;
    }

    return (sum2 << 8) | sum1;
}

uint16_t fletcher_checksum(const void* data, size_t len, int checksum_offset) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    int length = len;

    int32_t x, y;
	uint32_t mul;
	uint32_t c0 = 0, c1 = 0;
	uint16_t checksum = 0;

	for (int index = 0; index < length; index++) {
		if (index == checksum_offset ||
			index == checksum_offset+1) {
            // in case checksum has not set 0 before
			c1 += c0;
			ptr++;
		} else {
			c0 = c0 + *(ptr++);
			c1 += c0;
		}
	}

	c0 = c0 % 255;
	c1 = c1 % 255;	
    mul = (length - checksum_offset)*(c0);
  
	x = mul - c0 - c1;
	y = c1 - mul - 1;

	if ( y >= 0 ) y++;
	if ( x < 0 ) x--;

	x %= 255;
	y %= 255;

	if (x == 0) x = 255;
	if (y == 0) y = 255;

	y &= 0x00FF;
  
	return (x << 8) | y;
}

/* OSPF Packet */
void OSPFLSR::net2host() {
    type = ntohl(type);
    state_id = ntohl(state_id);
    adverising_router = ntohl(adverising_router);
}

/* ============ LSA Header ============ */
LSAHeader::LSAHeader() {
    ls_age = 100;
    options = 0x2;
    ls_sequence_number = 0x80000000; // init
}

void LSAHeader::host2net() {
    ls_age = htons(ls_age);
    link_state_id = htonl(link_state_id);
    advertising_router = htonl(advertising_router);
    ls_checksum = htons(ls_checksum);
    length = htons(length);
    ls_sequence_number = htonl(ls_sequence_number);
}

void LSAHeader::net2host() {
    ls_age = ntohs(ls_age);
    link_state_id = ntohl(link_state_id);
    advertising_router = ntohl(advertising_router);
    ls_checksum = ntohs(ls_checksum);
    length = ntohs(length);
    ls_sequence_number = ntohl(ls_sequence_number);
}

void LSAHeader::printInfo() {
    // assume data is in the host sequence
    printf("====[LSA Header]====\n");
    printf("ls_age: %d\n", ls_age);
    printf("ls_type: %d\n", ls_type);
    ipaddr_tmp.s_addr = htonl(link_state_id);
    printf("link_state_id: %s\n", inet_ntoa(ipaddr_tmp));
    ipaddr_tmp.s_addr = htonl(advertising_router);
    printf("advertising_router: %s\n", inet_ntoa(ipaddr_tmp));
    printf("ls_sequence_number: %x\n", ls_sequence_number);
    printf("====================\n");
}

/* ============ Router Link ============ */
LSARouterLink::LSARouterLink() {
    tos_num = 0;    // we assume 0 for convenience
    metric  = 1;
}

LSARouterLink::LSARouterLink(char* net_ptr) {
    link_id   = ntohl(*(uint32_t*)(net_ptr));
    link_data = ntohl(*(uint32_t*)(net_ptr + 4));
    type      = *(net_ptr + 8);
    tos_num   = 0;    // we assume 0 for convenience
    metric    = ntohs(*(uint16_t*)(net_ptr + 10));
}

bool LSARouterLink::operator==(const LSARouterLink& other) {
    return link_id   == other.link_id && 
           link_data == other.link_data && 
           type      == other.type &&
           tos_num   == other.tos_num &&
           metric    == other.metric;
}


/* ============ LSA ============ */
LSA::LSA() {
    lsa_header = LSAHeader();
}

/* bigger(>) means newer */
bool LSA::operator>(const LSA& other) {
    // comparible: two Instances of one LSA
    // assert link_state_id and advertising_router is equal
    assert(lsa_header.link_state_id == other.lsa_header.link_state_id);
    assert(lsa_header.advertising_router == other.lsa_header.advertising_router);
    if (this->lsa_header.ls_sequence_number > other.lsa_header.ls_sequence_number) {
        return true;
    } else if (this->lsa_header.ls_sequence_number == other.lsa_header.ls_sequence_number) {
        // TODO: compare checksum and more
    }
    return false;
}

/* ============ Router LSA ============ */
LSARouter::LSARouter() {
    // lsa_header = LSAHeader();
    lsa_header.ls_type = 1;
    zero1 = 0;
    b_B = b_E = b_V = 0;
    zero2 = 0;
    link_num = 0;
}

LSARouter::LSARouter(char* net_ptr) {
    /* init lsa_headr */
    lsa_header = *((LSAHeader*)net_ptr);
    lsa_header.net2host();
    /* init data part */
    char* data_net_ptr = net_ptr + LSAHDR_LEN;
    zero1 = 0; 
    b_B = b_E = b_V = 0; // TODO: unfulfilled tags
    zero2 = 0;
    link_num = ntohs(*(uint16_t*)(data_net_ptr + 2));
    // fill links
    char* link_net_ptr = data_net_ptr + 4;
    for (int i = 0; i < link_num; ++i) {
        links.emplace_back(LSARouterLink(link_net_ptr));
        link_net_ptr += sizeof(LSARouterLink);
    }
}

char* LSARouter::toRouterLSA() {
    char *rlsa_data = new char[size()];
    /* fix some dynamic values */
    lsa_header.length = size();
    lsa_header.ls_checksum = 0; // init to avoid disturb
    link_num = links.size();
    /* fill lsa_header */
    LSAHeader lsa_header_net = lsa_header;
    lsa_header_net.host2net();
    memcpy(rlsa_data, &lsa_header_net, LSAHDR_LEN);
    /* fill flags */
    uint16_t* p_flags = (uint16_t*)(rlsa_data + LSAHDR_LEN);
    *p_flags = 0;
    /* fill link_num */
    uint16_t* p_link_num = p_flags + 1;
    *p_link_num = htons(link_num);
    /* fill lsa router links */
    LSARouterLink* p_router_link = (LSARouterLink*)(p_link_num + 1);
    for (auto& link: links) {
        p_router_link->link_id   = htonl(link.link_id);
        p_router_link->link_data = htonl(link.link_data);
        p_router_link->type      = link.type;
        p_router_link->tos_num   = 0;   // 0 in arbitrary
        p_router_link->metric    = htons(link.metric);
        p_router_link += 1;
    }
    /* fill osi fletcher checksum */
    lsa_header.ls_checksum = fletcher_checksum(rlsa_data+2, lsa_header.length-2, 14);
#ifdef DEBUG
    uint16_t ck2 = not_a_fletcher_checksum(rlsa_data+2, lsa_header.length-2);
    printf("fletcher1: %x\n", lsa_header.ls_checksum);
    printf("fletcher2: %x\n", ck2);
#endif
    LSAHeader* p_lsa_header = (LSAHeader*)rlsa_data;
    p_lsa_header->ls_checksum = htons(lsa_header.ls_checksum);

    return rlsa_data;
}

char* LSARouter::toLSAPacket() {
    char *rlsa_data = new char[size()];
    /* fix some dynamic values */
    lsa_header.length = size();
    lsa_header.ls_checksum = 0; // init to avoid disturb
    link_num = links.size();
    /* fill lsa_header */
    LSAHeader lsa_header_net = lsa_header;
    lsa_header_net.host2net();
    memcpy(rlsa_data, &lsa_header_net, LSAHDR_LEN);
    /* fill flags */
    uint16_t* p_flags = (uint16_t*)(rlsa_data + LSAHDR_LEN);
    *p_flags = 0;
    /* fill link_num */
    uint16_t* p_link_num = p_flags + 1;
    *p_link_num = htons(link_num);
    /* fill lsa router links */
    LSARouterLink* p_router_link = (LSARouterLink*)(p_link_num + 1);
    for (auto& link: links) {
        p_router_link->link_id   = htonl(link.link_id);
        p_router_link->link_data = htonl(link.link_data);
        p_router_link->type      = link.type;
        p_router_link->tos_num   = 0;   // 0 in arbitrary
        p_router_link->metric    = htons(link.metric);
        p_router_link += 1;
    }
    /* fill osi fletcher checksum */
    lsa_header.ls_checksum = fletcher_checksum(rlsa_data+2, lsa_header.length-2, 14);
#ifdef DEBUG
    uint16_t ck2 = not_a_fletcher_checksum(rlsa_data+2, lsa_header.length-2);
    printf("fletcher1: %x\n", lsa_header.ls_checksum);
    printf("fletcher2: %x\n", ck2);
#endif
    LSAHeader* p_lsa_header = (LSAHeader*)rlsa_data;
    p_lsa_header->ls_checksum = htons(lsa_header.ls_checksum);
    
    return rlsa_data;
}

size_t LSARouter::size() {
    return LSAHDR_LEN + 4 + sizeof(LSARouterLink) * links.size();
}

bool LSARouter::operator==(const LSARouter& other) {    // lsa update? i don't think it's necessary to compare all
    if (lsa_header.link_state_id      != other.lsa_header.link_state_id ||
        lsa_header.advertising_router != other.lsa_header.advertising_router ||
        lsa_header.ls_sequence_number != other.lsa_header.ls_sequence_number
        ) return false;
    return true;
}


/* ============ Netword LSA ============ */
LSANetwork::LSANetwork() {
    // lsa_header = LSAHeader();
    lsa_header.ls_type = 2;
    network_mask = 0xffffff00;  // not good
}

LSANetwork::LSANetwork(char* net_ptr) {
    /* init lsa_headr */
    lsa_header = *((LSAHeader*)net_ptr);
    lsa_header.net2host();
    /* init data part */
    char* data_net_ptr = net_ptr + LSAHDR_LEN;
    network_mask = ntohl(*(uint32_t*)data_net_ptr);
    /* fill routers vector */
    int router_num = (lsa_header.length - LSAHDR_LEN - 4) / 4;
    uint32_t* router_net_ptr = (uint32_t*)(data_net_ptr + 4);
    for (int i = 0; i < router_num; ++i) {
        attached_routers.push_back(ntohl(*router_net_ptr));
        router_net_ptr += 1;
    }
}

char* LSANetwork::toNetworkLSA() {
    char* nlsa_data = new char[size()];
    /* fix some dynamic values */
    lsa_header.length = size();
    lsa_header.ls_checksum = 0; // init to avoid disturb
    /* fill lsa_header */
    LSAHeader lsa_header_net = lsa_header;
    lsa_header_net.host2net();
    memcpy(nlsa_data, &lsa_header_net, LSAHDR_LEN);
    /* fill network_mask and attached routers */
    uint32_t* ptr = (uint32_t*)(nlsa_data + LSAHDR_LEN);
    ptr[0] = htonl(network_mask);
    ptr += 1;
    for (auto& router : attached_routers) {
        *ptr = htonl(router);
        ptr += 1;
    }
    /* fill osi fletcher checksum */
    lsa_header.ls_checksum = fletcher_checksum(nlsa_data+2, lsa_header.length-2, 14);
    LSAHeader* p_lsa_header = (LSAHeader*)nlsa_data;
    p_lsa_header->ls_checksum = htons(lsa_header.ls_checksum);
    
    return nlsa_data;
}

char* LSANetwork::toLSAPacket() {
    char* nlsa_data = new char[size()];
    /* fix some dynamic values */
    lsa_header.length = size();
    lsa_header.ls_checksum = 0; // init to avoid disturb
    /* fill lsa_header */
    LSAHeader lsa_header_net = lsa_header;
    lsa_header_net.host2net();
    memcpy(nlsa_data, &lsa_header_net, LSAHDR_LEN);
    /* fill network_mask and attached routers */
    uint32_t* ptr = (uint32_t*)(nlsa_data + LSAHDR_LEN);
    ptr[0] = htonl(network_mask);
    ptr += 1;
    for (auto& router : attached_routers) {
        *ptr = htonl(router);
        ptr += 1;
    }
    /* fill osi fletcher checksum */
    lsa_header.ls_checksum = fletcher_checksum(nlsa_data+2, lsa_header.length-2, 14);
    LSAHeader* p_lsa_header = (LSAHeader*)nlsa_data;
    p_lsa_header->ls_checksum = htons(lsa_header.ls_checksum);
    
    return nlsa_data;
}

size_t LSANetwork::size() {
    return LSAHDR_LEN + 4 + sizeof(uint32_t) * attached_routers.size();
}

bool LSANetwork::operator==(const LSANetwork& other) {
    if (lsa_header.link_state_id      != other.lsa_header.link_state_id ||
        lsa_header.advertising_router != other.lsa_header.advertising_router ||
        lsa_header.length             != other.lsa_header.length ||
        network_mask != other.network_mask ||
        attached_routers.size() != other.attached_routers.size()) return false;
    for (int i = 0; i < attached_routers.size(); ++i) {
        if (!(attached_routers[i] == other.attached_routers[i])) 
            return false;
    }
    return true;
}