cmake_minimum_required(VERSION 3.16)

project(MiniAiServerControlCenter LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(MiniAiServerControlCenter
    main.cpp
    main_window.cpp
    main_window.h
    server_process_manager.cpp
    server_process_manager.h
    server_status_client.cpp
    server_status_client.h
)

target_link_libraries(MiniAiServerControlCenter
    PRIVATE
    Qt6::Widgets
    Qt6::Network
)