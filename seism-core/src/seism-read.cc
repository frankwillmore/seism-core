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

#include <cstring>
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
    MPI_Comm comm;
    char subfile_name[256];

    if (argc < 2) {
        cout << "No filename specified\n" ;
        MPI_Finalize();
        return(1);
    }
    char* filename = argv[1];

    if (mpi_rank == 0){
        cout << endl
             << "=====================================================================" 
             << endl;
        cout << "Reading " << filename << endl;
    }
    
    // open file to read the attributes
    hid_t file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    seismCoreAttributes attr(file);
    if (mpi_rank == 0){
        cout << endl;
        cout << "processor layout:\t\t";
        cout << attr.processor_dims[0] << ":";
        cout << attr.processor_dims[1] << ":";
        cout << attr.processor_dims[2] << endl;
        cout << "domain dims:\t\t\t";
        cout << attr.domain_dims[0] << ":";
        cout << attr.domain_dims[1] << ":";
        cout << attr.domain_dims[2] << endl;
        cout << endl;
    }
    
    unsigned int subfile = attr.subfile;
    for (int i = 2; i<argc; i++) if (!strcmp(argv[i], "--ignore-subfile")) {
            subfile = 0;
            if (mpi_rank == 0) cout << "Ignoring subfiling..." << endl;
        }

    // if subfiling was done, close and re-open the file
    if (subfile) {

#ifdef H5_SUBFILING

        MPI_Comm comm = MPI_COMM_WORLD;
        MPI_Info info = MPI_INFO_NULL;

        // close the file now that we have metadata and re-open later for read
        H5Fclose(file);

        // set up property list and communicator
        hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
        assert (fapl_id >= 0);
        assert (H5Pset_fapl_mpio(fapl_id, comm, info) >= 0);

        // split by color
        int color = mpi_rank % attr.subfile;
        // group io to same node
        if (attr.n_nodes > attr.subfile) color = (mpi_rank % attr.n_nodes) % attr.subfile;
        MPI_Comm_split (MPI_COMM_WORLD, color, mpi_rank, &comm);
        sprintf(subfile_name, "Subfile_%d.h5", color);
        cout << "reading subfiled file:\t\t" << subfile_name << endl;

        // now open file again, with subfiling enabled
        H5Pset_subfiling_access(fapl_id, subfile_name, comm, MPI_INFO_NULL);
        file = H5Fopen(filename, H5F_ACC_RDONLY, fapl_id);
        assert (file >= 0);
        assert(H5Pclose(fapl_id) >= 0);
#else
        cout << "Warning:  This datafile was sub-filed but this reader was not" 
             << endl
             << "          built with subfiling feature, Sub-file awrare read" 
             << endl
             << "          is not supported and sub-filing will be ignnored." 
             << endl << endl;
#endif

    }


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

    // adjust start point by attr.domain_dims
    start[1] *= attr.domain_dims[0];
    start[2] *= attr.domain_dims[1];
    start[3] *= attr.domain_dims[2];

    // get the dataspace
    hid_t fspace = H5Dget_space(dset);
    assert(fspace >= 0);
    hsize_t stride[4] = {1,1,1,1};
    hsize_t count[4] = {1,1,1,1};
    hsize_t block[4] = {1, attr.domain_dims[0], attr.domain_dims[1], attr.domain_dims[2]};

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

    MPI_Reduce(&_read_time, &read_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (mpi_rank == 0){
        unsigned long data_size = sizeof(float) * mpi_size * domain_size;
        double throughput_MB = (1.0e-6 * data_size) / read_time;
        cout << endl;
        cout << "Total bytes read:\t\t" << data_size << endl;
        cout << "Read time: \t\t\t" << read_time << " s"  << endl;
        cout << "Read throughput: \t\t" << throughput_MB << " MB/s" << endl;
        cout << "seism-read done. " << endl << endl;
        cout << "=====================================================================" 
             << endl;
    }
    
    attr.finalize(); // will finalize/dispose of internal H5 resources
    H5Sclose(fspace);
    H5Sclose(mspace);
    H5Dclose(dset);
    H5Fclose(file);

    MPI_Finalize();
}

