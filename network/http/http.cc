// network/http_request.cpp
#include "http_request.h"
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace BMS {

HTTPRequest::HTTPRequest() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HTTPRequest::~HTTPRequest() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void HTTPRequest::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

std::string HTTPRequest::buildRequest() const {
    std::stringstream ss;
    
    // Method and path
    std::string methodStr;
    switch (method_) {
        case Method::GET: methodStr = "GET"; break;
        case Method::POST: methodStr = "POST"; break;
        case Method::PUT: methodStr = "PUT"; break;
        case Method::DELETE: methodStr = "DELETE"; break;
    }
    
    // Parse URL
    std::string host = url_;
    std::string path = "/";
    size_t protocolPos = url_.find("://");
    if (protocolPos != std::string::npos) {
        host = url_.substr(protocolPos + 3);
    }
    size_t pathPos = host.find('/');
    if (pathPos != std::string::npos) {
        path = host.substr(pathPos);
        host = host.substr(0, pathPos);
    }
    
    ss << methodStr << " " << path << " HTTP/1.1\r\n";
    ss << "Host: " << host << "\r\n";
    
    // Add default headers if not set
    if (headers_.find("User-Agent") == headers_.end()) {
        ss << "User-Agent: BMS-Browser/1.0\r\n";
    }
    if (headers_.find("Accept") == headers_.end()) {
        ss << "Accept: */*\r\n";
    }
    if (headers_.find("Connection") == headers_.end()) {
        ss << "Connection: close\r\n";
    }
    if (method_ == Method::POST || method_ == Method::PUT) {
        if (headers_.find("Content-Type") == headers_.end()) {
            ss << "Content-Type: application/x-www-form-urlencoded\r\n";
        }
        if (headers_.find("Content-Length") == headers_.end()) {
            ss << "Content-Length: " << body_.length() << "\r\n";
        }
    }
    
    // Custom headers
    for (const auto& [key, value] : headers_) {
        if (key != "Host" && key != "User-Agent" && key != "Accept" && 
            key != "Connection" && key != "Content-Type" && key != "Content-Length") {
            ss << key << ": " << value << "\r\n";
        }
    }
    
    ss << "\r\n";
    if (method_ == Method::POST || method_ == Method::PUT) {
        ss << body_;
    }
    
    return ss.str();
}

HTTPRequest::Response HTTPRequest::send() {
    Response response;
    response.statusCode = 0;
    
    // Parse URL
    std::string host = url_;
    std::string path = "/";
    int port = 80;
    bool isHTTPS = false;
    
    if (url_.find("https://") == 0) {
        isHTTPS = true;
        port = 443;
        host = url_.substr(8);
    } else if (url_.find("http://") == 0) {
        host = url_.substr(7);
    }
    
    size_t pathPos = host.find('/');
    if (pathPos != std::string::npos) {
        path = host.substr(pathPos);
        host = host.substr(0, pathPos);
    }
    
    size_t portPos = host.find(':');
    if (portPos != std::string::npos) {
        port = std::stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        response.statusCode = -1;
        response.statusMessage = "Socket creation failed";
        return response;
    }
    
    // Resolve host
    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        response.statusCode = -2;
        response.statusMessage = "Host resolution failed";
        return response;
    }
    
    // Connect
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        response.statusCode = -3;
        response.statusMessage = "Connection failed";
        return response;
    }
    
    // Send request
    std::string request = buildRequest();
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        response.statusCode = -4;
        response.statusMessage = "Send failed";
        return response;
    }
    
    // Receive response
    char buffer[4096];
    std::string fullResponse;
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        fullResponse += buffer;
    }
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    // Parse response
    size_t headerEnd = fullResponse.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headers = fullResponse.substr(0, headerEnd);
        response.body = fullResponse.substr(headerEnd + 4);
        
        // Parse status line
        size_t firstLineEnd = headers.find("\r\n");
        if (firstLineEnd != std::string::npos) {
            std::string statusLine = headers.substr(0, firstLineEnd);
            std::stringstream ss(statusLine);
            std::string httpVersion;
            ss >> httpVersion >> response.statusCode;
            std::getline(ss, response.statusMessage);
        }
        
        // Parse headers
        size_t pos = firstLineEnd + 2;
        while (pos < headers.length()) {
            size_t lineEnd = headers.find("\r\n", pos);
            if (lineEnd == std::string::npos) break;
            
            std::string line = headers.substr(pos, lineEnd - pos);
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 2);
                response.headers[key] = value;
            }
            
            pos = lineEnd + 2;
        }
    }
    
    return response;
}

void HTTPRequest::sendAsync(std::function<void(const Response&)> callback) {
    callback_ = callback;
    // In a real implementation, this would use a thread pool
    Response response = send();
    if (callback_) {
        callback_(response);
    }
}

} // namespace BMS