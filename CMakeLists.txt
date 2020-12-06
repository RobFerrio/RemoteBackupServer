cmake_minimum_required(VERSION 3.16)
project(RemoteBackupServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(RemoteBackupServer main.cpp BackupServer.cpp BackupServer.h Connection.cpp Connection.h Message.cpp Message.h)

#Necessari per far funzionare i threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.74.0 REQUIRED COMPONENTS filesystem serialization)
include_directories(${Boost_INCLUDE_DIR})

# Note: a target should be already defined using 'add_executable' or 'add_library'
target_link_libraries(RemoteBackupServer PRIVATE Threads::Threads ${Boost_LIBRARIES} crypto)
