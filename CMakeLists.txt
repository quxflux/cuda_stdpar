cmake_minimum_required(VERSION 3.22)

project(cuda_stdpar LANGUAGES CXX)

add_executable(cuda_stdpar main.cpp)

set_target_properties(cuda_stdpar PROPERTIES CXX_STANDARD 20
                                             CXX_STANDARD_REQUIRED ON
                                             CXX_EXTENSIONS OFF)

target_compile_options(cuda_stdpar PRIVATE 
    $<$<CXX_COMPILER_ID:NVHPC>: -stdpar=gpu -Wextra $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>: -gpu=fastmath -O3>>
)

target_link_options(cuda_stdpar PRIVATE 
    $<$<CXX_COMPILER_ID:NVHPC>: -stdpar=gpu>
)
