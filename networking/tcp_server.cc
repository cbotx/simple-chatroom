#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <ostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "tcp_server.h"

using namespace net;


bool TCPServer::Init(int port) {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "Create socket failed" << std::endl;
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd_, (sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << "Bind to IP/port failed" << std::endl;
        return false;
    }
    if (listen(server_fd_, SOMAXCONN) == -1) {
        std::cerr << "Listen failed" << std::endl;
        return false;
    }
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        std::cerr << "Epoll create failed" << std::endl;
        return false;
    }

    AddFD(epoll_fd_, server_fd_);
    
    std::cout << "Listening on port " << port << std::endl;
    return true;
}


void TCPServer::Start(int port) {
    static epoll_event events[MAX_EVENTS];

    if (!Init(port)) return;
    
    bool running = true;
    while(running) {
        int event_cnt = epoll_wait(epoll_fd_, events, MAX_EVENTS, EPOLL_TIMEOUT);
        if (event_cnt == -1) {
            std::cerr << "Epoll wait failed" << std::endl;
        }
        for (int i = 0; i < event_cnt; ++i) {
            int socket_fd = events[i].data.fd;
            if (socket_fd == server_fd_) {
                sockaddr_in client_address;
                socklen_t client_address_len = sizeof(sockaddr_in);
                int client_fd = accept(server_fd_, (sockaddr*)&client_address, &client_address_len);
                std::cout << "Client connect from " << inet_ntoa(client_address.sin_addr) << ':' << ntohs(client_address.sin_port) << ", client_fd=" << client_fd << std::endl;

                AddFD(epoll_fd_, client_fd);

                clients_.insert(client_fd);
                std::cout << clients_.size() << " clients total:" << std::endl;

                for (const auto &client : clients_) {
                    std::cout << client << ", ";
                }
                std::cout << std::endl;
                SendMessage(MessageType::AUTH, client_fd, std::to_string(client_fd).c_str());
            } else {
                RedirectMessage(socket_fd);
            }
        }
    }
}


bool TCPServer::SendMessage(MessageType type, int client_fd, const char* send_msg) {
    static Message msg;
    memset(&msg.message, 0, sizeof(msg.message));
    memcpy(&msg.message, send_msg, std::min(sizeof(msg.message), sizeof(send_msg)));
    msg.type = type;
    msg.src = -1;
    return SendMessage(client_fd, msg);
}


bool TCPServer::SendMessage(int client_fd, const Message &msg) {
    static char send_buffer[BUFFER_SIZE];
    memset(send_buffer, 0, sizeof(send_buffer));
    memcpy(send_buffer, &msg, sizeof(msg));
    if (send(client_fd, send_buffer, sizeof(send_buffer), 0) == -1) {
        std::cerr << "Send message failed" << std::endl;
        return false;
    }
    return true;
}

bool TCPServer::RedirectMessage(int client_fd) {
    static char recv_buffer[BUFFER_SIZE];
    Message msg;
    int len = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
    memset(&msg, 0, sizeof(msg));
    memcpy(&msg, recv_buffer, sizeof(msg));
    if (len == 0) {
        close(client_fd);
        clients_.erase(client_fd);
        std::cout << "Client " << client_fd << " closed." << std::endl;
    } else {
        if (msg.type == MessageType::PUBLIC) {
            for (int dst_fd : clients_) {
                if (dst_fd != client_fd) {
                    if (!SendMessage(dst_fd, msg)) {
                        return false;
                    }
                }
            }
        } else {
            int dst_fd = msg.dst;
            if (clients_.find(dst_fd) != clients_.end()) {
                if (!SendMessage(dst_fd, msg)) {
                    return false;
                }
            } else {
                SendMessage(MessageType::SERVER, client_fd, "Failed: User Offline");
            }
        }
    }
    return true;
}