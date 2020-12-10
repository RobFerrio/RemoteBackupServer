//
// Created by rob on 04/12/20.
//

#include <cstring>
#include <utility>
#include "Message.h"

Message::Message(): type(-1), size(0) {}

Message::Message(int type): type(type), size(0) {}

Message::Message(int type, std::vector<char> data, size_t size): type(type), data(std::move(data)), size(size) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, this->data.data(), this->data.size());
    SHA256_Final(hash, &sha256);
}

int Message::getType() const {
    return type;
}

std::vector<char> Message::getData() const {
    return data;
}

size_t Message::getSize() const {
    return size;
}

bool Message::checkHash() const{
    unsigned char hashRecomputed[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hashRecomputed, &sha256);

    bool isEqual = (memcmp(hash, hashRecomputed, SHA256_DIGEST_LENGTH) == 0);

    return isEqual;
}
