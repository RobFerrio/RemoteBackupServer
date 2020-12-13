//
// Created by rob on 04/12/20.
//

#include <cstring>
#include <utility>
#include "Message.h"

Message::Message(): type(ERROR_MSG){}

Message::Message(int type): type(type){}

Message::Message(int type, std::vector<char> data): type(type), data(std::move(data)) {
    hashData();
}

int Message::getType() const {
    return type;
}

std::vector<char> Message::getData() const {
    return data;
}

void Message::hashData() {
    unsigned char dataHash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, this->data.data(), this->data.size());
    SHA256_Final(dataHash, &sha256);

    hash = std::string(reinterpret_cast<char *>(dataHash), SHA256_DIGEST_LENGTH);
}

bool Message::checkHash() const{
    if(this->data.empty()) return false;

    unsigned char hashRecomputed[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, this->data.data(), this->data.size());
    SHA256_Final(hashRecomputed, &sha256);

    return hash == std::string(reinterpret_cast<char *>(hashRecomputed), SHA256_DIGEST_LENGTH);
}

std::optional<std::pair<std::string, std::string>> Message::extractAuthData() {
    if(this->type != AUTH_RES)
        return std::optional< std::pair<std::string, std::string> >();

    std::string codedAuthData(this->data.begin(), this->data.end());

    size_t pos = 0;
    std::string username;
    std::string password;

    //Estrazione USERNAME
    pos = codedAuthData.find(UDEL);
    if(pos == std::string::npos)
        return std::optional< std::pair<std::string, std::string> >();
    username = codedAuthData.substr(0, pos);
    codedAuthData.erase(0, pos + sizeof(UDEL) + username.length());

    //Estrazione PASSWORD
    pos = codedAuthData.find(PDEL);
    if(pos == std::string::npos)
        return std::optional< std::pair<std::string, std::string> >();
    password = codedAuthData.substr(0,  pos);
    codedAuthData.erase(0, pos + sizeof(PDEL) + password.length());

    return std::pair(username, password);
}

std::optional<std::unordered_map<std::string, std::string>> Message::extractFileList() {
    if(this->type != FILE_LIST)
        return std::optional< std::unordered_map<std::string, std::string> >();

    std::unordered_map<std::string, std::string> decodedFileList;
    std::string codedFileList(this->data.begin(), this->data.end());

    size_t pos;
    std::string file;
    std::string hash_;
    while ((pos = codedFileList.find(FDEL)) != std::string::npos) {
        //Estrazione FILE
        file = codedFileList.substr(0, pos);
        codedFileList.erase(0, pos + sizeof(FDEL) + file.length());

        //Estrazione HASH
        pos = codedFileList.find(HDEL);
        hash_ = codedFileList.substr(0,  pos);
        decodedFileList[file] = hash_;
        codedFileList.erase(0, pos + sizeof(HDEL) + hash_.length());
    }

    return decodedFileList;
}
