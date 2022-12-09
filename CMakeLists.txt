cmake_minimum_required(VERSION 3.16)
project(HelloDPDK)

option(USER "Option for compiling user code" OFF)
option(CONTROLLER "Option for compiling test code" OFF)

find_package(PkgConfig REQUIRED)
pkg_search_module(DPDK REQUIRED libdpdk)

if (${CONTROLLER})
    message(STATUS "Fetching GTest")
    include(FetchContent)
    FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
    )
    set(gtest_force_shared_crt ON)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)

    message(STATUS "Will compile Test Code")
    add_executable(test test.cpp)
    target_link_libraries(test PUBLIC ${DPDK_LIBRARIES})
    target_include_directories(test PUBLIC ${DPDK_INCLUDE_DIRS})
    target_link_libraries(test PUBLIC GTest::gtest_main)

    target_compile_features(test PUBLIC cxx_std_17)
    target_compile_options(test PUBLIC -mssse3)
endif()

if (${USER})
    message(STATUS "Will compile User Code")
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/yourcode.cpp)
        add_library(router ${CMAKE_CURRENT_SOURCE_DIR}/yourcode.cpp)
        message(STATUS "C++ Mode")
    elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/yourcode.c)
        add_library(router ${CMAKE_CURRENT_SOURCE_DIR}/yourcode.c)
        message(STATUS "C Mode")
    else()
        message(FATAL_ERROR "Your code is not found")
    endif()

    add_executable(client main.cpp)
    target_compile_features(client PUBLIC cxx_std_17)

    target_link_libraries(client PUBLIC router)
    target_link_libraries(client PUBLIC ${DPDK_LIBRARIES})
    target_include_directories(client PUBLIC ${DPDK_INCLUDE_DIRS})
    target_compile_options(client PUBLIC -mssse3)
endif()
