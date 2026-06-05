cmake_minimum_required(VERSION 3.10)
project(bdgb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(bdgb
    src/main.c
    src/node.c
    src/grid.c
    src/storage.c
    src/semantics.c
    src/concept_graph.c
    src/search.c
    src/learning.c
)

if(WIN32)
    target_link_libraries(bdgb m)
endif()
