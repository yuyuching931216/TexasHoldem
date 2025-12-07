cmake_minimum_required(VERSION 3.10)
project(TexasHoldemServer)

# 設置C++標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找到 Boost
find_package(Boost 1.70 REQUIRED COMPONENTS system)

# 包含目錄
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/server)

# 收集源文件
set(SERVER_SOURCES
    server/main.cpp
    server/Server.cpp
    server/Session.cpp
    server/Room.cpp
    server/Game.cpp
    server/Player.cpp
    server/Card.cpp
    server/HandEvaluator.cpp
)

# 創建可執行文件
add_executable(TexasHoldemServer ${SERVER_SOURCES})

# 鏈接庫
target_link_libraries(TexasHoldemServer ${Boost_LIBRARIES})

# Windows 特定設置
if(WIN32)
    target_link_libraries(TexasHoldemServer ws2_32 wsock32)
    # 設置為控制台應用程序
    set_target_properties(TexasHoldemServer PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# 編譯選項
if(MSVC)
    target_compile_options(TexasHoldemServer PRIVATE /W4)
else()
    target_compile_options(TexasHoldemServer PRIVATE -Wall -Wextra -pedantic)
endif()

# 安裝規則
install(TARGETS TexasHoldemServer
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Boost Version: ${Boost_VERSION}")
message(STATUS "Boost Include Dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")
