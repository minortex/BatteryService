cmake_minimum_required(VERSION 3.16)
project(BatteryService)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS DBus)

file(GLOB_RECURSE SOURCE "src/*.cpp")
file(GLOB_RECURSE HEADER "include/*.h")

qt_add_executable(BatteryService
${SOURCE} ${HEADER}
)

target_include_directories(BatteryService PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(BatteryService
    PRIVATE Qt6::DBus
)
