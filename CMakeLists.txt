cmake_minimum_required(VERSION 3.10)
project(MiniAiServer)

# 导出 compile_commands.json（供 clangd 使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 默认 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug 编译参数（兼容 valgrind）
set(CMAKE_CXX_FLAGS_DEBUG "-g -gdwarf-4 -O0 -Wall -Wextra")
add_compile_options(-Wunused-parameter -Wno-unused-parameter -Wno-reorder -Wreorder)

# === 子模块依赖 ===
# 加入ModularNetServer
add_subdirectory(external/ModularNetServer)
# include_directories(external/ModularNetServer)

# === 项目源码 ===
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/ModularNetServer/src
    # ${PROJECT_SOURCE_DIR}/src/third_party
)

# 递归包含 ModularNetServer 所有 src 子目录
file(GLOB_RECURSE MODULAR_HEADERS LIST_DIRECTORIES true ${PROJECT_SOURCE_DIR}/external/ModularNetServer/src/*)
foreach(dir ${MODULAR_HEADERS})
    if(IS_DIRECTORY ${dir})
        include_directories(${dir})
    endif()
endforeach()


# === Onnx Runtime 路径 ===
set(ONNXRUNTIME_ROOTDIR /dev/onnxruntime)
include_directories(${ONNXRUNTIME_ROOTDIR}/include ${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session)
link_directories(${ONNXRUNTIME_ROOTDIR}/build/Linux/Release)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*src/test/.*")
list(FILTER SOURCES EXCLUDE REGEX ".*src/main.cpp")

message(STATUS "Sources: ${SOURCES}")
# === 可执行程序 ===
# 主程序
add_executable(MiniAiServer src/main.cpp ${SOURCES})
# message(STATUS "optional: -fstandalone-debug")
target_link_libraries(MiniAiServer ModularNetServer onnxruntime)

# 测试程序
add_executable(test_chat src/test/test_chat.cpp ${SOURCES})
target_link_libraries(test_chat ModularNetServer onnxruntime)

# add_executable(test_cache src/test/test_cache.cpp ${SOURCES})
# target_link_libraries(test_cache stdc++fs mysqlclient ssl crypto hiredis pthread )