#include "retransmitter.h"
#include "interface.h"
#include "packet_manage.h"
#include "common.h"

PacketData::PacketData(const char* data, size_t data_len, uint8_t type, uint32_t dst_ip, uint32_t duration) {
    this->data = data;
    this->data_len = data_len;
    this->type = type;
    this->dst_ip = dst_ip;

    this->age = 0;
    this->duration = duration;
}

PacketData::~PacketData() {
    // free((void*)data);  will cause 'free(): double free detected in tcache 2'
    // TODO : why, seem to have memory leak
}


Retransmitter::Retransmitter(Interface* interface) {
    host_interface = interface;
    alloc_id = 0;
    pthread_mutex_init(&list_lock, NULL);
}

uint32_t Retransmitter::addPacketData(PacketData packet_data) {
    packet_data.id = alloc_id++;
    pthread_mutex_lock(&list_lock);
        packet_list.push_back(packet_data);
    pthread_mutex_unlock(&list_lock);
    #ifdef DEBUG
        printf("[rxmtter]add packet: type %d, id %d\n", packet_data.type, packet_data.id);
    #endif
    return packet_data.id;
}

void Retransmitter::delPacketData(uint32_t id) {
    pthread_mutex_lock(&list_lock);
        for (auto it = packet_list.begin(); it != packet_list.end(); ++it) {
            if (it->id == id) {
            #ifdef DEBUG
                printf("[rxmtter]del packet: type %d, id %d\n", it->type, it->id);
            #endif
                packet_list.erase(it);
                break;
            }
        }
    pthread_mutex_unlock(&list_lock);
}

void* Retransmitter::run(void* retrans) {
    Retransmitter* retransmitter = (Retransmitter*) retrans;
    while (true) {
        pthread_mutex_lock(&retransmitter->list_lock);
        for (auto& packet_d: retransmitter->packet_list) {
            packet_d.age += 1;
            if (packet_d.age == packet_d.duration) {
                packet_d.age = 0;
                sendPackets(packet_d.data, packet_d.data_len, packet_d.type, packet_d.dst_ip, retransmitter->host_interface);
            }
        }
        pthread_mutex_unlock(&retransmitter->list_lock);
        sleep(1);   // update per seconds (approximately)
    }
}