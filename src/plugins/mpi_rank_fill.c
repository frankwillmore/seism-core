#include <math.h>
#include <stdlib.h>
#include <hdf5.h>

/* MPI rank fill function
 *
 * invocation:
 *
 * use_function_lib libplugins.so
 * use_function_name mpi_rank_fill
 * use_function_argc 0
 * use_function_argv 1 1 1 1
 *
 * IN:  mpi_rank                                     rank of calling process
 *      system_size[3]                               number of domain blocks in each direction
 *      domain_block_size[3]                         size of domain blocks
 *      domain_block_number[3]                       which domain block
 *      position_in_block[3]                         position within domain block
 *
 * OUT: returns f(x, x, z) = mpi_rank
 */

float mpi_rank_fill(int mpi_rank, hsize_t* system_size, hsize_t* domain_block_size, hsize_t* domain_block_number, hsize_t* position_in_block, int argc, char **argv){

    return (float)mpi_rank;
}

bool mpi_rank_fill_check(int mpi_rank, hsize_t* system_size, hsize_t* domain_block_size, hsize_t* domain_block_number, hsize_t* position_in_block, int argc, char **argv){

    /* check feature is not implemented, this is just a stub to show how check function might look */
    return false; 
}

