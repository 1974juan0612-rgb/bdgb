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
    src/nlp.c
    src/agent.c
)

add_executable(bdgb_test
    tests/test_bdgb.c
    src/node.c
    src/grid.c
    src/storage.c
    src/semantics.c
    src/concept_graph.c
    src/search.c
    src/learning.c
    src/nlp.c
    src/agent.c
)

if(WIN32)
    target_link_libraries(bdgb m)
    target_link_libraries(bdgb_test m)
endif()
