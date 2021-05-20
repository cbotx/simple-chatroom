#ifndef __TCP_SERVER__
#define __TCP_SERVER__

#include <set>

#include "common.h"

namespace net {
    class TCPServer {
    public:
        TCPServer() = default;
        void Start(int port);
    private:
        bool SendMessage(MessageType type, int client_fd, const char* msg);
        bool SendMessage(int client_fd, const Message &msg);
        bool RedirectMessage(int client_fd);
        bool Init(int port);
        int server_fd_ = -1;
        int epoll_fd_ = -1;
        std::set<int> clients_;
    };
} // namespace net

#endif