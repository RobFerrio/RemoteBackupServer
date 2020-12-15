//
// Created by rob on 12/12/20.
//

#ifndef REMOTEBACKUPSERVER_FOLDER_H
#define REMOTEBACKUPSERVER_FOLDER_H

#include <unordered_map>

#define CLIENT_MISSING_DIR  0
#define CLIENT_MISSING_FILE 1

class Folder {
    std::unordered_map<std::string, std::string> paths;
    std::string folderPath;

public:
    Folder() = default;
    explicit Folder(std::string folderPath);

    std::unordered_map<std::string, std::string> getPaths() const ;

    std::unordered_map<std::string, int> compare(const std::unordered_map<std::string, std::string>& clientFolder);
};


#endif //REMOTEBACKUPSERVER_FOLDER_H
