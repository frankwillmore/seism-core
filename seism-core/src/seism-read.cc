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

    /*
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
    */

    // open the dataset
    hid_t dset = H5Dopen (file, "chunked", H5P_DEFAULT);
    assert (dset >= 0);

    // create a buffer to hold one domain worth of data
    hsize_t domain_size = 
        attr.domain_dims[0] * attr.domain_dims[1] * attr.domain_dims[2];
    float *buffer = (float *)malloc(sizeof(float) * domain_size);

    // calculate offsets from MPI rank
    hsize_t start[4];
    start[3] = (hsize_t) mpi_rank % attr.processor_dims[2];
    start[2] = (hsize_t) ((mpi_rank - start[3])/attr.processor_dims[2]) % attr.processor_dims[1];
    start[1] = (hsize_t) ((mpi_rank - start[3])/attr.processor_dims[2] - start[2]) / attr.processor_dims[1];
    start[0] = 0; // because we can only do single time step with current subfiling implementation

//        cout << "Got process #" << mpi_rank << " starting at " << start[1] << ", " << start[2] << ", " << start[3] << "]" << endl;

    // get the dataspace
    hid_t fspace = H5Dget_space(dset);
    assert(fspace >= 0);
    hsize_t stride[4] = {1,1,1,1};
    hsize_t count[4] = {1,1,1,1};
    hsize_t block[4] = {1, attr.domain_dims[0], attr.domain_dims[1], attr.domain_dims[2]};

//if (mpi_rank == 0) cout << "Got block size [" << block[1] << ", " << block[2] << ", " << block[3] << "]" << endl;

    // select hyperslab within file dataspace
    assert ( H5Sselect_hyperslab( fspace, H5S_SELECT_SET, start, stride, count, block ) >= 0 );

    hid_t mspace = H5Screate_simple(4, block, NULL);
    assert (mspace >= 0);
    assert (H5Sselect_all(mspace) >= 0);

    // read the dataset into the buffer
    double begin_read = MPI_Wtime();
    assert( H5Dread (dset, H5T_NATIVE_FLOAT, mspace, fspace, H5P_DEFAULT, buffer) >= 0);
    double end_read = MPI_Wtime();

    double _read_time = end_read - begin_read;
    double read_time;

    //MPI_Reduce( void* send_data, void* recv_data, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm communicator);
    MPI_Reduce(&_read_time, &read_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (mpi_rank == 0){
        cout << "Read time: \t\t\t" << read_time << endl;
        unsigned long data_size = sizeof(float) * mpi_size * domain_size;
        double throughput_MB = (1.0e-6 * data_size) / read_time;
        cout << "Throughput: \t\t\t" << throughput_MB << " MB/s" << endl;
    }
    
    attr.finalize(); // will finalize/dispose of internal H5 resources
    H5Sclose(fspace);
    H5Sclose(mspace);
    H5Dclose(dset);
    H5Fclose(file);

    MPI_Finalize();
}

