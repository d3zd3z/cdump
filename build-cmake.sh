# cmake -DCMAKE_CXX_COMPILER=$(which clang++) -DCMAKE_C_COMPILER=$(which clang) ..
CMAKE_FLAGS=""
CMAKE_FLAGS="${CMAKE_FLAGS} -DCMAKE_CXX_COMPILER=$(which clang++)"
CMAKE_FLAGS="${CMAKE_FLAGS} -DCMAKE_C_COMPILER=$(which clang)"
CMAKE_FLAGS="${CMAKE_FLAGS} -G Ninja"
cmake ${CMAKE_FLAGS} ..
