cmake_minimum_required(VERSION 3.16)
project(BatteryService)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS DBus)

set(SOURCES
  src/main.cpp
  src/UPowerBatteryInterface.cpp
  src/WriteSysfs.cpp)

set(HEADERS
  include/UPowerBatteryInterface.h
  include/WriteSysfs.h)

qt_add_executable(BatteryService
${SOURCES} ${HEADERS}
)

target_include_directories(BatteryService PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(BatteryService
    PRIVATE Qt6::DBus
)

install(TARGETS BatteryService DESTINATION bin)
