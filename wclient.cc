#include <iostream>
#include "networking/tcp_client.h"

int main() {
    net::TCPClient client;
    if (!client.Connect("127.0.0.1", 8090)) {
        return 0;
    }
    client.Start();
    return 0;
}