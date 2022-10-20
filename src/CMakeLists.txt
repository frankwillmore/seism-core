add_executable(seism-core 
    "${PROJECT_SOURCE_DIR}/src/seism-core-slice.cc"
    "${PROJECT_SOURCE_DIR}/src/seism-core-attributes.cc"
)
target_include_directories(seism-core PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${HDF5_INCLUDE_DIRS}"
    "${MPI_INCLUDE_PATH}"
)
#target_link_libraries(seism-core PUBLIC hdf5 mpi)
target_link_libraries(seism-core PUBLIC hdf5 mpi dl)

#add_executable(ljx 
#    "${PROJECT_SOURCE_DIR}/applications/ljx/ljx_main.c"
#    "${PROJECT_SOURCE_DIR}/applications/ljx/command_line_parser.c"
#    "${PROJECT_SOURCE_DIR}/applications/ljx/io_setup.c"
#    "${PROJECT_SOURCE_DIR}/applications/ljx/energy.c"
#    "${PROJECT_SOURCE_DIR}/applications/ljx/graphics.c"
#)
#target_include_directories(ljx PUBLIC 
#    "${PROJECT_SOURCE_DIR}/include"
#    "${X11_INCLUDE_DIR}"
#)
#target_link_libraries(ljx PUBLIC ftw_general m pthread X11)

#add_executable(hs 
#    "${PROJECT_SOURCE_DIR}/applications/hs/hs_main.c"
#    "${PROJECT_SOURCE_DIR}/applications/hs/io_setup.c"
#    "${PROJECT_SOURCE_DIR}/applications/hs/graphics.c"
#)
#target_include_directories(hs PUBLIC 
#    "${PROJECT_SOURCE_DIR}/include"
#    "${X11_INCLUDE_DIR}"
#)
#target_link_libraries(hs PUBLIC ftw_general m pthread X11)

install(TARGETS 
        seism-core 
        DESTINATION bin)
