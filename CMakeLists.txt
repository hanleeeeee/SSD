cmake_minimum_required(VERSION 3.10)
project(VirtualSSD)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# SSD Server (ssd_server.cpp)
add_executable(ssd_server ssd_server.cpp)
target_link_libraries(ssd_server Boost::system Threads::Threads)

# SSD Shell / Client (ssd_shell.cpp)
add_executable(ssd_shell ssd_shell.cpp)
target_link_libraries(ssd_shell Boost::system Threads::Threads)
