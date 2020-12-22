//
// Created by rob on 02/12/20.
//

#include "BackupServer.h"

BackupServer::BackupServer(io_context& ioContext, uint16_t port): ioContext(ioContext),
                                                                  sslContext(boost::asio::ssl::context::tlsv12_server),
                                                                  acceptor(ioContext, tcp::endpoint(tcp::v4(), port))
{
    sslContext.set_options(boost::asio::ssl::context::single_dh_use);
    sslContext.use_certificate_chain_file("../sec_files/user.crt");
    sslContext.use_private_key_file("../sec_files/user.key", boost::asio::ssl::context::pem);
    sslContext.use_tmp_dh_file("../sec_files/dh2048.pem");
    acceptConnection();
}

void BackupServer::acceptConnection() {
    connSocket.emplace(ioContext);  //All'inizio creavo direttamente la Connection dal context, ma andava in segfault quando scattava la prima async (probabilmente il socket o la Connection venivano distrutte)
    acceptor.async_accept(*connSocket, [&](boost::system::error_code error){
        if(!error) {
            connSocket->set_option(boost::asio::socket_base::keep_alive(true));
            std::make_shared<Connection>(ioContext, sslContext, std::move(*connSocket))->doHandshake();
        }
        acceptConnection();
    });
}