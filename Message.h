//
// Created by rob on 04/12/20.
//

#ifndef REMOTEBACKUPSERVER_MESSAGE_H
#define REMOTEBACKUPSERVER_MESSAGE_H

#include <vector>
#include <openssl/sha.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>
#include "SafeCout.h"

//Tipi messaggio
#define ERROR_MSG   -100
#define FILE_ERR   -2
#define AUTH_ERR   -1
#define AUTH_REQ    0
#define AUTH_RES    1
#define AUTH_OK     2
#define FILE_LIST   3
#define DIR_SEND    4
#define FILE_START  5
#define FILE_DATA   6
#define FILE_END    7

//Delimitatori
#define UDEL "/:USERNAME/"
#define PDEL "/:PASSWORD/"
#define FDEL "/:FILE/"
#define HDEL "/:HASH/"

class Message {
    int type;
    std::vector<char> data{};
    std::string hash;

    void hashData();

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version){
        ar & type;      //Se Archive è un output archive allora & è uguale a <<
        ar & data;      //Se Archive è un input  archive allora & è uguale a >>
        ar & hash;
    };     //Funzione di supporto per gli archive di boost

    friend class boost::serialization::access;
public:
    Message();
    explicit Message(int type);
    Message(int type, std::vector<char> data);
    Message(int type, std::string data);
    explicit Message(const std::unordered_map<std::string, std::string>& paths);    //Costruttore per mappa <file/directory, hash>

    [[nodiscard]] int getType() const;
    [[nodiscard]] std::vector<char>& getData();

    [[nodiscard]] int checkHash() const;
    std::optional<std::pair<std::string, std::string>> extractAuthData();
    std::optional<std::unordered_map<std::string, std::string>> extractFileList();
};


#endif //REMOTEBACKUPSERVER_MESSAGE_H
