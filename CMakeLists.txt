cmake_minimum_required(VERSION 3.16)
project(RemoteBackupServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(RemoteBackupServer main.cpp BackupServer.cpp BackupServer.h Connection.cpp Connection.h)

#Necessari per far funzionare i threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(RemoteBackupServer PRIVATE Threads::Threads)