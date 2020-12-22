//
// Created by rob on 02/12/20.
//

#ifndef REMOTEBACKUPSERVER_BACKUPSERVER_H
#define REMOTEBACKUPSERVER_BACKUPSERVER_H

#include "Connection.h"

class BackupServer {
    io_context& ioContext;
    ssl_context sslContext;
    tcp::acceptor acceptor;
    std::optional<tcp::socket> connSocket;

public:
    BackupServer(io_context& ioContext, std::uint16_t port);
    void acceptConnection();
};

#endif //REMOTEBACKUPSERVER_BACKUPSERVER_H
