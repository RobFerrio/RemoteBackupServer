//
// Created by rob on 12/12/20.
//

#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <openssl/sha.h>
#include "Folder.h"
#include "Message.h"

#define HASH_CHUNK_SIZE 1024

std::string fileHash(const std::string& file){
    int success;
    unsigned char tmp[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    std::ifstream ifs;
    std::vector<char> buffer(HASH_CHUNK_SIZE);

    success = SHA256_Init(&sha256);

    if(!success) return "";

    //Lettura + hash chunk file
    ifs.open(file, std::ios::binary);
    while(!ifs.eof()) {
        if(!std::filesystem::exists(file)) {
            success = 0;
            break;
        }
        ifs.read(buffer.data(), HASH_CHUNK_SIZE);
        size_t size= ifs.gcount();
        success = SHA256_Update(&sha256, buffer.data(), size);
        if(!success) break;
    }

    if(!success) {
        return "";
    }

    ifs.close();

    success = SHA256_Final(tmp, &sha256);

    if(!success) {
        return "";
    }

    return Message::unsignedCharToHEX(tmp, SHA256_DIGEST_LENGTH);
}

Folder::Folder(std::string path): folderPath(std::move(path)) {
    if(!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath))
        std::filesystem::create_directory(folderPath);  //Crea la cartella se non esiste

    for(auto &file : std::filesystem::recursive_directory_iterator(folderPath)) {
        if(file.is_regular_file())
            paths[file.path().string()] = fileHash(file.path().string());   //Path è un file
        else
            paths[file.path().string()] = {};                               //Path è una directory
    }
}

std::map<std::string, std::string>& Folder::getPaths() {
    return paths;
}

std::map<std::string, int> Folder::compare(const std::map<std::string, std::string>& clientFolder) {
    std::map<std::string, int> diffs;

    //Check file/directory mancanti al client
    for(std::pair<std::string, std::string> path : paths){
        if(!clientFolder.contains(path.first)){
            if(path.second.empty()){                //Directory mancante
                diffs[path.first] = CLIENT_MISSING_DIR;
            } else {                                //File mancante
                diffs[path.first] = CLIENT_MISSING_FILE;
            }
        }
    }

    return diffs;
}
