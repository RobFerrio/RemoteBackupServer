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

#define ERROR_MSG  -1;
#define AUTH_MSG    0;
#define ENDINIT_MSG 1;
#define PROBE_MSG   2;
#define FILE_MSG    3;

class Message {
    int type;
    std::vector<char> data{};
    size_t size;
    unsigned char hash[SHA256_DIGEST_LENGTH]{};

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version){
        ar & type;      //Se Archive è un output archive allora & è uguale a <<
        ar & data;      //Se Archive è un input  archive allora & è uguale a >>
        ar & size;
        ar & hash;
    };     //Funzione di supporto per gli archive di boost

    friend class boost::serialization::access;
public:
    Message();
    explicit Message(int type);
    Message(int type, std::vector<char> data, size_t size);

    int getType() const;
    std::vector<char> getData() const;
    size_t getSize() const;

    bool checkHash() const;
};


#endif //REMOTEBACKUPSERVER_MESSAGE_H
