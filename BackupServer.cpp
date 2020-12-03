//
// Created by rob on 02/12/20.
//

#include "BackupServer.h"
#include "boost/bind/bind.hpp"

BackupServer::BackupServer(io_context& ioContext, uint16_t port): ioContext(ioContext),
                                                                  acceptor(ioContext, tcp::endpoint(tcp::v4(), port))
{
    acceptConnection();
}

void BackupServer::acceptConnection() {
    connSocket.emplace(ioContext);  //All'inizio creavo direttamente la Connection dal context, ma andava in segfault quando scattava la prima async (probabilmente il socket o la Connection venivano distrutte)
    acceptor.async_accept(*connSocket, [&](boost::system::error_code error){
        if(!error) {
            std::make_shared<Connection>(std::move(*connSocket))->handleConnection();
        }
        acceptConnection();
    });
}