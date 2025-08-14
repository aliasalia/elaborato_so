cmake_minimum_required(VERSION 3.20)
project(SHA256 VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MACOSX_RPATH 1)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wno-deprecated-declarations)

find_package(OpenSSL REQUIRED)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

add_executable(server src/server.c src/queue.c src/cache.c)
target_link_libraries(server ${OPENSSL_LIBRARIES})

add_executable(client src/client.c)
target_link_libraries(client ${OPENSSL_LIBRARIES})
