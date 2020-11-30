//
// Created by rob on 30/11/20.
//

#include "ServerSocket.h"

ServerSocket::ServerSocket(uint16_t portNumber) {
    struct sockaddr_in sockaddrIn;
    sockaddrIn.sin_port = htons(portNumber);
    sockaddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddrIn.sin_family = AF_INET;
    if(::bind(sockfd, reinterpret_cast<struct sockaddr*>(&sockaddrIn), sizeof(sockaddrIn)) != 0)
        throw std::runtime_error("Impossibile effettuare il bind");
    if(::listen(sockfd, 10) != 0)
        throw std::runtime_error("Impossibile effettuare la listen");
}

Socket ServerSocket::accept(struct sockaddr_in *addr, socklen_t len) {
    int fd = ::accept(sockfd, reinterpret_cast<struct sockaddr*>(addr), &len);
    if(fd < 0)
        throw std::runtime_error("Impossibile effettuare la accept");
    return Socket(fd);
}
