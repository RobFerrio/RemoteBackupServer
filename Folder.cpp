//
// Created by rob on 12/12/20.
//

#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <openssl/sha.h>
#include "Folder.h"

#define CHUNK_SIZE 1024

std::string fileHash(const std::string& file){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    std::ifstream ifs;
    std::vector<char> buffer(CHUNK_SIZE);

    ifs.open(file, std::ios::binary);
    while(!ifs.eof()) {
        ifs.read(buffer.data(), CHUNK_SIZE);
        size_t size = ifs.gcount();
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, buffer.data(), size);
    }
    SHA256_Final(hash, &sha256);
    ifs.close();

    //Conversione da unsigned char a string
    char hash_[2*SHA256_DIGEST_LENGTH+1];
    hash_[2*SHA256_DIGEST_LENGTH] = 0;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(hash_+i*2, "%02x", hash[i]);

    return std::string(hash_);
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
