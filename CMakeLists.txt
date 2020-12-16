cmake_minimum_required(VERSION 3.16)
project(RemoteBackupServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(RemoteBackupServer main.cpp BackupServer.cpp BackupServer.h Connection.cpp Connection.h Message.cpp Message.h Folder.cpp Folder.h SafeCout.h)

if(CMAKE_SYSTEM_NAME STREQUAL Linux OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif ()

# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.74.0 REQUIRED COMPONENTS filesystem serialization)
include_directories(${Boost_INCLUDE_DIR})

# Note: a target should be already defined using 'add_executable' or 'add_library'
if(CMAKE_SYSTEM_NAME STREQUAL Linux OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
    target_link_libraries(RemoteBackupServer PRIVATE Threads::Threads ${Boost_LIBRARIES} crypto)
else()
    target_link_libraries(RemoteBackupServer ${Boost_LIBRARIES} crypto)
endif()
