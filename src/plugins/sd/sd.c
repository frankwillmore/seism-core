#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <hdf5.h>

/* Fill function using data from UCSD group
 *
 * invocation:
 *
 * use_function_lib libplugins.so
 * use_function_name sd
 * use_function_argc 1
 * use_function_argv /path/to/data/file
 *
 * IN:  mpi_rank                                     rank of calling process (ignored)
 *      system_size[3]                               number of domain blocks in each direction (ignored) 
 *      domain_block_size[3]                         size of domain blocks 
 *      domain_block_number[3]                       which domain block (ignored)
 *      position_in_block[3]                         position within domain block
 *
 *      argc should be 1:                       
 *      argv[0] == path_to_data_file
 *
 * OUT: float value from the reference data file
 */

static bool sd_initialized = false;
static float *buffer;

static void initialize_sd(char* filename){

    FILE *fileptr;
    long filelen;

    fileptr = fopen(filename, "rb");      // Open the file in binary mode
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file

    buffer = (float *)malloc((filelen));  // Enough memory for file 
    fread(buffer, filelen, 1, fileptr);   // Read in the entire file
    fclose(fileptr);                      // Close the file

    sd_initialized = true;
}

static void close_sd(){
    free(buffer);
}

float sd(int mpi_rank, hsize_t* system_size, hsize_t* domain_block_size, hsize_t* domain_block_number, hsize_t* position_in_block, int argc, char **argv){

    if (!sd_initialized) initialize_sd(argv[0]);

    // map position_in_block and domain_block_size to a position in the data array.
    unsigned position = position_in_block[2] * domain_block_size[1] * domain_block_size[0]
                      + position_in_block[1] * domain_block_size[0]
                      + position_in_block[0];

    return buffer[position];
}

