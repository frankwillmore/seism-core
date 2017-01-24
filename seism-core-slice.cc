///////////////////////////////////////////////////////////////////////////////
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
// domain 180 64 64
// time 2
// collective_write
// precreate
// set_collective_metadata
// never_fill
// DONE
// EOF
//
//////////////////////////////////////////////////////////////////////////////

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
  
void precreate_0
(
    const string& fname,
    hid_t         fspace,
    hid_t         dcpl
)
{
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);
    assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >=
           0);

    hid_t file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
    assert(file >= 0);
    hid_t dset = H5Dcreate(file, CHUNKED_DSET_NAME, H5T_IEEE_F32LE, fspace,
                           H5P_DEFAULT, dcpl, H5P_DEFAULT);
    assert(dset >= 0);
    assert(H5Dclose(dset) >= 0);
    assert(H5Fclose(file) >= 0);
    assert(H5Pclose(fapl) >= 0);
}

///////////////////////////////////////////////////////////////////////////////

void setMPI_Info(MPI_Info& info, const size_t& v_size, int mpi_size)
{
    assert(MPI_Info_create(&info) == MPI_SUCCESS);
    assert(MPI_Info_set( info, "romio_cb_write", "enable" ) == MPI_SUCCESS);
    assert(MPI_Info_set( info, "romio_ds_write", "disable" ) == MPI_SUCCESS);

    ostringstream ost;
    ost << v_size * sizeof(float);
    assert(MPI_Info_set( info, "cb_buffer_size", ost.str().c_str()) 
          == MPI_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    double begin = MPI_Wtime();

    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);


    // rank 0 reads the input file, then broadcasts
    string parameter, rest_of_line;
    unsigned int simulation_time, processor[3], chunk[3], domain[3];
    int collective_write = 0;
    int precreate = 0;
    int set_collective_metadata = 0;
    int never_fill = 0;

    if (mpi_rank==0)
    {
        while (true)
        {
            cin >> parameter;
            if (parameter.at(0) == '#') continue; // ignore as comment
            if (parameter.at(0) == 0) continue; // ignore empty line
            if (!parameter.compare("DONE")) break; // exit
            if (!parameter.compare("processor"))
              cin >> processor[0] >> processor[1] >> processor[2];
            if (!parameter.compare("chunk"))
              cin >> chunk[0] >> chunk[1] >> chunk[2];
            if (!parameter.compare("domain"))
              cin >> domain[0] >> domain[1] >> domain[2];
            if (!parameter.compare("time"))
              cin >> simulation_time;
            if (!parameter.compare("collective_write"))
              collective_write = true;
            if (!parameter.compare("precreate"))
              precreate = true;
            if (!parameter.compare("set_collective_metadata"))
              set_collective_metadata = true;
            if (!parameter.compare("never_fill"))
              never_fill = true;
            getline(cin, rest_of_line); // read the rest of the line
        }
    }

    assert(MPI_Bcast(&simulation_time, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&processor, 3, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&chunk, 3, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&domain, 3, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&collective_write, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&precreate, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&set_collective_metadata, 1, MPI_INT, 0, MPI_COMM_WORLD) 
           == MPI_SUCCESS);
    assert(MPI_Bcast(&never_fill, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);

    // check the arguments
    assert(time > 0);
    assert(processor[0]*processor[1]*processor[2] == (hsize_t) mpi_size);
    assert(processor[0] > 1 && processor[1] > 1 && processor[2] > 1);
    assert(chunk[0] > 1 && chunk[1] > 1 && chunk[2] > 1);
    assert(domain[0] > 1 && domain[1] > 1 && domain[2] > 1);

    if (mpi_rank == 0)
    {
        cout << 
        "====================================================================="
        << endl;
        cout << "Number of processes:\t\t" << mpi_size << endl;
        cout << "Process layout:\t\t\t" << processor[0] << " x " <<
          processor[1] << " x " << processor[2] << endl;
        cout << "Per process grid:\t\t" << domain[0] << " x " << domain[1] <<
          " x " << domain[2] << endl;
        cout << "Chunk dimensions:\t\t" << chunk[0] << " x " << chunk[1] <<
          " x " << chunk[2] << endl;
        cout << "Number of time steps:\t\t" << simulation_time << endl;
        cout << "Pre-create:\t\t\t" << precreate << endl;
        cout << "Collective I/O:\t\t\t" << collective_write << endl;
        cout << "Collective metadata requested:\t" << set_collective_metadata 
            << endl;
        cout << "H5D_FILL_TIME_NEVER set:\t" << never_fill << endl;
        cout << endl;
    }

    //////////////////////////////////////////////////////////////////////////
    // create the fle dataspace, time dimension first!
    hsize_t n_dims = 4;
    hsize_t dims[H5S_MAX_RANK];

    dims[0] = simulation_time;
    dims[1] = processor[0]*domain[0];
    dims[2] = processor[1]*domain[1];
    dims[3] = processor[2]*domain[2];

    hid_t fspace = H5Screate_simple(n_dims, dims, NULL);
    assert(fspace >= 0);

    // set up chunking... NOTE: extent of time dimension is 1

    hsize_t cdims[H5S_MAX_RANK];
    cdims[0] = 1;
    cdims[1] = chunk[0];
    cdims[2] = chunk[1];
    cdims[3] = chunk[2];

    // create dcpl and set properties
    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    assert(dcpl >= 0);
    assert(H5Pset_chunk(dcpl, n_dims, cdims) >= 0);
    if (never_fill) assert(H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER ) >= 0);
    assert(H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_EARLY) >= 0);

    hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
    assert(dapl >= 0);
    ///////////////////////////////////////////////////////////////////////////
    // prepare hyperslab selection, use max dims, can ignore 4th as needed
    hsize_t start[4], block[4], count[4] = {1,1,1,1};

    // calculate offsets from MPI rank
    start[3] = (hsize_t) mpi_rank % processor[2];
    start[2] = (hsize_t) ((mpi_rank - start[3])/processor[2]) % processor[1];
    start[1] = (hsize_t) ((mpi_rank - start[3])/processor[2] - start[2]) /
      processor[1];
    start[1] *= domain[0];
    start[2] *= domain[1];
    start[3] *= domain[2];

    block[0] = 1;
    block[1] = domain[0];
    block[2] = domain[1];
    block[3] = domain[2];

    ///////////////////////////////////////////////////////////////////////////
    // data transfer property list for collective I/O, if selected
    hid_t dxpl = H5P_DEFAULT;
    if (collective_write)
    {
        dxpl = H5Pcreate(H5P_DATASET_XFER);
        assert(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) >= 0);
    }

    ///////////////////////////////////////////////////////////////////////////
    // create in-memory dataspace
    dims[0] = 1;
    dims[1] = domain[0];
    dims[2] = domain[1];
    dims[3] = domain[2];

    hid_t mspace = H5Screate_simple(n_dims, dims, NULL);
    assert(mspace >= 0);
    assert(H5Sselect_all(mspace) >= 0);

    ///////////////////////////////////////////////////////////////////////////
    // initialize the test data to MPI rank
    vector<float> v((size_t) domain[0]*domain[1]*domain[2], (float) mpi_rank);
  
    // create the fapl
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);

    // use the latest file format
    assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >=
         0);

    // set collective metadata reads
    if ((H5_VERS_MAJOR == 1) && (H5_VERS_MINOR >= 10) 
            && set_collective_metadata)
    {
        assert(H5Pset_all_coll_metadata_ops(fapl, true) >=0 );
        assert(H5Pset_all_coll_metadata_ops(dapl, true) >=0 );
    }

    MPI_Info info;
    if (collective_write)
    {
        setMPI_Info(info, v.size(), mpi_size);
    }
    else
    {
        info = MPI_INFO_NULL;
    }
    assert(H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, info) >= 0);

    // file handle and name for file which will be created
    string fname = "seism-test.h5";
    hid_t file, dset_chunked;

    MPI_Barrier(MPI_COMM_WORLD);

    ///////////////////////////////////////////////////////////////////////////
    // precreate datasets, as needed
    double start_create = MPI_Wtime();
    double create_1=0.0f, create_2=0.0f, create_3=0.0f;

    if (precreate)
    {
        if (mpi_rank == 0) // create with process 0, then close & re-open
        {
            precreate_0(fname, fspace, dcpl);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        create_1 = MPI_Wtime();
        file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, fapl);
        assert (file >= 0);
        MPI_Barrier(MPI_COMM_WORLD);
        create_2 = MPI_Wtime();
        dset_chunked = H5Dopen(file, CHUNKED_DSET_NAME, dapl);
        assert(dset_chunked >= 0);
        MPI_Barrier(MPI_COMM_WORLD);
        create_3 = MPI_Wtime();
    }
    else
    {
        file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
        assert(file >= 0);
        dset_chunked = H5Dcreate(file, CHUNKED_DSET_NAME, H5T_IEEE_F32LE, 
                fspace, H5P_DEFAULT, dcpl, dapl);
        assert(dset_chunked >= 0);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double stop_create = MPI_Wtime();

    ///////////////////////////////////////////////////////////////////////////
    // write the chunked dataset

    MPI_Barrier(MPI_COMM_WORLD);
    double start_chunked = MPI_Wtime();

    for (size_t it = 0; it < simulation_time; ++it)
    {
        start[0] = (hsize_t) it;
        assert(H5Sselect_hyperslab(fspace, H5S_SELECT_SET, start, NULL, count,
                                   block) >= 0);
        assert(H5Dwrite(dset_chunked, H5T_NATIVE_FLOAT, mspace, fspace, dxpl,
                        &v[0]) >= 0);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double stop_chunked = MPI_Wtime();

    ///////////////////////////////////////////////////////////////////////////

    assert(H5Dclose(dset_chunked) >= 0);
    assert(H5Pclose(fapl) >= 0);
    assert(H5Sclose(mspace) >= 0);
    assert(H5Pclose(dxpl) >= 0);
    assert(H5Sclose(fspace) >= 0);
    assert(H5Pclose(dcpl) >= 0);
    // verify that metadata ops actually performed collectively
    hbool_t actual_metadata_ops_collective;
    H5Pget_all_coll_metadata_ops( dapl, &actual_metadata_ops_collective );
    assert(H5Pclose(dapl) >= 0);

    MPI_Barrier(MPI_COMM_WORLD);
    double fclose_start = MPI_Wtime();

    assert(H5Fclose(file) >= 0);

    MPI_Barrier(MPI_COMM_WORLD);
    double fclose_stop = MPI_Wtime();

    if (mpi_rank == 0)
    {
        size_t bytes_written = simulation_time * processor[0] * domain[0] *
          processor[1] * domain[1] *  processor[2] * domain[2] * sizeof(float);

        if (precreate)
        {
            cout << "Pre-c";
        }
        else
        {
            cout << "C";
        }
        cout << "reate/open:\t\t";
        if (!precreate) { cout << "\t";}
        cout << (stop_create - start_create) << " s" << endl;
        if (precreate)
        {
            cout << "Time in precreate_0():\t\t"  << (create_1 - start_create) 
                 << " s" << endl;
            cout << "Time in H5Fopen():\t\t"  << (create_2 - create_1) << " s" 
                 << endl;
            cout << "Time in H5Dopen():\t\t"  << (create_3 - create_2) 
                 << " s" << endl;
        }
        cout << "Write:\t\t\t\t" <<
          (stop_chunked - start_chunked) << " s" << endl;
        cout << "Write throughput:\t\t" << bytes_written /
          (stop_chunked - start_chunked) / ((double) (1<<20)) << " MB/s"
             << endl;
        cout << "Close file:\t\t\t" << (fclose_stop - fclose_start) << " s"
             << endl;
        cout << "Aggregate throughput:\t\t" << bytes_written /
          (fclose_stop - begin) / ((double) (1<<20)) << " MB/s"
             << endl;
        cout << "Mdata ops actually collective:\t" 
             << actual_metadata_ops_collective
             << endl;

    }

    if (mpi_rank == 0)
    {
        // re-open the file and write the simulation attributes
        file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        assert (file >= 0);
        seismCoreAttributes attr((char*)"my_attr", processor, chunk, domain, 
                simulation_time, collective_write, precreate, 
                set_collective_metadata, never_fill);
        attr.writeAttributesToFile(file);
        assert(H5Fclose(file) >=0);
    }

    MPI_Finalize();

    return 0;
}

