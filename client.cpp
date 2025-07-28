// client.cpp
#include <iostream>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9090
#define BUFFER_SIZE 1024

void send_data(int id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Client " << id << " - Socket creation error\n";
        return;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Client " << id << " - Invalid address\n";
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Client " << id << " - Connection failed\n";
        return;
    }

    char buffer[BUFFER_SIZE];
    while (true) {
        const char* msg = (rand() % 2 == 0) ? "1" : "0";
        send(sock, msg, strlen(msg), 0);
        int bytes = read(sock, buffer, BUFFER_SIZE);
        if (bytes <= 0) break;
    }

    close(sock);
}

int main() {
    send_data(1);
    return 0;
}