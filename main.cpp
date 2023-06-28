#include "common.h"

#include "interface.h"
#include "packet_manage.h"
#include "lsdb.h"

#include <pthread.h>

bool start_ospfd();

LSDB lsdb;

int main(int argc, char** argv) {
    // start_ospfd();
    pthread_t hello_sender_thread;
    pthread_t receiver_thread;

    /* init interface */
    Interface interface1;
    interface1.ip = ntohl(inet_addr("192.168.75.128")); // TODO: read nic for addr
    myconfigs::interfaces.push_back(&interface1);

    /* init thread global attribute */
    pthread_attr_init(&myconfigs::thread_attr);
    pthread_attr_setdetachstate(&myconfigs::thread_attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&hello_sender_thread, NULL, threadSendHelloPackets, &interface1);
    pthread_create(&receiver_thread, NULL, threadRecvPackets, &interface1);

    pthread_join(hello_sender_thread, NULL);
    pthread_join(receiver_thread, NULL);
    return 0;
}

bool start_ospfd() {
    int fd;
    switch(fork()) {
        case -1: 
            printf("fork() failed\n");
            return false;
        case 0:  // child process
            break;
        default: // parent process
            exit(0);
    }

    /* 创建新对话 */
    if (setsid() == -1) {
        printf("setsid() failed\n");
        return false;
    }

    switch (fork()) {
        case -1:
            printf("fork() failed\n");
            return false;
        case 0:
            break;
        default:
            exit(0);
    }

    umask(0);

    long maxfd;
    if ((maxfd = sysconf(_SC_OPEN_MAX)) != -1) {
        for (fd = 0; fd < maxfd; fd++) {
            close(fd);
        }
    }

    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        printf("open(\"/dev/null\") failed\n");
        return false;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        printf("dup2(STDIN) failed\n");
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        printf("dup2(STDOUT) failed\n");
        return false;
    }

    if (dup2(fd, STDERR_FILENO) == -1) {
        printf("dup2(STDERR) failed\n");
        return false;
    }

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            printf("close() failed\n");
            return false;
        }
    }

    return true;
}