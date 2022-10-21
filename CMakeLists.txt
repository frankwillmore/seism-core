cmake_minimum_required(VERSION 3.12)

# set the project name
project(seism-core)

option(BUILD_PLUGINS "Build plugin library for additional data options" OFF)

option(BUILD_SHARED_LIBS "shared libs " ON)
option(BUILD_STATIC_LIBS "static libs " ON)

find_package(HDF5)
#add_library(hdf5 SHARED IMPORTED)
#add_library(hdf5-shared SHARED IMPORTED)
#set_target_properties(hdf5 PROPERTIES IMPORTED_LOCATION "${HDF5_LIBRARIES}")
set_target_properties(hdf5-shared PROPERTIES IMPORTED_LOCATION "${HDF5_LIBRARIES}")

find_package(MPI)
add_library(mpi SHARED IMPORTED)
set_target_properties(mpi PROPERTIES IMPORTED_LOCATION "${MPI_C_LIBRARIES}")

add_subdirectory(src)

