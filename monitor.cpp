// monitor.cpp
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>

void print_stats(int clients, int messages, size_t bytes, double bandwidth_kbps, double latency_ms) {
    std::cout << "\033[2J\033[1;1H"; // Clear terminal
    std::cout << "📊 Server Stress Test Monitor\n";
    std::cout << "-----------------------------\n";
    std::cout << "Active Clients     : " << clients << "\n";
    std::cout << "Total Messages     : " << messages << "\n";
    std::cout << "Total Bytes        : " << bytes << "\n";
    std::cout << "Bandwidth          : " << bandwidth_kbps << " kbps\n";
    std::cout << "Latency Estimate   : " << latency_ms << " ms/message\n";
    std::cout << "-----------------------------\n";
}

int main() {
    int prev_messages = 0;
    size_t prev_bytes = 0;

    while (true) {
        std::ifstream file("metrics.log");
        int clients = 0, messages = 0;
        size_t bytes = 0;

        if (file >> clients >> messages >> bytes) {
            int delta_msgs = messages - prev_messages;
            size_t delta_bytes = bytes - prev_bytes;

            double bandwidth_kbps = (delta_bytes * 8.0) / 1024.0;
            double latency_ms = (delta_msgs > 0) ? (1000.0 / delta_msgs) : 0.0;

            print_stats(clients, messages, bytes, bandwidth_kbps, latency_ms);

            prev_messages = messages;
            prev_bytes = bytes;
        } else {
            std::cerr << "⚠️  Unable to read metrics.log\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
