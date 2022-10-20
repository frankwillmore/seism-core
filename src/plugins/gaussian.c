//#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <hdf5.h>

/* Gaussian fill function
 *
 * invocation:
 *
 * use_function_lib libplugins.so
 * use_function_name gaussian
 * use_function_argc 4
 * use_function_argv 1 1 1 1
 *
 * IN:  mpi_rank                                     rank of calling process
 *      system_size[3]                               number of domain blocks in each direction
 *      domain_block_size[3]                         size of domain blocks
 *      domain_block_number[3]                       which domain block
 *      position_in_block[3]                         position within domain block
 *
 *      Below args are plugin-specific:
 *
 *      argc                                         # of arguments passed
 *      argv[1] == A                                 amplitude
 *      argv[2, 3, 4] == sigma_x, sigma_y, sigma_z   scaling parameters
 *
 * OUT: returns f(x, x, z) = A * exp{ -((x - x0)^2 / sigma_x^2 + (y - y0)^2 / sigma_y^2 + (z - z0)^2 / simgma_z^2) )
 */

float gaussian(int mpi_rank, hsize_t* system_size, hsize_t* domain_block_size, hsize_t* domain_block_number, hsize_t* position_in_block, int argc, char **argv){

    float A = strtof(argv[0], NULL);

    float sigma_x = strtof(argv[1], NULL);
    float sigma_y = strtof(argv[2], NULL);
    float sigma_z = strtof(argv[3], NULL);

    float x = domain_block_size[0] * domain_block_number[0] + position_in_block[0];
    float y = domain_block_size[1] * domain_block_number[1] + position_in_block[1];
    float z = domain_block_size[2] * domain_block_number[2] + position_in_block[2];
     
    // center the shape in the middle of the block
    float x0 = 0.5 * domain_block_size[0] * system_size[0];
    float y0 = 0.5 * domain_block_size[1] * system_size[1];
    float z0 = 0.5 * domain_block_size[2] * system_size[2];

    return A * exp( - ( ((x - x0) * (x - x0)) / (sigma_x * sigma_x) + ((y - y0) * (y - y0)) / (sigma_y * sigma_y) + ((z - z0) * (z - z0)) / (sigma_z * sigma_z) ) );
}

