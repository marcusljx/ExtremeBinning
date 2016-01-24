cmake_minimum_required(VERSION 3.3)
project(ExtremeBinning)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lssl -lcrypto")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(SOURCE_FILES main.cpp PrimaryIndex.cpp PrimaryIndex.h XB_includes.h)
add_executable(ExtremeBinning ${SOURCE_FILES})

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})

# Boost
#set(BOOST_ROOT /usr/local/src/boost_1_60_0)
find_package(Boost REQUIRED)
include_directories(${BOOST_INCLUDEDIR})
set(LIBS ${LIBS} ${BOOST_LIBRARIES})


# Link all libs
target_link_libraries(${PROJECT_NAME} ${LIBS})
