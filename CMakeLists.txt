cmake_minimum_required(VERSION 3.15)
project(obs_websocket_sdk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3   -static-libstdc++ -static-libgcc")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -static-libstdc++ -static-libgcc")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -static-libstdc++ -static-libgcc")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

find_package(Libwebsockets)
find_package(nlohmann_json)

add_library(obs_websocket_sdk SHARED
    src/collections.h
    src/obs_websocket_client.cpp
    src/obs_websocket_client.h
	src/obs_websocket.h
	src/sha_256.h
	src/base64.h
)


target_include_directories(obs_websocket_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(obs_websocket_sdk
    PRIVATE
    websockets
    nlohmann_json::nlohmann_json
)

add_executable(obs_websocket_demo
    src/test.cpp
)

target_link_libraries(obs_websocket_demo
    PRIVATE
    obs_websocket_sdk
)