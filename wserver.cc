#include <iostream>
#include "networking/tcp_server.h"

int main() {
    net::TCPServer server;
    server.Start(8090);
    return 0;
}