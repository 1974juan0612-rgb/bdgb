cmake_minimum_required(VERSION 3.10)
project(bdgb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include crypt)

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
    src/glifo.c
    src/util.c
    crypt/bdgb_crypt.c
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
    src/glifo.c
    src/util.c
    crypt/bdgb_crypt.c
)
target_compile_definitions(bdgb_test PRIVATE BDGB_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

if(UNIX)
    target_link_libraries(bdgb m)
    target_link_libraries(bdgb_test m)
endif()
