// seism-core.cc

#include "hdf5.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <cstring>
#include <stdio.h>

using namespace std;

int main(int argc, char** argv)
{
    string parameter;
    std::string rest_of_line;
    unsigned int time, processor[3], chunk[3], domain[3];
    string fname = "seism-test.h5";
    string dname = "seism-data";
    int mpi_collective_io_int = 0;

    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // rank 0 reads the input file, then broadcasts
    if (rank==0) while (true){
        std::cin >> parameter;
        if (parameter.at(0) == '#') continue; // ignore as comment
        if (parameter.at(0) == 0) continue; // ignore empty line
        if (!parameter.compare("DONE")) break; // exit
        if (!parameter.compare("processor")) std::cin >> processor[0] >> processor[1] >> processor[2];
        if (!parameter.compare("chunk")) std::cin >> chunk[0] >> chunk[1] >> chunk[2];
        if (!parameter.compare("domain")) std::cin >> domain[0] >> domain[1] >> domain[2];
        if (!parameter.compare("time")) std::cin >> time;
        if (!parameter.compare("use_collective")) mpi_collective_io_int = true;
        std::getline(std::cin, rest_of_line); // read the rest of the line
    }
    
    MPI_Bcast(&time, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&processor, 3, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&chunk, 3, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&domain, 3, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mpi_collective_io_int, 1, MPI_INT, 0, MPI_COMM_WORLD);
    bool coll_flg = (bool)mpi_collective_io_int; // flag for collective I/O

    // check the arguments
    assert(time > 0);
    assert(processor[0]*processor[1]*processor[2] == (hsize_t) size);
    assert(processor[0] > 1 && processor[1] > 1 && processor[2] > 1);

    if (rank == 0) {
        cout << "Number of processes:\t" << size << endl;
        cout << "Process layout:\t\t" << processor[0] << " x " << processor[1] << " x " << processor[2] << endl;
        cout << "Per process grid:\t" << domain[0] << " x " << domain[1] << " x " << domain[2] << endl;
        cout << "Chunk dimensions:\t" << chunk[0] << " x " << chunk[1] << " x " << chunk[2] << endl;
        cout << "Collective I/O:\t\t" << coll_flg << endl;
    }

    // create the file
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);
    // use the latest file format
    assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >= 0);
    // use MPI-IO
    assert(H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL) >= 0);
    hid_t file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
    assert(file >= 0);
    assert(H5Pclose(fapl) >= 0);

    // create the dataset
    hsize_t dims[H5S_MAX_RANK];
    dims[0] = time; dims[1] = processor[0]*domain[0]; dims[2] = processor[1]*domain[1]; dims[3] = processor[2]*domain[2];
    hid_t fspace = H5Screate_simple(4, dims, NULL);
    assert(fspace >= 0);

    // set up chunking
    hsize_t cdims[H5S_MAX_RANK];
    cdims[0] = 1; cdims[1] = chunk[0]; cdims[2] = chunk[1]; cdims[3] = chunk[2];

    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    assert(dcpl >= 0);
    assert(H5Pset_chunk(dcpl, 4, cdims) >= 0);

    hid_t dset = H5Dcreate2(file, dname.c_str(), H5T_IEEE_F32LE, fspace, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    assert(dset >= 0);

    // initialize the test data
    vector<float> v((size_t) domain[0]*domain[1]*domain[2], (float) rank);

    hsize_t start[4], block[4], count[4] = {1,1,1,1}; 
    // calculate offsets from RANK
    start[3] = (hsize_t) rank % processor[2];
    start[2] = (hsize_t) ((rank - start[3])/processor[2]) % processor[1];
    start[1] = (hsize_t) ((rank - start[3])/processor[2] - start[2]) / processor[1];

    start[1] *= domain[0];
    start[2] *= domain[1];
    start[3] *= domain[2];

    block[0] = 1;
    block[1] = domain[0];
    block[2] = domain[1];
    block[3] = domain[2];

    hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
    assert(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) >= 0);

    vector<double> tstamps(time + 1);

    dims[0] = 1; dims[1] = domain[0]; dims[2] = domain[1]; dims[3] = domain[2];
    hid_t mspace = H5Screate_simple(4, dims, NULL);
    assert(mspace >= 0);
    assert(H5Sselect_all(mspace) >= 0);

    // start the time stepping
    for (size_t it = 0; it < time; ++it) {
        tstamps[it] = MPI_Wtime();

        start[0] = (hsize_t) it;

        assert(H5Sselect_hyperslab(fspace, H5S_SELECT_SET, start, NULL, count, block) >= 0);

        if (coll_flg) assert(H5Dwrite(dset, H5T_NATIVE_FLOAT, mspace, fspace, dxpl, &v[0]) >= 0);
        else assert(H5Dwrite(dset, H5T_NATIVE_FLOAT, mspace, fspace, H5P_DEFAULT, &v[0]) >= 0);
    }

    assert(H5Sclose(mspace) >= 0);
    assert(H5Pclose(dxpl) >= 0);
    assert(H5Sclose(fspace) >= 0);
    assert(H5Pclose(dcpl) >= 0);
    assert(H5Dclose(dset) >= 0);
    assert(H5Fclose(file) >= 0);

    MPI_Barrier(MPI_COMM_WORLD);

    tstamps[time] = MPI_Wtime();

    // print timings/throughput
    size_t bytes_written = time * processor[0] * domain[0] * processor[1] * domain[1] *  processor[2] * domain[2] * sizeof(float);
    if (rank == 0) cout << "Aggregate throughput:\t" << bytes_written / (tstamps[time] - tstamps[0]) / ((double) (1<<20)) << " MB/s" << endl;
    MPI_Finalize();

    return 0;
}
