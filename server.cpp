// server.cpp
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <map>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 9090
#define BUFFER_SIZE 1024

std::atomic<int> total_clients(0);
std::atomic<int> total_messages(0);
std::atomic<size_t> total_bytes_received(0);
std::atomic<int> next_node_id(1);

std::mutex file_mutex;
std::mutex node_mutex;
std::map<int, std::string> node_health;

void log_metrics() {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ofstream log("metrics.log", std::ios::trunc);
    if (log.is_open()) {
        log << total_clients << " "
            << total_messages << " "
            << total_bytes_received << "\n";

        std::lock_guard<std::mutex> node_lock(node_mutex);
        for (const auto& [id, status] : node_health) {
            log << "Node" << id << ":" << status << "\n";
        }

        log.close();
    }
}

void handle_client(int client_socket) {
    int node_id = next_node_id++;

    {
        std::lock_guard<std::mutex> lock(node_mutex);
        node_health[node_id] = "Active";
    }

    total_clients++;
    log_metrics();

    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) break;

        {
            std::lock_guard<std::mutex> lock(node_mutex);
            node_health[node_id] = "Active";
        }

        total_messages++;
        total_bytes_received += bytes_read;
        log_metrics();

        const char* ack = "ACK";
        send(client_socket, ack, strlen(ack), 0);
    }

    close(client_socket);

    {
        std::lock_guard<std::mutex> lock(node_mutex);
        node_health[node_id] = "Disconnected";
    }

    total_clients--;
    log_metrics();
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    std::vector<std::thread> threads;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed.\n";
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed.\n";
        return 1;
    }

    std::cout << "Server running on port " << PORT << "...\n";

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket >= 0) {
            threads.emplace_back(handle_client, new_socket);
        }
    }

    for (auto& t : threads) t.join();
    close(server_fd);
    return 0;
}
