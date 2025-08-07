#include <iostream>
#include <string>
#include "json.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")
    #define CLOSESOCKET closesocket
    #define SOCKET_CLEANUP() WSACleanup()
#else
    #include <netinet/in.h>
    #include <unistd.h>
    #define SOCKET int
    #define CLOSESOCKET close
    #define SOCKET_CLEANUP()
#endif

using json = nlohmann::json;

bool initSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void handleMessage(const std::string& jsonStr) {
    try {
        auto j = json::parse(jsonStr);
        std::cout << "[RECV] " << j["symbol"] << " $" << j["price"] << " @ " << j["timestamp"] << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Failed to parse JSON\n";
    }
}

int main() 
{
    if (!initSocket())
    {
        std::cerr << "Socket init failed\n";
        return 1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERROR] Bind failed\n";
        return 1;
    }

    listen(server_fd, 1);
    SOCKET client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "[ERROR] Accept failed\n";
        return 1;
    }

    std::string buffer;
    char recv_buf[1024];

    while (true) {
        int bytes = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
        if (bytes <= 0) break;
        buffer.append(recv_buf, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            handleMessage(line);
        }
    }

    CLOSESOCKET(client_fd);
    CLOSESOCKET(server_fd);
    SOCKET_CLEANUP();

    return 0;
}

