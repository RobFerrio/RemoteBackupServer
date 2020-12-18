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

Message::Message(int type, std::string data): type(type), data(std::vector<char>(data.begin(), data.end())) {
    hashData();
}

//Costruttore per mappa <file/directory, hash>
Message::Message(const std::map<std::string, std::string>& fileList) {
    this->type = FILE_LIST;
    std::string tmp;

    for (const auto & file : fileList) {
        tmp += file.first + FDEL;
        if(!file.second.empty())  tmp+=(file.second);
        tmp += HDEL;
    }
    this->data = std::vector<char>(tmp.begin(), tmp.end());

    hashData();
}

int Message::getType() const {
    return type;
}

std::vector<char>& Message::getData() {
    return data;
}

void Message::hashData() {
    if(this->data.empty())
        return;

    unsigned char tmp[SHA256_DIGEST_LENGTH];

    //Calcolo hash
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, this->data.data(), this->data.size());
    SHA256_Final(tmp, &sha256);

    //Conversione da unsigned char a string (con un reinterpret cast esplode perchè alcuni caratteri non sono accettati da std::string)
    char hash_[2*SHA256_DIGEST_LENGTH+1];
    hash_[2*SHA256_DIGEST_LENGTH] = 0;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(hash_+i*2, "%02x", tmp[i]);

    this->hash = std::string(hash_);
}

int Message::checkHash() const{
    if(this->data.empty() || this->hash.empty())
        return -1;

    Message tmp(ERROR_MSG, this->data);
    if(this->hash == tmp.hash)
        return 1;

    return 0;
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
    username = codedAuthData.substr(0, pos);
    codedAuthData.erase(0,pos + sizeof(UDEL) - 1);

    //Estrazione PASSWORD
    pos = codedAuthData.find(PDEL);
    password = codedAuthData.substr(0,  pos);
    codedAuthData.erase(0, pos + sizeof(PDEL) - 1);

    return std::pair(username, password);
}

std::optional<std::map<std::string, std::string>> Message::extractFileList() {
    if(this->type != FILE_LIST)
        return std::optional< std::map<std::string, std::string> >();

    std::map<std::string, std::string> decodedFileList;
    std::string codedFileList(this->data.begin(), this->data.end());

    size_t pos;
    std::string file;
    std::string hash_;
    while ((pos = codedFileList.find(FDEL)) != std::string::npos) {
        //Estrazione FILE
        file = codedFileList.substr(0, pos);
        codedFileList.erase(0, pos + sizeof(FDEL) - 1);

        //Estrazione HASH
        pos = codedFileList.find(HDEL);
        hash_ = codedFileList.substr(0,  pos);
        codedFileList.erase(0, pos + sizeof(HDEL) - 1);
        if(!hash_.empty()) {
            decodedFileList[file] = hash_;
        } else decodedFileList[file] = "";
    }

    return decodedFileList;
}
