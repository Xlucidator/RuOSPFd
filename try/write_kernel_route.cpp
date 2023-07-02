#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/route.h>
#include <iostream>
#include <cstring>
#include <sys/ioctl.h> // ioctl
#include <unistd.h> // close

int main() {
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    // 设置路由表条目
    struct rtentry route;
    std::memset(&route, 0, sizeof(route));
    route.rt_dst.sa_family = AF_INET;
    inet_pton(AF_INET, "10.128.5.0", &((struct sockaddr_in *)&route.rt_dst)->sin_addr);
    route.rt_genmask.sa_family = AF_INET;
    inet_pton(AF_INET, "255.255.255.0", &((struct sockaddr_in *)&route.rt_genmask)->sin_addr);
    ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.75.4", &((struct sockaddr_in *)&route.rt_gateway)->sin_addr);
    route.rt_metric = 1;
    route.rt_flags = RTF_UP | RTF_GATEWAY;

    // 添加路由表条目
    int ret = ioctl(sockfd, SIOCADDRT, &route);
    if (ret == -1) {
        std::cerr << "Failed to add route" << std::endl;
        return -1;
    }

    std::cout << "Route added successfully" << std::endl;

    // 删除路由表条目
    ret = ioctl(sockfd, SIOCDELRT, &route);
    if (ret == -1) {
        std::cerr << "Failed to delete route" << std::endl;
        return -1;
    }

    std::cout << "Route deleted successfully" << std::endl;

    // 关闭套接字
    close(sockfd);

    return 0;
}