# plugins

add_library(plugins gaussian.c mpi_rank_fill.c "sd/sd.c")
target_include_directories(plugins PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${HDF5_INCLUDE_DIRS}"
    "${MPI_INCLUDE_PATH}"
)

install(TARGETS 
        plugins
        DESTINATION lib)
