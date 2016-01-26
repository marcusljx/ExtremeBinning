cmake_minimum_required(VERSION 2.8)
project(XB_Backup)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lssl -lcrypto")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(BACKUP_SOURCE_FILES XB_Backup.cpp PrimaryIndex.cpp PrimaryIndex.h XB_includes.h)
add_executable(XB_Backup ${BACKUP_SOURCE_FILES})

set(RESTORE_SOURCE_FILES XB_Restore.cpp XB_includes.h)
add_executable(XB_Restore ${RESTORE_SOURCE_FILES})

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})

# Link all libs
target_link_libraries(${PROJECT_NAME} ${LIBS})