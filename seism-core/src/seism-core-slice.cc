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

#include <cstdlib>
#ifdef INCLUDE_ZFP
#include <H5Zzfp.h>
#endif
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <dlfcn.h>
#include <cstring>

#include "seism-core-attributes.hh"

using namespace std;

#define CHUNKED_DSET_NAME "chunked"

#if ! ( (H5_VERS_MAJOR == 1) && (H5_VERS_MINOR >= 9) )   

herr_t H5Pset_all_coll_metadata_ops(hid_t fapl, hbool_t true_or_false)
{
    cout << "set_collective_metadata option only available with HDF5 version 1.10+\n" << endl;
    return NULL;
}

herr_t H5Pget_all_coll_metadata_ops(hid_t dapl, hbool_t *actual_metadata_ops_collective )
{
    *actual_metadata_ops_collective = false;
    return NULL; 
}

#endif

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
    unsigned int simulation_time;
    hsize_t processor[3], chunk[3], domain[3];
    int collective_write = 0;
    int precreate = 0;
    int set_collective_metadata = 0;
    int never_fill = 0;
    int deflate = 0;
    int subfile = 0;
    int n_nodes = 0;
    char use_function_lib[256];
    use_function_lib[0] = 0; // truncate any junk string in auto var
    char use_function_name[256];
    use_function_name[0] = 0; // if no param passed, then strcmp test will fail
    int use_function_argc = 0;
    string use_function_argv_string;
    char use_function_argv[256];
    use_function_argv[0] = 0; // truncate any junk string in auto var
	int zfp = 0;
    const char *use_function_argv_c_str = NULL;

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
            if (!parameter.compare("deflate"))
              cin >> deflate;
            if (!parameter.compare("subfile"))
              cin >> subfile;
            if (!parameter.compare("n_nodes"))
              cin >> n_nodes;
            if (!parameter.compare("use_function_lib"))
              cin >> use_function_lib;
            if (!parameter.compare("use_function_name"))
              cin >> use_function_name;
            if (!parameter.compare("use_function_argc"))
              cin >> use_function_argc;
            if (!parameter.compare("use_function_argv"))
            {
                // read the rest of the line
                getline(cin, use_function_argv_string);
                // extract a C-string to pass via MPI
                use_function_argv_c_str = use_function_argv_string.c_str(); 
                strcpy(use_function_argv, use_function_argv_c_str);
                continue;
            }
            if (!parameter.compare("zfp"))
              cin >> zfp;
            getline(cin, rest_of_line); // read the rest of the line
        }
    }

    assert(MPI_Bcast(&simulation_time, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&processor, 3, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&chunk, 3, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&domain, 3, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&collective_write, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&precreate, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&set_collective_metadata, 1, MPI_INT, 0, MPI_COMM_WORLD) 
           == MPI_SUCCESS);
    assert(MPI_Bcast(&never_fill, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&deflate, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&subfile, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&n_nodes, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
           MPI_SUCCESS);
    assert(MPI_Bcast(&use_function_lib, 256, MPI_CHAR, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&use_function_name, 256, MPI_CHAR, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&use_function_argc, 1, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&use_function_argv, 256, MPI_CHAR, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&deflate, 1, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Bcast(&zfp, 1, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    // check the arguments
    assert(time > 0);
    assert(processor[0]*processor[1]*processor[2] == (hsize_t) mpi_size);
    // I'm removing the below restriction to allow for serial case
    // assert(processor[0] > 1 && processor[1] > 1 && processor[2] > 1);

    // Subfiling and chunking not compatible, so ignore chunk info
    if (!subfile) assert(chunk[0] > 1 && chunk[1] > 1 && chunk[2] > 1);
    assert(domain[0] > 0 && domain[1] > 0 && domain[2] > 0);

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
        if (!subfile) cout << "Chunk dimensions:\t\t" << chunk[0] << " x " 
            << chunk[1] << " x " << chunk[2] << endl;
        if (n_nodes) cout << "n_nodes:\t\t\t" << n_nodes << endl;
        cout << "Number of time steps:\t\t" << simulation_time << endl;
        cout << "Pre-create:\t\t\t" << precreate << endl;
        cout << "Collective I/O:\t\t\t" << collective_write << endl;
        cout << "Collective metadata requested:\t" << set_collective_metadata 
            << endl;
        cout << "H5D_FILL_TIME_NEVER set:\t" << never_fill << endl;
        cout << "Deflate: \t\t\t" << deflate << endl;
        cout << "Subfile: \t\t\t" << subfile << endl;
        cout << "ZFP: \t\t\t\t" << zfp << endl;
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
    // subfiling not compatible with chunking
    if (!subfile) assert(H5Pset_chunk(dcpl, n_dims, cdims) >= 0);
    if (never_fill) assert(H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER ) >= 0);
    assert(H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_EARLY) >= 0);
    if (deflate != 0) assert(H5Pset_deflate (dcpl, deflate) >= 0);
    
	// ZFP
	size_t cd_nelmts = 4;
	unsigned int cd_values[] = {3, 0, 0, 0};
#ifdef INCLUDE_ZFP
    if (zfp != 0) 
	{
		assert(H5Z_zfp_initialize() >= 0);
		assert(H5Pset_filter(dcpl, H5Z_FILTER_ZFP, H5Z_FLAG_MANDATORY,cd_nelmts, cd_values) >= 0);
	}
#endif

    hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
    assert(dapl >= 0);
    ///////////////////////////////////////////////////////////////////////////
    // prepare hyperslab selection
    hsize_t start[4], block[4], count[4] = {1,1,1,1};

    // calculate offsets from MPI rank
    start[3] = (hsize_t) mpi_rank % processor[2];
    start[2] = (hsize_t) ((mpi_rank - start[3])/processor[2]) % processor[1];
    start[1] = (hsize_t) ((mpi_rank - start[3])/processor[2] - start[2]) /
      processor[1];
    hsize_t domain_block_number[3];
    domain_block_number[0] = start[1];
    domain_block_number[1] = start[2];
    domain_block_number[2] = start[3];
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

    // if we're loading a function, use it here, now.
    // function will receive mpi_rank, argc, argv

    if (strcmp(use_function_name, "") != 0) {

        void *handle;
        // This awkward cast provided to you by the ISO standards team...
        float (*use_function)(int, hsize_t*, hsize_t*, hsize_t*, hsize_t*, int,
                char **);
        char *error;

        handle = dlopen (use_function_lib, RTLD_LAZY);
        if (!handle) {
            fputs (dlerror(), stderr);
            exit(1);
        }

        *(void **) (&use_function) = dlsym(handle, use_function_name);
        if ((error = dlerror()) != NULL)  {
            fputs(error, stderr);
            exit(1);
        }

        // split use_function_argv into tokens, then pass to library function
        
        const char *array[16];
        for (int i = 0; i < use_function_argc; i++) {
            if (i == 0) array[i] = strtok(use_function_argv, " ");
            else array[i] = strtok(NULL, " ");
        }   

        hsize_t position_in_block[3];
        for (position_in_block[0] = 0; position_in_block[0] < domain[0]; position_in_block[0]++)
        for (position_in_block[1] = 0; position_in_block[1] < domain[1]; position_in_block[1]++)
        for (position_in_block[2] = 0; position_in_block[2] < domain[2]; position_in_block[2]++)
        {
            hsize_t index = position_in_block[0] * domain[1] * domain[2] + position_in_block[1] * domain[2] + position_in_block[2];
            v[index] = (*use_function)(mpi_rank, processor, domain, domain_block_number, position_in_block, use_function_argc, (char **)array);
        }

        dlclose(handle);
    }
  
    // create the fapl
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);

    // use the latest file format
    assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >=
         0);

    // set collective metadata reads
    if (set_collective_metadata)
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

    if (subfile) 
    {
        MPI_Comm comm;
        char subfile_name[256];

        // split by color
        int color = mpi_rank % subfile;
        // if (n_nodes > subfile) color = mpi_rank % n_nodes;
cout << mpi_rank << " has color = " << color << " and subfile = " << subfile << endl;
        MPI_Comm_split (MPI_COMM_WORLD, color, mpi_rank, &comm);
cout << mpi_rank << " has write comm = " << comm << endl;
        sprintf(subfile_name, "Subfile_%d.h5", color);
        assert(H5Pset_subfiling_access(fapl, subfile_name, comm, MPI_INFO_NULL) >= 0); 
        
        // select hyperslab for subfiling, superset of selection for writing.
        hsize_t subfiling_block[] = {simulation_time, domain[0], domain[1], domain[2]};
        hsize_t subfiling_start[] = {0, start[1], start[2], start[3]};
        assert(H5Sselect_hyperslab(fspace, H5S_SELECT_SET, subfiling_start, NULL, count, subfiling_block) >= 0);
        assert (H5Pset_subfiling_selection(dapl, fspace) >= 0); 
    }

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

    // get storage size before closing dataset
    hsize_t storage_size = H5Dget_storage_size(dset_chunked);

    assert(H5Dclose(dset_chunked) >= 0);
    assert(H5Pclose(fapl) >= 0);
    assert(H5Sclose(mspace) >= 0);
    assert(H5Pclose(dxpl) >= 0);
    assert(H5Sclose(fspace) >= 0);
    assert(H5Pclose(dcpl) >= 0);

#ifdef INCLUDE_ZFP
    if (zfp != 0) assert(H5Z_zfp_finalize() >= 0);
#endif

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
		cout << "Total bytes written:\t\t" << bytes_written << endl; 
		cout << "Compressed size in bytes:\t" << storage_size << endl; 

    }

    if (mpi_rank == 0)
    {
        // re-open the file and write the simulation attributes
        file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        assert (file >= 0);
        char* argv_junk = (char*)use_function_argv_c_str;
        seismCoreAttributes attr((char*)"my_attr", processor, chunk, domain, 
                simulation_time, n_nodes, subfile, collective_write, precreate, 
                set_collective_metadata, never_fill, deflate, zfp,
                use_function_lib, use_function_name, use_function_argc, 
                argv_junk );
        attr.writeAttributesToFile(file);
        assert(H5Fclose(file) >=0);
    }

    MPI_Finalize();

    return 0;
}

