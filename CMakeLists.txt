cmake_minimum_required(3.10)

add_executable(test test.cpp)

if (EXISTS yourcode.cpp)
    add_library(router yourcode.cpp)
elseif (EXISTS yourcode.c)
    add_library(router yourcode.c)
endif()
add_executable(client main.cpp)
target_link_libraries(client PUBLIC router)
