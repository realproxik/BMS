#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace BMS {
namespace IPC {

class IPCManager {
public:
    using MessageCallback = std::function<void(const std::string&)>;

    IPCManager();
    ~IPCManager();

    bool startServer(int minPort = 40000, int maxPort = 41000);
    void stopServer();

    bool isRunning() const;
    bool isConnected() const;

    bool sendMessage(const std::string& message);
    void setMessageCallback(MessageCallback callback);

    int getPort() const;

private:
    void serverLoop();
    bool openListener(int port);
    void closeListener();
    bool acceptConnection();
    bool receiveLoop();

    MessageCallback messageCallback_;
    std::thread serverThread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    std::mutex sendMutex_;
    int listenerPort_;

#ifdef _WIN32
    void* listenerSocket_;
    void* clientSocket_;
#else
    int listenerSocket_;
    int clientSocket_;
#endif
};

} // namespace IPC
} // namespace BMS
