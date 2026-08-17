cmake_minimum_required(VERSION 3.16)

project(MiniAiServerControlCenter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(MiniAiServerControlCenter
    main.cpp
    main_window.cpp
    main_window.h
    server_process_manager.cpp
    server_process_manager.h
)

target_link_libraries(MiniAiServerControlCenter
    PRIVATE
    Qt6::Widgets
)