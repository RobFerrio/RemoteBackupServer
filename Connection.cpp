//
// Created by rob on 03/12/20.
//

#include "Connection.h"

void Connection::handleConnection() {
    boost::asio::async_write(socket, boost::asio::buffer("Inserisci username: "), [self = shared_from_this()](boost::system::error_code error, std::size_t bytes_transferred){
        if(!error)
            boost::asio::async_read_until(self->socket, self->streambuf, '\n', [self] (boost::system::error_code error, std::size_t bytes_transferred)
            {
                std::cout << std::istream(&self->streambuf).rdbuf(); // Da mettere riconoscimento utente e invio cartella
            });
    });
}