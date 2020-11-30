//
// Created by rob on 30/11/20.
//

#ifndef REMOTEBACKUPSERVER_SERVERSOCKET_H
#define REMOTEBACKUPSERVER_SERVERSOCKET_H

#include "Socket.h"

class ServerSocket: private Socket { //Ereditando in modo privato, i metodi di Socket non possono essere usati esternamente al ServerSocket (ci serve ereditare solo per demandare la creazione alla superclasse)
public:
    explicit ServerSocket(uint16_t portNumber);
    Socket accept(struct sockaddr_in *addr, socklen_t len);
};


#endif //REMOTEBACKUPSERVER_SERVERSOCKET_H
