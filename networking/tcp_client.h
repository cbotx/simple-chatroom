#ifndef __TCP_CLIENT__
#define __TCP_CLIENT__

#include <string>

#include "common.h"

namespace net {
    class TCPClient {
    public:
        TCPClient() = default;
        bool Connect(std::string ip, int port);
        bool Connect(const char* ip, int port);
        void Start();
        void Close();
    private:
        bool ConstructMessage(const std::string &input);
        void TCPClient::ReceiveMessage();
        void TCPClient::ReadPipe(int pipe_fd);
        int client_id_ = -1;
        int client_fd_ = -1;
        int pipe_fd_[2] = {-1, -1};
        int epoll_fd_ = -1;
        int pid_ = -1;
        bool is_run_ = true;
        Message msg_;
        char send_buffer_[BUFFER_SIZE];
        char recv_buffer_[BUFFER_SIZE];
    };
} // namespace net

#endif