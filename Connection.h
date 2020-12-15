//
// Created by rob on 03/12/20.
//

#ifndef REMOTEBACKUPSERVER_CONNECTION_H
#define REMOTEBACKUPSERVER_CONNECTION_H

#include <boost/asio.hpp>
#include <iostream>
#include "Message.h"
#include "Folder.h"

#define HEADER_LENGTH 8
#define CHUNK_SIZE 1024

using io_context = boost::asio::io_context;
using tcp = boost::asio::ip::tcp;

class Connection: public std::enable_shared_from_this<Connection>{
    io_context& ioContext;
    tcp::socket socket;
    std::string username;
    Folder folder;

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
    void listenMessages();
    void handleFileList();
    void handleDiffs(std::shared_ptr<std::unordered_map<std::string, int>> diffs);
    void handleFileRecv(std::string path);
    void sendFile(std::shared_ptr<std::ifstream> ifs, std::shared_ptr<std::unordered_map<std::string, int>> diffs);
};

#endif //REMOTEBACKUPSERVER_CONNECTION_H
