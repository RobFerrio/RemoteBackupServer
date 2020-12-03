//
// Created by rob on 03/12/20.
//

#ifndef REMOTEBACKUPSERVER_CONNECTION_H
#define REMOTEBACKUPSERVER_CONNECTION_H

#include <boost/asio.hpp>
#include <iostream>

using io_context = boost::asio::io_context;
using tcp = boost::asio::ip::tcp;

class Connection: public std::enable_shared_from_this<Connection>{
    tcp::socket socket;
    boost::asio::streambuf streambuf;
    std::string username;

public:
    explicit Connection(tcp::socket&& socket): socket(std::move(socket)){};

    void handleConnection();
};

#endif //REMOTEBACKUPSERVER_CONNECTION_H
