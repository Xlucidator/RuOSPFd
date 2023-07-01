#include "routing.h"
#include "common.h"
#include "lsdb.h"
#include "ospf_packet.h"

#define INF (0x7fffffff)

/* Vertex */
Vertex::Vertex() {}
Vertex::Vertex(uint32_t router_id):rtr_id(router_id) {}

void Vertex::printInfo() {
    ipaddr_tmp.s_addr = htonl(rtr_id);
    printf("Vertex [%-15s]\n", inet_ntoa(ipaddr_tmp));
    for (auto& adj_item: adjacencies) {
        EdgeToVertex* e2v = &adj_item.second;
        ipaddr_tmp.s_addr = htonl(e2v->src_ip);
        printf("\t(%-15s)----%02d---->",inet_ntoa(ipaddr_tmp), e2v->cost);
        ipaddr_tmp.s_addr = htonl(e2v->end_vertex_id);
        printf("[%-15s]\n", inet_ntoa(ipaddr_tmp));
    }
}

/* EdgeToVertex */
EdgeToVertex::EdgeToVertex() {
    this->src_ip = 0;
    this->cost = 0;
    this->end_vertex_id = 0;
}
EdgeToVertex::EdgeToVertex(uint32_t src, uint32_t c, uint32_t end_rtr_id) {
    this->src_ip = src;
    this->cost = c;
    this->end_vertex_id = end_rtr_id;
}

/* ToVertex */
ToVertex::ToVertex() {}
ToVertex::ToVertex(uint32_t rtr_id):dst_vtx_id(rtr_id) {}
ToVertex::ToVertex(uint32_t rtr_id, uint32_t cost):dst_vtx_id(rtr_id), metric(cost) {}
ToVertex::ToVertex(uint32_t rtr_id, uint32_t cost, uint32_t next_hop, Interface* interface):dst_vtx_id(rtr_id), metric(cost), next_hop(next_hop), out_interface(interface) {}

void ToVertex::printInfo() {
    ipaddr_tmp.s_addr = htonl(dst_vtx_id);
    printf("to [%-15s]: %d\t", inet_ntoa(ipaddr_tmp), metric);
    ipaddr_tmp.s_addr = htonl(next_hop);
    printf("%-15s\n", inet_ntoa(ipaddr_tmp));
}

/* RouteItem */
RouteItem::RouteItem() {}
RouteItem::RouteItem(uint32_t dst_ip, uint32_t next_hop, uint32_t metric) {
    this->mask = 0xffffff00; // TODO: different mask
    this->dest = dst_ip;
    this->next_hop = next_hop;
    this->metric = metric;
    this->type = 1;
}

void RouteItem::printInfo() {
    ipaddr_tmp.s_addr = htonl(dest);
    printf("%-15s ", inet_ntoa(ipaddr_tmp));
    ipaddr_tmp.s_addr = htonl(mask);
    printf("|%-15s ", inet_ntoa(ipaddr_tmp));
    ipaddr_tmp.s_addr = htonl(next_hop);
    printf("|%-15s ", inet_ntoa(ipaddr_tmp));
    printf("| %d \n", metric);
}


/* RouteTable */
void RouteTable::buildTopology() {
    topo.clear();

    pthread_mutex_lock(&lsdb.router_lock);
    pthread_mutex_lock(&lsdb.network_lock);
        LSDB lsdb_snapshot = lsdb; // get the snapshot of current lsdb
    pthread_mutex_unlock(&lsdb.network_lock);
    pthread_mutex_unlock(&lsdb.router_lock);

    for (auto& rlsa: lsdb_snapshot.router_lsas) {
        /* init vertex */
        uint32_t self_id = rlsa->lsa_header.advertising_router; 
        if (topo.find(self_id) == topo.end()) {
            topo[self_id] = Vertex(self_id);
        }
        Vertex* vtx = &topo[self_id];
        /* build edges */
        for (auto& link: rlsa->links) {
            uint32_t self_ip = link.link_data;
            uint32_t cost    = link.metric;
            uint32_t end_id;
            if (link.type == L_TRANSIT) {
                LSANetwork* nlsa = lsdb_snapshot.getNetworkLSA(link.link_id); // generated by dr
                for (uint32_t end_id: nlsa->attached_routers) {
                    if (end_id == self_id) {
                        continue;
                    }
                    vtx->adjacencies[end_id] = EdgeToVertex(self_ip, cost, end_id);
                }
            } else if (link.type == L_P2P) {
                end_id = link.link_id;
                vtx->adjacencies[end_id] = EdgeToVertex(self_ip, cost, end_id);
            } else { // L_STUB ignore, L_VIRTUAL todo
                continue;
            }
        }
    }
}

void RouteTable::caclulateRouting() {
    if (topo.size() == 0) {
    #ifdef DEBUG
        printf("topo is emtpy, need to run buildTopology!\n");
    #endif
        return;
    }

    result.clear();

    /*== run dijstra algorithm ==*/
    std::map<uint32_t, ToVertex> pending_vertexs;
    /* init pending_vertexes */
    for (auto& item: topo) {
        if (item.first != myconfigs::router_id) {
            pending_vertexs[item.first] = ToVertex(item.first, INF);
        }
    }
    /* init result */
    result[myconfigs::router_id] = ToVertex(myconfigs::router_id, 0, 0, nullptr);
    
    ToVertex tmp_tv(0, INF); 
    
    /* begin dijstra */
    uint32_t new_id = myconfigs::router_id;
    // struct about the 'newest item' added in to 'result'
    Vertex* new_v = nullptr;
    ToVertex* new_tv = nullptr;
    // struct about the adjacency about 'newest item' : will be updated
    Vertex* update_v = nullptr;
    EdgeToVertex* e2v = nullptr;
    ToVertex* update_tv = nullptr;
    while (pending_vertexs.size() > 0) {
        /* update pending_vertexes's metric */
        new_v  = &topo[new_id];
        new_tv = &result[new_id];
        if (new_v->adjacencies.size() == 0) {
            break; // to the end
        }
        for (auto& adj_item: new_v->adjacencies) {
            update_v  = &topo[adj_item.first];
            e2v       = &adj_item.second;
            if (pending_vertexs.find(e2v->end_vertex_id) == pending_vertexs.end()) {
                continue; // means: already in 'result'
            }
            update_tv = &pending_vertexs[e2v->end_vertex_id];
            if (update_tv->metric > new_tv->metric + e2v->cost) {
                update_tv->metric = new_tv->metric + e2v->cost;
                if (new_id == myconfigs::router_id) {
                    update_tv->next_hop = update_v->adjacencies[new_id].src_ip;
                    update_tv->out_interface = myconfigs::ip2interface[e2v->src_ip];
                } else {
                    update_tv->next_hop = new_tv->next_hop;
                    update_tv->out_interface = new_tv->out_interface;
                }
            }
        }

        /* select the minimal metric vertex from pending to result */
        ToVertex* min_tv = &tmp_tv; 
        for (auto& tv_item: pending_vertexs) {
            if (tv_item.second.metric < min_tv->metric) {
                min_tv = &tv_item.second;
            }
        }
        new_id = min_tv->dst_vtx_id;
        result[new_id] = *min_tv;
        pending_vertexs.erase(new_id);
    }
}

/* must after buildTopology */
void RouteTable::genRouting() {
    routings.clear();

    uint32_t rtr_id;
    for (auto& res_item: result) {
        rtr_id = res_item.first;
        if (rtr_id == myconfigs::router_id) {
            continue;
        }
        Vertex* vtx = &topo[rtr_id];
        ToVertex* to_vtx = &res_item.second;
        for (auto& adj_item: vtx->adjacencies) {
            EdgeToVertex *e2v = &adj_item.second;
            uint32_t dest_net = e2v->src_ip & 0xffffff00;
            if (routings.find(dest_net) == routings.end()) {
                routings[dest_net] = RouteItem(dest_net, to_vtx->next_hop, to_vtx->metric);
            } else {
                RouteItem* exist_rt = &routings[dest_net];
                if (exist_rt->metric > to_vtx->metric) {
                    // replace
                    routings[dest_net] = RouteItem(dest_net, to_vtx->next_hop, to_vtx->metric);
                }
            }
        }
    }
}

// public
void RouteTable::updateRoutings() {
#ifdef DEBUG
    printf("...Update OSPF Routing...\n");
#endif
    buildTopology();
    printTopology();

    caclulateRouting();
    printResult();

    genRouting();
    printRouting();

}

void RouteTable::printTopology() {
    printf("## Topology ##\n");
    for (auto& topo_item: topo) {
        topo_item.second.printInfo();
    }
    printf("##############\n");
}

void RouteTable::printResult() {
    printf("##  Result  ##\n");
    for (auto& result_item: result) {
        result_item.second.printInfo();
    }
    printf("##############\n");
}

void RouteTable::printRouting() {
    printf("##  Routing ##\n");
    for (auto& route_item: routings) {
        route_item.second.printInfo();
    }
    printf("##############\n");
}
