// seism-core-test.c //////////////////////////////////////////////////////////
// This kernel writes a four-dimensional a chunked HDF5 dataset in parallel
// based on inputs provided on standard input. The required inputs are the
// following:
//
// 1. The dimensions (> 1) of a 3D process grid. The number of processes
//    must be equal to the MPI communicator size.
// 2. The spatial dimensions (> 1) of the HDF5 dataset chunks.
// 3. The voxel resolution (per process).
// 4. The number of time steps (>= 1) to be written.
//
// An example input script is shown below. Binary options are set by 
// presence or absence of corresponding line:
//
// mpiexec -n 8 ./seism-core << EOF
// processor 2 2 2
// chunk 180 64 64
// domain 360 64 64
// time 2
// collective_write
// precreate
// set_collective_metadata
// early_allocation
// never_fill
// DONE
// EOF
//
///////////////////////////////////////////////////////////////////////////////

#include "hdf5.h"

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "seism-core-attributes.hh"

using namespace std;

#define CHUNKED_DSET_NAME "chunked"

///////////////////////////////////////////////////////////////////////////////
  
int main(int argc, char** argv)
{
    if (argc < 2) 
      {
        cout << "No filename specified\n" ;
        exit(1);
      }
    char *filename = argv[1];

    // open file and read the attributes
    hid_t file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    assert(file >= 0);
    seismCoreAttributes attr(file);

    cout << endl;
    cout << "processor dims: ";
    cout << attr.processor_dims[0] << ":";
    cout << attr.processor_dims[1] << ":";
    cout << attr.processor_dims[2] << endl;
    cout << "chunk dims: ";
    cout << attr.chunk_dims[0] << ":";
    cout << attr.chunk_dims[1] << ":";
    cout << attr.chunk_dims[2] << endl;
    cout << "domain dims: ";
    cout << attr.domain_dims[0] << ":";
    cout << attr.domain_dims[1] << ":";
    cout << attr.domain_dims[2] << endl;
    cout << endl;

    // open the dataset
    hid_t dset = H5Dopen (file, CHUNKED_DSET_NAME, H5P_DEFAULT);

    // create a buffer to hold one domain worth of data
    hsize_t domain_size = 
        attr.domain_dims[0] * attr.domain_dims[1] * attr.domain_dims[2];
    unsigned int *buffer = 
        (unsigned int *)malloc(sizeof(unsigned int) * domain_size);

    unsigned long found_correct = 0;
    unsigned long found_incorrect = 0;

    // loop over time and processor geom
    for (unsigned int t=0; t<attr.simulation_time; t++) 
    for (
            unsigned int processor_i=0; 
            processor_i<attr.processor_dims[0]; 
            processor_i++
        )
            for 
            (
                unsigned int processor_j=0; 
                processor_j<attr.processor_dims[1]; 
                processor_j++
            )
                for 
                (
                    unsigned int processor_k=0;
                    processor_k<attr.processor_dims[2];
                    processor_k++
                )
                {
                    unsigned int original_mpi_rank = 
                        processor_i * attr.processor_dims[1] 
                        * attr.processor_dims[2] 
                        + processor_j * attr.processor_dims[2] 
                        + processor_k;
                    cout << "checking time step #" << t << " / processor @ ( " 
                         << processor_i << ", " << processor_j << ", " 
                         << processor_k << " )" << " with original rank #" 
                         << original_mpi_rank << endl;

                    // get the dataspace
                    hid_t fspace = H5Dget_space(dset);
                    hsize_t start[4] = {t, 
                        processor_i * attr.domain_dims[0], 
                        processor_j * attr.domain_dims[1],
                        processor_k * attr.domain_dims[2]};
                    hsize_t stride[4] = {1,1,1,1};
                    hsize_t count[4] = {1,1,1,1};
                    hsize_t block[4] = {1,
                        attr.domain_dims[0],
                        attr.domain_dims[1],
                        attr.domain_dims[2]};

                    // select hyperslab within file dataspace
                    assert ( H5Sselect_hyperslab(
                        fspace, H5S_SELECT_SET, start, stride, count, block ) 
                        >= 0 );

                    hid_t mspace = H5Screate_simple(4, block, NULL);
                    assert (mspace >= 0);
                    assert (H5Sselect_all(mspace) >= 0);

                    // read the dataset into the buffer
                    assert( H5Dread (dset, H5T_NATIVE_UINT, mspace, fspace, 
                            H5P_DEFAULT, buffer) >= 0);
                    
                    unsigned long local_errors = 0;
                    // loop over elements in buffer, check value
                    for 
                    (
                        unsigned int domain_i=0;
                        domain_i<attr.domain_dims[0]; 
                        domain_i++
                    )
                        for 
                        (
                            unsigned int domain_j=0; 
                            domain_j<attr.domain_dims[1]; 
                            domain_j++
                        )
                            for 
                            (
                                unsigned int domain_k=0; 
                                domain_k<attr.domain_dims[2]; 
                                domain_k++
                            )
                            {
                                unsigned int buffer_element = 
                                    domain_i * attr.domain_dims[1] 
                                    * attr.domain_dims[2] 
                                    + domain_j * attr.domain_dims[2] 
                                    + domain_k ;
                                if (buffer[buffer_element] 
                                        == original_mpi_rank) found_correct++;
                                else local_errors++;
                            }
                            if (local_errors)
                            {
                                cout << "Found " << local_errors << " errors."
                                     << endl;
                                found_incorrect += local_errors;
                            }

                    H5Sclose(fspace);
                    H5Sclose(mspace);

                } // end loop over k, j, i, t

    cout << endl;
    cout << "Checking complete. Found totals of: " << found_correct 
         << " correct / " << found_incorrect << " incorrect." << endl;
    cout << endl;

    H5Dclose(dset);
    H5Fclose(file);
}

