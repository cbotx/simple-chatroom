#ifndef __W_COMMON__
#define __W_COMMON__

#include <sys/epoll.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MAX_EVENTS 5000
#define EPOLL_TIMEOUT 10000


static void AddFD(int epoll_fd, int fd) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

enum MessageType {
    DEFAULT = 0,
    PUBLIC = 0,
    PRIVATE,
    AUTH,
    SERVER,
    EXIT,
};

struct Message {
    MessageType type;
    int src;
    int dst;
    char message[BUFFER_SIZE];
};

#endif