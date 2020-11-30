//
// Created by rob on 30/11/20.
//

#include "Socket.h"
#include <unistd.h>

Socket::Socket() {
    sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
        throw std::runtime_error("Impossibile creare il socket");
    safe_cout(std::string("Socket creato: ") + std::to_string(sockfd));
}

Socket::Socket(Socket &&other) noexcept: sockfd(other.sockfd) {
    other.sockfd = 0;
}

Socket &Socket::operator=(Socket &&other)  noexcept {
    if(sockfd != 0)
        close(sockfd);
    sockfd = other.sockfd;
    other.sockfd = 0;
    return *this;
}

Socket::~Socket() {
    if(sockfd != 0){
        safe_cout(std::string("Socket chiuso: ") + std::to_string(sockfd));
        close(sockfd);
    }
}

ssize_t Socket::read(char *buffer, size_t len, int options) {
    ssize_t res = recv(sockfd, buffer, len, options);
    if(res < 0)
        throw std::runtime_error(std::string("Impossibile ricevere dal socket ") + std::to_string(sockfd));
    return res;
}

ssize_t Socket::write(char *buffer, size_t len, int options) {
    ssize_t res = send(sockfd, buffer, len, options);
    if(res < 0)
        throw std::runtime_error(std::string("Impossibile inviare sul socket ") + std::to_string(sockfd));
    return res;
}

void Socket::connect(struct sockaddr_in *addr, unsigned int len) {
    if(::connect(sockfd, reinterpret_cast<struct sockaddr*>(addr), len) != 0)
        throw std::runtime_error("Impossibile effettuare la connect");
}

void Socket::shutdown(int how) {
    if(sockfd != 0)
        ::shutdown(sockfd, how);
}