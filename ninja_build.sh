mkdir -p build_ninja && cd build_ninja
cmake .. -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=On -D CMAKE_CXX_COMPILER=g++-8 -D CMAKE_C_COMPILER=gcc-8 -D CMAKE_BUILD_TYPE=Debug
ninja
