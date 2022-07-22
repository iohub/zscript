cmake_minimum_required(VERSION 3.11)
project(Zeroscript)

include(FetchContent)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

set(ANTLR_URL https://www.antlr.org/download/antlr4-cpp-runtime-4.7.2-source.zip)
FetchContent_Declare(antlr URL ${ANTLR_URL} URL_HASH MD5=3a88c898579f78d8c84c583c5160986b)
FetchContent_MakeAvailable(antlr)
set(ANTLR_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/antlr-src)
list(APPEND CMAKE_MODULE_PATH ${ANTLR_DIR}/cmake)

include_directories(${ANTLR_DIR}/runtime/src)

add_custom_target(Antlr
        COMMAND antlr4 zeroscript.g4 -Dlanguage=Cpp -o src/compiler/parser/generated
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES zeroscript.g4)

add_executable(zero src/main.cpp)
add_executable(tester src/tester/tester.cpp)

target_link_libraries(zero pthread antlr4_static)

add_dependencies(zero Antlr)

set_target_properties(zero PROPERTIES COMPILE_FLAGS " -Ofast")
