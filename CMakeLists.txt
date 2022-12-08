cmake_minimum_required(VERSION 3.16)
project(HelloDPDK)

option(USER "Option for compiling user code" OFF)
option(CONTROLLER "Option for compiling test code" OFF)

if (${CONTROLLER})
    message(STATUS "Compile Test Code")
    add_executable(test test.cpp)

    target_compile_features(test PUBLIC cxx_std_17)
endif()

if (${USER})
    message(STATUS "Compile User Code")
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
endif()
