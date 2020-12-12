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

Message::Message(const Message &m) {
    this->type = m.type;
    this->data = m.data;
    if (m.hash != nullptr)
        memcpy(this->hash, m.hash, SHA256_DIGEST_LENGTH);
}

Message::Message(Message &&src) noexcept : type(ERROR_MSG), hash(nullptr) {
    swap(*this, src);
}

Message &Message::operator=(Message src) {
    swap(*this, src);
    return *this;
}

Message &Message::operator=(Message&& src) noexcept{
    swap(*this, src);
    return *this;
}

Message::~Message() {
    delete[] hash;
}

int Message::getType() const {
    return type;
}

std::vector<char> Message::getData() const {
    return data;
}

void swap(Message &src, Message &dst) {
    std::swap(src.type, dst.type);
    std::swap(src.data, dst.data);
    std::swap(src.hash, dst.hash);
}

void Message::hashData() {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, this->data.data(), this->data.size());
    SHA256_Final(hash, &sha256);
}

bool Message::checkHash() const{
    if(this->data.empty() || this->hash == nullptr) return -1;

    unsigned char hashRecomputed[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, this->data.data(), this->data.size());
    SHA256_Final(hashRecomputed, &sha256);

    bool isEqual = (memcmp(hash, hashRecomputed, SHA256_DIGEST_LENGTH) == 0);

    return isEqual;
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
    codedAuthData.erase(0, pos + sizeof(UDEL) + username.length());

    //Estrazione PASSWORD
    pos = codedAuthData.find(PDEL);
    password = codedAuthData.substr(0,  pos);
    codedAuthData.erase(0, pos + sizeof(PDEL) + password.length());

    return std::pair(username, password);
}

std::optional<std::unordered_map<std::string, unsigned char *>> Message::extractFileList() {
    if(this->type != FILE_LIST)
        return std::optional< std::unordered_map<std::string, unsigned char *> >();

    std::unordered_map<std::string, unsigned char *> decodedFileList;
    std::string codedFileList(this->data.begin(), this->data.end());

    size_t pos = 0;
    std::string file;
    std::string hash_;
    unsigned char* hash_ptr;
    while ((pos = codedFileList.find(FDEL)) != std::string::npos) {
        //Estrazione FILE
        file = codedFileList.substr(0, pos);
        codedFileList.erase(0, pos + sizeof(FDEL) + file.length());

        //Estrazione HASH
        pos = codedFileList.find(HDEL);
        hash_ = codedFileList.substr(0,  pos);
        codedFileList.erase(0, pos + sizeof(HDEL) + hash_.length());
        if(!hash_.empty()) {
            hash_ptr = new unsigned char[SHA256_DIGEST_LENGTH]();
            memcpy(hash_ptr, hash_.c_str(), SHA256_DIGEST_LENGTH);
            decodedFileList[file] = hash_ptr;
            hash_ptr = nullptr;
        } else decodedFileList[file] = nullptr;
    }

    return decodedFileList;
}
