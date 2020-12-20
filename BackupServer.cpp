//
// Created by rob on 02/12/20.
//

#include "BackupServer.h"

BackupServer::BackupServer(io_context& ioContext, uint16_t port): ioContext(ioContext),
                                                                  acceptor(ioContext, tcp::endpoint(tcp::v4(), port))
{
    acceptConnection();
}

void BackupServer::acceptConnection() {
    connSocket.emplace(ioContext);  //All'inizio creavo direttamente la Connection dal context, ma andava in segfault quando scattava la prima async (probabilmente il socket o la Connection venivano distrutte)
    acceptor.async_accept(*connSocket, [&](boost::system::error_code error){
        if(!error) {
            connSocket->set_option(boost::asio::socket_base::keep_alive(true));
            std::make_shared<Connection>(ioContext, std::move(*connSocket))->handleConnection();
        }
        acceptConnection();
    });
}