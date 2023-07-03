#ifndef ROUTING_H
#define ROUTING_H

#include <stdint.h>
#include <map>
#include <linux/route.h>
#include "interface.h"

struct EdgeToVertex {
    uint32_t src_ip;
    // uint32_t dst_ip;  // cannot get at once
    uint32_t cost;
    uint32_t end_vertex_id;

    EdgeToVertex();
    EdgeToVertex(uint32_t src, uint32_t c, uint32_t end_rtr_id); // usually we don't know the dst_ip in TRANSIT
};

struct Vertex {
    uint32_t rtr_id;
    std::map<uint32_t, EdgeToVertex> adjacencies;

    Vertex();
    Vertex(uint32_t router_id);
    void printInfo();
};

struct ToVertex {
    uint32_t dst_vtx_id;
    uint32_t metric;  // sum of costs along the path
    /* extra info */
    uint32_t next_hop;          // based on self router
    Interface* out_interface;   // based on self router

    ToVertex();
    ToVertex(uint32_t rtr_id);
    ToVertex(uint32_t rtr_id, uint32_t cost);
    ToVertex(uint32_t rtr_id, uint32_t cost, uint32_t next_hop, Interface* interface);
    void printInfo();
};

struct RouteItem {
    uint32_t dest;
    uint32_t mask;
    uint32_t next_hop;
    uint32_t metric;
    uint8_t  type;
    // TODO: interface <--> dev

    RouteItem();
    RouteItem(uint32_t dst_ip, uint32_t next_hop, uint32_t metric);
    void printInfo();
};

class RouteTable {
public:
    /* about content */
    std::map<uint32_t, Vertex> topo;     // <rtr_id, v >
    std::map<uint32_t, ToVertex> result; // <rtr_id, tv>
    std::map<uint32_t, RouteItem> routings; // <net, routeitem>

    /* interact with linux kernel route */
    int routefd;  // socket to read/write kernel route
    std::vector<struct rtentry> rtentries_written;  // route that has been written to kernel

    RouteTable();
    ~RouteTable();
    void updateRoutings();
    void printTopology();
    void printResult();
    void printRouting();

private:
    void buildTopology();
    void caclulateRouting();
    void genRouting();
    void resetRoute();  // reset kernel route to normal
    void writeKernelRoute();
};

extern RouteTable route_manager;

#endif // ROUTING_H