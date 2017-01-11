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
// domain 180 64 64
// time 2
// collective_write
// precreate
// set_collective_metadata
// early_allocation
// never_fill
// DONE
// EOF
//
////////////////////////////////////////////////////////////////////////////

#include "hdf5.h"

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "seism-core-attributes.hh"

using namespace std;

#define CHUNKED_DSET_NAME "chunked"

////////////////////////////////////////////////////////////////////////////////
  
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
    seismCoreAttributes attr(file);

    cout << attr.name << endl;

}

/*






      cout << "================================================================================" << endl;
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
      cout << "Early allocation:\t\t" << early_allocation << endl;
      cout << "H5D_FILL_TIME_NEVER set:\t" << never_fill << endl;
      cout << endl;






  // create the fle dataspace, time dimension first!
  hsize_t n_dims = 4;
  hsize_t dims[H5S_MAX_RANK];

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
  if (early_allocation) 
      assert(H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_EARLY) >= 0);

  hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
  assert(dapl >= 0);
  //////////////////////////////////////////////////////////////////////////////
  // prepare hyperslab selection, use max dims, can ignore 4th as needed
  hsize_t start[4], block[4], count[4] = {1,1,1,1};

  //////////////////////////////////////////////////////////////////////////////
  // create in-memory dataspace
  dims[0] = 1;
  dims[1] = domain[0];
  dims[2] = domain[1];
  dims[3] = domain[2];

  hid_t mspace = H5Screate_simple(n_dims, dims, NULL);
  assert(mspace >= 0);
  assert(H5Sselect_all(mspace) >= 0);

  // use the latest file format
  assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >=
         0);

  // set collective metadata reads
  if ((H5_VERS_MAJOR == 1) && (H5_VERS_MINOR >= 10) && set_collective_metadata)
    {
      assert(H5Pset_all_coll_metadata_ops(fapl, true) >=0 );
      assert(H5Pset_all_coll_metadata_ops(dapl, true) >=0 );
    }

  // file handle and name for file which will be created
  string fname = "seism-test.h5";
  hid_t file, dset_chunked;



  //////////////////////////////////////////////////////////////////////////////

  assert(H5Dclose(dset_chunked) >= 0);
  assert(H5Pclose(fapl) >= 0);
  assert(H5Sclose(mspace) >= 0);
  assert(H5Pclose(dxpl) >= 0);
  assert(H5Sclose(fspace) >= 0);
  assert(H5Pclose(dcpl) >= 0);
  assert(H5Fclose(file) >= 0);


  if (mpi_rank == 0)
    {
      // re-open the file and write the simulation attributes
      // create the fapl
      hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
      assert(fapl >= 0);
      assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >= 0);
      file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, fapl);
      //file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
      assert (file >= 0);
      cout << "here" <<endl;
      seismCoreAttributes attr((char*)"my_attr", processor, domain, chunk, simulation_time, collective_write, precreate, set_collective_metadata, early_allocation, never_fill);
      cout << "here2" <<endl;
      attr.writeAttributesToFile(file);
    }

  return 0;
}
*/
