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

#include "tcp_client.h"

using namespace net;

bool TCPClient::Connect(std::string ip, int port) {
    const char* c_ip = ip.c_str();
    return Connect(c_ip, port);
}

bool TCPClient::Connect(const char* ip, int port) {
    client_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd_ == -1) {
        std::cerr << "Create socket failed" << std::endl;
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if(inet_pton(AF_INET, ip, &address.sin_addr)<=0) {
        std::cerr << "Invalid address" << std::endl;
        return false;
    }
    if (connect(client_fd_, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return false;
    }
    int result = recv(client_fd_, recv_buffer_, sizeof(recv_buffer_), 0);
    memset(&msg_, 0, sizeof(msg_));
    memcpy(&msg_, recv_buffer_, sizeof(msg_));
    if (result == 0) {
        std::cerr << "Connection closed" << std::endl;
        close(client_fd_);
        return false;
    }
    if (msg_.type != MessageType::AUTH) {
        std::cerr << "Received error message type" << std::endl;
        return false;
    }
    client_id_ = std::stoi(msg_.message);

    std::cout << "Successfully connected" << std::endl;
    if (pipe(pipe_fd_) < 0) {
        std::cerr << "Pipe error" << std::endl;
        return false;
    }
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        std::cerr << "Epoll create failed" << std::endl;
        return false;
    }
    AddFD(epoll_fd_, client_fd_);
    AddFD(epoll_fd_, pipe_fd_[0]);
    return true;
}

void TCPClient::Start() {
    static epoll_event events[MAX_EVENTS];
    pid_ = fork();
    if (pid_ < 0) {
        std::cerr << "Fork failed" << std::endl;
        close(client_fd_);
        return;
    } else if (pid_ == 0) {
        close(pipe_fd_[0]);
        while(is_run_) {
            std::string input;
            std::getline(std::cin, input);
            if (!ConstructMessage(input)) {
                continue;
            }

            if (msg_.type == MessageType::EXIT) {
                is_run_ = false;
            } else {
                memset(send_buffer_, 0, sizeof(send_buffer_));
                memcpy(send_buffer_, &msg_, sizeof(msg_));
                if (write(pipe_fd_[1], send_buffer_, sizeof(send_buffer_)) < 0) {
                    std::cerr << "Write to pipe failed" << std::endl;
                    return;
                }
            }
        }
    } else {
        close(pipe_fd_[1]);
        while(is_run_) {
            int event_cnt = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
            for (int i = 0; i < event_cnt; ++i) {
                memset(recv_buffer_, 0, sizeof(recv_buffer_));
                int socket_fd = events[i].data.fd;
                if (socket_fd == client_fd_) {
                    ReceiveMessage();
                } else {
                    ReadPipe(events[i].data.fd);
                }
            }
        }
    }
    Close();
}

void TCPClient::ReceiveMessage() {
    int result = recv(client_fd_, recv_buffer_, sizeof(recv_buffer_), 0);
    memset(&msg_, 0, sizeof(msg_));
    memcpy(&msg_, recv_buffer_, sizeof(msg_));
    if (result == 0) {
        std::cout << "Connection closed" << std::endl;
        close(client_fd_);
        is_run_ = false;
    } else {
        if (msg_.type == MessageType::PRIVATE) {
            std::cout << "*private* [" << msg_.src << "]: " << msg_.message << std::endl;
        } else if (msg_.type == MessageType::PUBLIC) {
            std::cout << '[' << msg_.src << "]: " << msg_.message << std::endl;
        } else if (msg_.type == MessageType::SERVER) {
            std::cout << msg_.message << std::endl;
        } else {
            std::cout << "Received error message type" << std::endl;
        }
    }
}


void TCPClient::ReadPipe(int pipe_fd) {
    int result = read(pipe_fd, send_buffer_, sizeof(send_buffer_));
    if (result == 0) {
        is_run_ = false;
    } else {
        if (send(client_fd_, send_buffer_, sizeof(send_buffer_), 0) < 0) {
            std::cerr << "Send failed" << std::endl;
        }
    }
}

void TCPClient::Close() {
    if (pid_) {
        close(pipe_fd_[0]);
        close(client_fd_);
    } else {
        close(pipe_fd_[1]);
    }
}

bool TCPClient::ConstructMessage(const std::string &input) {
    if (input.size() == 0) {
        return false;
    }
    if (input[0] == '/') {
        if (input.compare("/exit") == 0) {
            msg_.type = MessageType::EXIT;
            return false;
        }
        std::cout << "Invalid expression" << std::endl;
        return false;
    }
    if (input[0] == '@') {
        if (input.size() >= 2 && input[1] >= '0' && input[1] <= '9') {
            size_t p = input.find(' ');
            if (p != std::string::npos) {
                int dst = std::stoi(input.substr(1, p));
                if (dst == client_id_) {
                    std::cout << "Invalid user" << std::endl;
                    return false;
                }
                msg_.type = MessageType::PRIVATE;
                msg_.src = client_id_;
                msg_.dst = dst;
                
                strcpy(msg_.message, input.substr(p, p + BUFFER_SIZE).c_str());
                return true;
            }
        }
        std::cout << "Invalid expression" << std::endl;
        return false;
    } else {
        msg_.type = MessageType::PUBLIC;
        msg_.src = client_id_;
        msg_.dst = -1;
        strcpy(msg_.message, input.c_str());
        return true;
    }
}