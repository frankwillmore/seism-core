add_executable(seism-core 
    "${PROJECT_SOURCE_DIR}/src/seism-core-slice.cc"
    "${PROJECT_SOURCE_DIR}/src/seism-core-attributes.cc"
)
target_include_directories(seism-core PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${HDF5_INCLUDE_DIRS}"
    "${MPI_INCLUDE_PATH}"
)
target_link_libraries(seism-core PUBLIC hdf5 mpi dl)


add_executable(seism-core-check
    "${PROJECT_SOURCE_DIR}/src/seism-core-check.cc"
    "${PROJECT_SOURCE_DIR}/src/seism-core-attributes.cc"
)
target_include_directories(seism-core-check PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${HDF5_INCLUDE_DIRS}"
    "${MPI_INCLUDE_PATH}"
)
target_link_libraries(seism-core-check PUBLIC hdf5 mpi dl)


add_executable(seism-read
    "${PROJECT_SOURCE_DIR}/src/seism-read.cc"
    "${PROJECT_SOURCE_DIR}/src/seism-core-attributes.cc"
)
target_include_directories(seism-read PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${HDF5_INCLUDE_DIRS}"
    "${MPI_INCLUDE_PATH}"
)
target_link_libraries(seism-read PUBLIC hdf5 mpi dl)


if (BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()


install(TARGETS 
        seism-core 
        seism-core-check
        seism-read
        DESTINATION bin)
