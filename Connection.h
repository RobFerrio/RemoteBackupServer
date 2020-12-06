//
// Created by rob on 03/12/20.
//

#ifndef REMOTEBACKUPSERVER_CONNECTION_H
#define REMOTEBACKUPSERVER_CONNECTION_H

#include <boost/asio.hpp>
#include <iostream>
#include "Message.h"

#define HEADER_LENGTH 8

using io_context = boost::asio::io_context;
using tcp = boost::asio::ip::tcp;

class Connection: public std::enable_shared_from_this<Connection>{
    io_context& ioContext;
    tcp::socket socket;
    std::string username;

    std::string outboundHeader{};
    std::string outboundData{};
    char inboundHeader[HEADER_LENGTH]{};
    std::vector<char> inboundData{};

    Message bufferMessage;
public:
    Connection(io_context& ioContext, tcp::socket&& socket): ioContext(ioContext), socket(std::move(socket)){};

    template <typename T, typename Handler>
    void async_write(const T& t, Handler handler);

    template <typename Handler>
    void async_read(Handler handler);   //Il risultato può essere solo di tipo Message (viene salvato in bufferMessage)

    void handleConnection();
};

#endif //REMOTEBACKUPSERVER_CONNECTION_H
