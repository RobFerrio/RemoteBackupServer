//
// Created by rob on 30/11/20.
//

#ifndef REMOTEBACKUPSERVER_SOCKET_H
#define REMOTEBACKUPSERVER_SOCKET_H


#include <mutex>
#include <iostream>
#include <netinet/in.h>

static std::mutex coutMutex;
static void safe_cout(std::string msg){
    std::lock_guard<std::mutex> lg(coutMutex);
    std::cout << msg << std::endl;
}

class Socket {
    int sockfd;
    Socket(const Socket&) = delete;
    Socket& operator= (const Socket&) = delete;
    explicit Socket(int sockfd): sockfd(sockfd){
        safe_cout(std::string("Socket creato via fd: ") + std::to_string(sockfd));
    };
    friend class ServerSocket;
public:
    Socket();
    Socket(Socket&& other) noexcept ;
    Socket& operator= (Socket&& other) noexcept ;
    ~Socket();
    ssize_t read(char* buffer, size_t len, int options);
    ssize_t write(char* buffer, size_t len, int options);
    void connect(struct sockaddr_in* addr, unsigned int len);
    void shutdown(int how);
};

#endif //REMOTEBACKUPSERVER_SOCKET_H
