// http_server.cpp (Live-updating webpage + API)
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>

#define PORT 8080

std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return "File not found";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string read_metrics() {
    return read_file("metrics.log");
}

std::string read_nodes() {
    std::ifstream file("metrics.log");
    std::ostringstream nodes;
    std::string line;
    bool skip = true;
    while (std::getline(file, line)) {
        if (skip) { skip = false; continue; }
        nodes << line << "\n";
    }
    return nodes.str();
}

std::string ascii_graph() {
    std::ifstream file("metrics.log");
    std::ostringstream out;
    std::string line;
    bool first = true;
    int clients, messages;
    size_t bytes;
    std::map<std::string, std::string> nodes;

    while (std::getline(file, line)) {
        if (first) {
            std::istringstream ss(line);
            ss >> clients >> messages >> bytes;
            first = false;
        } else {
            auto colon = line.find(":");
            if (colon != std::string::npos) {
                std::string node = line.substr(0, colon);
                std::string stat = line.substr(colon + 1);
                nodes[node] = stat;
            }
        }
    }

    out << "Graph View:\n-----------\n";
    for (auto& [node, stat] : nodes) {
        int bar = rand() % 20;
        out << node << " [" << stat << "]: " << std::string(bar, '|') << "\n";
    }

    return out.str();
}

void serve(int client_socket) {
    char buffer[4096] = {0};
    read(client_socket, buffer, 4096);

    std::string req(buffer);
    std::string content;
    std::string type = "text/plain";

    if (req.find("GET / ") != std::string::npos || req.find("GET /index.html") != std::string::npos) {
        content = read_file("index.html");
        type = "text/html";
    } else if (req.find("GET /metrics") != std::string::npos) {
        content = read_metrics();
    } else if (req.find("GET /nodes") != std::string::npos) {
        content = read_nodes();
    } else if (req.find("GET /graph") != std::string::npos) {
        content = ascii_graph();
    } else {
        content = "404 Not Found";
    }

    std::ostringstream res;
    res << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << type << "\r\n"
        << "Content-Length: " << content.length() << "\r\n\r\n"
        << content;

    send(client_socket, res.str().c_str(), res.str().size(), 0);
    close(client_socket);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "🌍 Web Dashboard Live at: http://<your_ip>:8080/\n";

    while (true) {
        client_sock = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_sock >= 0)
            std::thread(serve, client_sock).detach();
    }

    close(server_fd);
    return 0;
}
