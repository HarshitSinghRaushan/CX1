// monitor.cpp
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <map>
#include <regex>
#include <iomanip>
#include <ctime>

#define GREEN   "\033[1;32m"
#define RED     "\033[1;31m"
#define YELLOW  "\033[1;33m"
#define RESET   "\033[0m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"

struct NodeStats {
    std::string status;
    int messages = 0;
    size_t bytes = 0;
    double avg_latency = 0.0;
    double avg_bandwidth = 0.0;
    time_t last_seen = 0;
    double last_latency = 0.0;
    double last_bandwidth = 0.0;
};

void print_stats(int clients, int messages, size_t bytes, double bandwidth_kbps, double latency_ms) {
    std::cout << "\033[2J\033[1;1H"; // Clear terminal
    std::cout << CYAN << "📊 Server Stress Test Monitor\n";
    std::cout << "-----------------------------\n" << RESET;
    std::cout << "Active Clients     : " << clients << "\n";
    std::cout << "Total Messages     : " << messages << "\n";
    std::cout << "Total Bytes        : " << bytes << "\n";
    std::cout << "Bandwidth          : " << bandwidth_kbps << " kbps\n";
    std::cout << "Latency Estimate   : " << latency_ms << " ms/message\n";
    std::cout << "-----------------------------\n";
}

std::string color_for_status(const std::string& status) {
    if (status == "Active") return GREEN + status + RESET;
    if (status == "Disconnected") return RED + status + RESET;
    return YELLOW + status + RESET;
}

std::string analyze_health(NodeStats& node, double latency, double bandwidth) {
    if (node.avg_latency == 0) {
        node.avg_latency = latency;
        node.avg_bandwidth = bandwidth;
        return "Healthy";
    }

    node.avg_latency = (node.avg_latency * 0.9) + (latency * 0.1);
    node.avg_bandwidth = (node.avg_bandwidth * 0.9) + (bandwidth * 0.1);
    node.last_latency = latency;
    node.last_bandwidth = bandwidth;

    if ((latency > node.avg_latency * 1.5) || (bandwidth < node.avg_bandwidth * 0.5)) {
        return "⚠️ Sluggish";
    }

    return "Healthy";
}

std::string draw_bar(double value, double max_value, int width = 30) {
    int filled = static_cast<int>((value / max_value) * width);
    if (filled > width) filled = width;
    return std::string(filled, '|') + std::string(width - filled, '.');
}

int main() {
    int prev_messages = 0;
    size_t prev_bytes = 0;
    std::map<std::string, NodeStats> nodes;

    while (true) {
        std::ifstream file("metrics.log");
        int clients = 0, messages = 0;
        size_t bytes = 0;
        std::map<std::string, std::string> current_status;

        if (file.is_open()) {
            std::string line;
            bool first_line = true;
            while (std::getline(file, line)) {
                if (first_line) {
                    std::istringstream ss(line);
                    ss >> clients >> messages >> bytes;
                    first_line = false;
                } else {
                    std::regex node_regex(R"(Node(\d+):(.*))");
                    std::smatch match;
                    if (std::regex_match(line, match, node_regex)) {
                        std::string node_id = "Node" + match[1].str();
                        std::string status = match[2].str();
                        current_status[node_id] = status;

                        if (!nodes.count(node_id)) {
                            nodes[node_id] = NodeStats();
                        }

                        nodes[node_id].status = status;
                        nodes[node_id].last_seen = std::time(nullptr);
                    }
                }
            }

            int delta_msgs = messages - prev_messages;
            size_t delta_bytes = bytes - prev_bytes;

            double bandwidth_kbps = (delta_bytes * 8.0) / 1024.0;
            double latency_ms = (delta_msgs > 0) ? (1000.0 / delta_msgs) : 0.0;

            print_stats(clients, messages, bytes, bandwidth_kbps, latency_ms);

            std::cout << "\n🔧 Node Health Status:\n";
            for (auto& [node_id, stats] : nodes) {
                std::string perf_status = "N/A";
                if (stats.status == "Active") {
                    perf_status = analyze_health(stats, latency_ms, bandwidth_kbps);
                }

                std::cout << node_id << " : "
                          << color_for_status(stats.status) << "  →  "
                          << ((perf_status == "⚠️ Sluggish") ? RED : GREEN)
                          << perf_status << RESET << "\n";
            }

            std::cout << "\n📈 Graph View (per node):\n";

            for (auto& [node_id, stats] : nodes) {
                if (stats.status != "Active") continue;

                std::cout << BLUE << node_id << RESET << "\n";

                std::cout << "  Bandwidth  [kbps]: "
                          << draw_bar(stats.last_bandwidth, 1000)  // scale max to 1000 kbps
                          << " " << std::fixed << std::setprecision(1) << stats.last_bandwidth << " kbps\n";

                std::cout << "  Latency    [ms]  : "
                          << draw_bar(stats.last_latency, 100)  // scale max to 100 ms
                          << " " << std::fixed << std::setprecision(1) << stats.last_latency << " ms\n";
            }

            prev_messages = messages;
            prev_bytes = bytes;
        } else {
            std::cerr << RED << "⚠️  Unable to read metrics.log\n" << RESET;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
