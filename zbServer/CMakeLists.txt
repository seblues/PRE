cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_STANDARD 11)

project (zigbeeServer)

include_directories(utils)

file(GLOB srcFiles utils/*.cpp)

add_executable(zigbeeServer ${srcFiles} zbserver.cpp)
target_link_libraries (zigbeeServer sqlite3 pthread)

add_executable(zigbeeClient ${srcFiles} zbclient.cpp)
target_link_libraries (zigbeeClient sqlite3 pthread)

add_executable(test_db ${srcFiles} databaseTest.cpp)
target_link_libraries (test_db sqlite3)

