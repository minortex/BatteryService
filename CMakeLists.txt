set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.16)
project(batteryctl)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS DBus)

set(SOURCES
  src/main.cpp
  src/UPowerBatteryInterface.cpp
  src/WriteSysfs.cpp)

set(HEADERS
  include/ChargePolicy.h
  include/UPowerBatteryInterface.h
  include/WriteSysfs.h)

qt_add_executable(batteryctl
${SOURCES} ${HEADERS}
)

target_include_directories(batteryctl PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(batteryctl
    PRIVATE Qt6::DBus
)

enable_testing()

add_executable(ChargePolicyTest
  tests/ChargePolicyTest.cpp
  include/ChargePolicy.h)

target_include_directories(ChargePolicyTest PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/include")

add_test(NAME ChargePolicyTest COMMAND ChargePolicyTest)

add_test(NAME batteryctlHelp COMMAND batteryctl help)
set_tests_properties(batteryctlHelp PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage: batteryctl")

install(TARGETS batteryctl DESTINATION bin)

# install systemd units

set(SYSTEMD_UNIT_DIR "/usr/lib/systemd/system")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BatteryService.service"
  DESTINATION "${SYSTEMD_UNIT_DIR}")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/docs/batteryctl.1"
  DESTINATION share/man/man1)
