cmake_minimum_required(VERSION 3.15)
project(mpm_simulation)

set(CMAKE_CXX_STANDARD 17)

find_package(pybind11 CONFIG REQUIRED)

add_library(mpm_simulation MODULE
    MPMSimulation.cpp
    MPMBindings.cpp
)

target_link_libraries(mpm_simulation PRIVATE pybind11::module)
set_target_properties(mpm_simulation PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")