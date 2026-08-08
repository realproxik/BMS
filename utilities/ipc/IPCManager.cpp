#include "IPCManager.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace BMS {
namespace IPC {

namespace {
    void initializeSockets() {
#ifdef _WIN32
        static bool initialized = false;
        if (!initialized) {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            initialized = true;
        }
#endif
    }

    void cleanupSockets() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

IPCManager::IPCManager()
    : running_(false), connected_(false), listenerPort_(0)
{
#ifdef _WIN32
    listenerSocket_ = INVALID_SOCKET;
    clientSocket_ = INVALID_SOCKET;
#else
    listenerSocket_ = -1;
    clientSocket_ = -1;
#endif
}

IPCManager::~IPCManager() {
    stopServer();
    cleanupSockets();
}

bool IPCManager::startServer(int minPort, int maxPort) {
    if (running_) return false;
    initializeSockets();
    running_ = true;
    serverThread_ = std::thread(&IPCManager::serverLoop, this);
    return true;
}

void IPCManager::stopServer() {
    running_ = false;
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    closeListener();
}

bool IPCManager::isRunning() const {
    return running_;
}

bool IPCManager::isConnected() const {
    return connected_;
}

int IPCManager::getPort() const {
    return listenerPort_;
}

void IPCManager::setMessageCallback(MessageCallback callback) {
    messageCallback_ = std::move(callback);
}

bool IPCManager::sendMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!connected_) return false;

#ifdef _WIN32
    int result = send(static_cast<SOCKET>(clientSocket_), message.c_str(), static_cast<int>(message.size()), 0);
#else
    int result = ::send(clientSocket_, message.c_str(), static_cast<int>(message.size()), 0);
#endif
    return result == static_cast<int>(message.size());
}

void IPCManager::serverLoop() {
    int port = 40000;
    for (; port < 41000 && running_; ++port) {
        if (openListener(port)) {
            listenerPort_ = port;
            break;
        }
    }

    if (listenerPort_ == 0) {
        std::cerr << "[IPC] Failed to open listener" << std::endl;
        running_ = false;
        return;
    }

    if (!acceptConnection()) {
        running_ = false;
        return;
    }

    receiveLoop();
    closeListener();
}

bool IPCManager::openListener(int port) {
#ifdef _WIN32
    listenerSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenerSocket_ == INVALID_SOCKET) return false;
#else
    listenerSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenerSocket_ < 0) return false;
    int flags = fcntl(listenerSocket_, F_GETFL, 0);
    fcntl(listenerSocket_, F_SETFL, flags | O_NONBLOCK);
#endif

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    int result = bind(listenerSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (result != 0) {
        closeListener();
        return false;
    }
    if (listen(listenerSocket_, 1) != 0) {
        closeListener();
        return false;
    }
    return true;
}

void IPCManager::closeListener() {
#ifdef _WIN32
    if (clientSocket_ != INVALID_SOCKET) {
        closesocket(static_cast<SOCKET>(clientSocket_));
        clientSocket_ = INVALID_SOCKET;
    }
    if (listenerSocket_ != INVALID_SOCKET) {
        closesocket(static_cast<SOCKET>(listenerSocket_));
        listenerSocket_ = INVALID_SOCKET;
    }
#else
    if (clientSocket_ >= 0) {
        close(clientSocket_);
        clientSocket_ = -1;
    }
    if (listenerSocket_ >= 0) {
        close(listenerSocket_);
        listenerSocket_ = -1;
    }
#endif
    connected_ = false;
}

bool IPCManager::acceptConnection() {
    std::cout << "[IPC] Waiting for Node process to connect on port " << listenerPort_ << std::endl;
    sockaddr_in clientAddr = {};
#ifdef _WIN32
    int addrLen = sizeof(clientAddr);
    clientSocket_ = accept(static_cast<SOCKET>(listenerSocket_), reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientSocket_ == INVALID_SOCKET) {
        std::cerr << "[IPC] Accept failed" << std::endl;
        return false;
    }
#else
    socklen_t addrLen = sizeof(clientAddr);
    clientSocket_ = accept(listenerSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientSocket_ < 0) {
        std::cerr << "[IPC] Accept failed" << std::endl;
        return false;
    }
#endif
    connected_ = true;
    std::cout << "[IPC] Connected to Node process" << std::endl;
    return true;
}

bool IPCManager::receiveLoop() {
    std::string buffer;
    buffer.reserve(4096);
    char temp[1024];

    while (running_ && connected_) {
#ifdef _WIN32
        int bytes = recv(static_cast<SOCKET>(clientSocket_), temp, sizeof(temp), 0);
#else
        int bytes = recv(clientSocket_, temp, sizeof(temp), 0);
#endif
        if (bytes <= 0) {
            connected_ = false;
            break;
        }
        buffer.append(temp, bytes);
        size_t pos = 0;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (messageCallback_) {
                messageCallback_(line);
            }
        }
    }
    return false;
}

} // namespace IPC
} // namespace BMS
