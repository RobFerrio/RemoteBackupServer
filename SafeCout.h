//
// Created by rob on 16/12/20.
//

#ifndef REMOTEBACKUPSERVER_SAFECOUT_H
#define REMOTEBACKUPSERVER_SAFECOUT_H

#include <mutex>
#include <iostream>

constexpr bool debug = true;

static std::mutex coutMutex;

static void safe_cout(const std::string& msg){
    std::lock_guard<std::mutex> lg(coutMutex);
    std::cout << msg << std::endl;
}

static void debug_cout(const std::string& msg){
    if(debug) {
        std::lock_guard<std::mutex> lg(coutMutex);
        std::cout << msg << std::endl;
    }
}

#endif //REMOTEBACKUPSERVER_SAFECOUT_H
