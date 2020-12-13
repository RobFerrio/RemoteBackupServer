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
    std::vector<char> buffer;

    ifs.open(file, std::ios::binary);
    while(!ifs.eof()) {
        ifs.read(buffer.data(), CHUNK_SIZE);
        size_t size = ifs.gcount();
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, buffer.data(), size);
    }
    SHA256_Final(hash, &sha256);
    ifs.close();

    return std::string(reinterpret_cast<char *>(hash), SHA256_DIGEST_LENGTH);
}

Folder::Folder(std::string path): folderPath(std::move(path)) {
    if(!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath))
        std::filesystem::create_directory(folderPath);  //Crea la cartella se non esiste

    for(auto &file : std::filesystem::recursive_directory_iterator(folderPath)) {
        paths[file.path().string()] = {};  //Path è una directory
        if(file.is_regular_file())
            paths[file.path().string()] = fileHash(file.path().string()); //Path è un file
    }
}

std::unordered_map<std::string, int> Folder::compare(std::unordered_map<std::string, std::string> clientFolder) {
    std::unordered_map<std::string, int> diffs;
    //Check client -> server
    for(std::pair<std::string, std::string> path : clientFolder){
        if(!paths.contains(path.first)) {
            if(path.second.empty()){                //Directory mancante
                std::filesystem::create_directory(path.first);
                paths[path.first] = path.second;
            } else {                                //File mancante
                diffs[path.first] = SERVER_MISSING_FILE;
            }
        }
        else if(paths[path.first] != path.second){  //File diverso
            diffs[path.first] = SERVER_MISSING_FILE;
        }
    }
    //Check server -> client
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
