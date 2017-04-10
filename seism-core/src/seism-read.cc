// seism-read.c //////////////////////////////////////////////////////////////
//
// This code reads a test file written by seism-core, extracts metadata, 
// and performs a parallel read of stored data. The metadata (attributes
// object in input h5 file) contains info on data size, subfiling, etc.
// usage: 
// 
// mpiexec seism-read input-file.h5
//
///////////////////////////////////////////////////////////////////////////////

#include "hdf5.h"

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>

#include "seism-core-attributes.hh"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
  
int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int mpi_rank, mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (argc < 2) {
        cout << "No filename specified\n" ;
        MPI_Finalize();
        return(1);
    }
    char* filename = argv[1];

    if (mpi_rank == 0) cout << "Reading " << filename << endl;
    
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
    hid_t dset = H5Dopen (file, "chunked", H5P_DEFAULT);
    assert (dset >= 0);

    // create a buffer to hold one domain worth of data
    hsize_t domain_size = 
        attr.domain_dims[0] * attr.domain_dims[1] * attr.domain_dims[2];
    float *buffer = (float *)malloc(sizeof(float) * domain_size);
    cout << "buffer: " << buffer << endl;

/*
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
                    float original_mpi_rank = 
                        processor_i * attr.processor_dims[1] 
                        * attr.processor_dims[2] 
                        + processor_j * attr.processor_dims[2] 
                        + processor_k;
                    cout << "checking time step #" << t << " / processor @ ( " 
                         << processor_i << ", " << processor_j << ", " 
                         << processor_k << " )" << " with original rank #" 
                         << original_mpi_rank << endl;
*/

        // calculate offsets from MPI rank
        hsize_t start[4];
        start[3] = (hsize_t) mpi_rank % attr.processor_dims[2];
        start[2] = (hsize_t) ((mpi_rank - start[3])/attr.processor_dims[2]) % attr.processor_dims[1];
        start[1] = (hsize_t) ((mpi_rank - start[3])/attr.processor_dims[2] - start[2]) / attr.processor_dims[1];
        start[0] = 0; // because we can only do single time step with current subfiling implementation

        cout << "Got process #" << mpi_rank << " starting at " << start[1] << ", " << start[2] << ", " << start[3] << "]" << endl;

                    // get the dataspace
                    hid_t fspace = H5Dget_space(dset);
                    assert(fspace >= 0);
//                    hsize_t start[4] = {t, processor_i * attr.domain_dims[0], processor_j * attr.domain_dims[1], processor_k * attr.domain_dims[2]};
                    hsize_t stride[4] = {1,1,1,1};
                    hsize_t count[4] = {1,1,1,1};
                    hsize_t block[4] = {1, attr.domain_dims[0], attr.domain_dims[1], attr.domain_dims[2]};
if (mpi_rank == 0) cout << "Got block size [" << block[1] << ", " << block[2] << ", " << block[3] << "]" << endl;

                    // select hyperslab within file dataspace
                    assert ( H5Sselect_hyperslab( fspace, H5S_SELECT_SET, start, stride, count, block ) >= 0 );

                    hid_t mspace = H5Screate_simple(4, block, NULL);
                    assert (mspace >= 0);
                    assert (H5Sselect_all(mspace) >= 0);

                    // read the dataset into the buffer
                    assert( H5Dread (dset, H5T_NATIVE_FLOAT, mspace, fspace, H5P_DEFAULT, buffer) >= 0);
                    
                    /*
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
                            */
//                    attr.~seismCoreAttributes(); // will finalize/dispose of internal H5 resources
                    attr.finalize(); // will finalize/dispose of internal H5 resources

    H5Sclose(fspace);
    H5Sclose(mspace);

                    /*
                } // end loop over k, j, i, t
*/

    H5Dclose(dset);
    H5Fclose(file);

    MPI_Finalize();
    // destructor gets called, tries to H5Tclose , but file is already closed
}

