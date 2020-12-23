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
    int success;

    if(this->data.empty()) return;

    unsigned char hash_[SHA256_DIGEST_LENGTH];

    //Calcolo hash
    SHA256_CTX sha256;
    success = SHA256_Init(&sha256);
    if(!success) {
        this->hash = "";
        return;
    }
    success = SHA256_Update(&sha256, this->data.data(), this->data.size());
    if(!success){
        this->hash = "";
        return;
    }
    success = SHA256_Final(hash_, &sha256);
    if(!success){
        this->hash = "";
        return;
    }

    this->hash = unsignedCharToHEX(hash_, SHA256_DIGEST_LENGTH);
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

std::string Message::unsignedCharToHEX(unsigned char* src, size_t src_length){
    int error;
    char tmp[2*src_length+1];
    tmp[2*src_length] = 0;
    for (int i = 0; i < src_length; i++) {
        error = sprintf(tmp + i * 2, "%02x", src[i]);
        if(error < 0) break;
    }

    if(error < 0) return "";

    return std::string(tmp);
}

unsigned char* Message::HEXtoUnsignedChar(const std::string& src){
    int error;
    auto tmp = new unsigned char[src.length()/2]{0};
    unsigned int number = 0;

    for(int i=0, j = 0;i<src.length();i+=2, j++) {
        error = sscanf(&src.c_str()[i], "%02x", &number);
        if(error == EOF) break;
        tmp[j] = (unsigned char) number;
    }

    if(error == EOF) return nullptr;

    return tmp;
}

std::string Message::compute_password(const std::string& password, const std::string& salt, int iterations, int dkey_lenght){
    auto salt_ = HEXtoUnsignedChar(salt);

    if(!salt_) return "";

    auto key = new unsigned char[dkey_lenght];
    int success = PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                                    salt_, dkey_lenght,
                                    iterations, EVP_sha3_512(),
                                    dkey_lenght, key);

    if(!success) return "";

    auto key_HEX = unsignedCharToHEX(key, dkey_lenght);

    delete[] salt_;
    delete[] key;

    return key_HEX;
}

unsigned char* Message::generate_salt(int salt_length){
    int success;
    auto salt = new unsigned char[salt_length];
    success = RAND_bytes(salt, salt_length);

    if(!success)
        return nullptr;
    else
        return salt;
}