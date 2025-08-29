cmake_minimum_required(VERSION 3.20)

include(FetchContent)

project(SmartHouse.Rf24Usb.Protocol.Device.Test LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TestSuitsSources 
  "SmartHouse/Rf24Usb/Protocol/DeviceTest.cpp"  "SmartHouse/Rf24Usb/Protocol/Utils.cpp")

enable_testing()

find_package(spdlog)
find_package(gtest)

add_executable(SmarthHouse.Rf24Usb.Protocol.Device.Test ${TestSuitsSources} )
set_property(TARGET SmarthHouse.Rf24Usb.Protocol.Device.Test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(SmarthHouse.Rf24Usb.Protocol.Device.Test PRIVATE SmartHouse.Rf24Usb.Protocol)
set_property(TARGET SmartHouse.Rf24Usb.Protocol PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(SmarthHouse.Rf24Usb.Protocol.Device.Test PRIVATE SmartHouse.Rf24Usb.Protocol.Device)
set_property(TARGET SmartHouse.Rf24Usb.Protocol.Device PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(SmarthHouse.Rf24Usb.Protocol.Device.Test PRIVATE spdlog::spdlog)
target_link_libraries(SmarthHouse.Rf24Usb.Protocol.Device.Test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(SmarthHouse.Rf24Usb.Protocol.Device.Test)