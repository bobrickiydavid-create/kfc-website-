#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string load_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "<h1>404 Not Found</h1>";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main() {
    const int PORT = 8080;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(server_fd, 16) < 0) { perror("listen"); return 1; }

    std::cout << "Server running on http://localhost:" << PORT << "\n";

    while (true) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int client_fd = accept(server_fd, (sockaddr*)&client, &len);
        if (client_fd < 0) { perror("accept"); continue; }

        char buffer[2048];
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n <= 0) { close(client_fd); continue; }
        buffer[n] = '\0';

        std::string body = load_file("index.html");
        std::ostringstream resp;
        resp << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html; charset=UTF-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;

        std::string out = resp.str();
        ssize_t sent = 0;
        while (sent < (ssize_t)out.size()) {
            ssize_t m = write(client_fd, out.data() + sent, out.size() - sent);
            if (m <= 0) break;
            sent += m;
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
