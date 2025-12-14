cmake_minimum_required(VERSION 4.0)
project(graphic_cw)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found, enabling parallelization")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    message(WARNING "OpenMP not found, compiling without parallelization")
endif()

add_executable(graphic_cw
        Code/main.cpp
        Code/Vector3.h
        Code/Shape.h
        Code/Sphere.h
        Code/Plane.h
        Code/Cube.h
        Code/BVH.h
        Code/main.cpp
        Code/Image.cpp
        Code/BVH.cpp
        Code/Sphere.cpp
        Code/Plane.cpp
        Code/Cube.cpp
        Code/camera.cpp
        Code/Scene.cpp
        Code/SceneUtils.h
        Code/SceneUtils.cpp
        Code/Sampling.h

)

if(OpenMP_CXX_FOUND)
    target_link_libraries(graphic_cw OpenMP::OpenMP_CXX)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(graphic_cw PRIVATE -fopenmp)
endif()
