////////////////////////////////////////////////////////////////////////////
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
// An example input line is shown below:
//
// mpiexec -n 8 ./seism-core << EOF
// processor 2 2 2
// chunk 180 64 64
// domain 180 64 64
// time 2
// DONE
// EOF
//
////////////////////////////////////////////////////////////////////////////

#include "hdf5.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char** argv)
{
  int size, rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // rank 0 reads the input file, then broadcasts
  string parameter;
  string rest_of_line;
  unsigned int time, processor[3], chunk[3], domain[3];
  int mpi_collective_io_int = 0;

  if (rank==0)
    {
      while (true){
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
          cin >> time;
        if (!parameter.compare("use_collective"))
          mpi_collective_io_int = true;
        getline(cin, rest_of_line); // read the rest of the line
      }
    }

  assert(MPI_Bcast(&time, 1, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
  assert(MPI_Bcast(&processor, 3, MPI_INT, 0, MPI_COMM_WORLD) ==
         MPI_SUCCESS);
  assert(MPI_Bcast(&chunk, 3, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
  assert(MPI_Bcast(&domain, 3, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
  assert(MPI_Bcast(&mpi_collective_io_int, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
         MPI_SUCCESS);
  bool coll_flg = (bool)mpi_collective_io_int; // flag for collective I/O

  // check the arguments
  assert(time > 0);
  assert(processor[0]*processor[1]*processor[2] == (hsize_t) size);
  assert(processor[0] > 1 && processor[1] > 1 && processor[2] > 1);
  assert(chunk[0] > 1 && chunk[1] > 1 && chunk[2] > 1);
  assert(domain[0] > 1 && domain[1] > 1 && domain[2] > 1);

  if (rank == 0)
    {
      cout << "\nNumber of processes:\t" << size << endl;
      cout << "Process layout:\t\t" << processor[0] << " x " <<
        processor[1] << " x " << processor[2] << endl;
      cout << "Per process grid:\t" << domain[0] << " x " << domain[1] <<
        " x " << domain[2] << endl;
      cout << "Chunk dimensions:\t" << chunk[0] << " x " << chunk[1] <<
        " x " << chunk[2] << endl;
      cout << "Number of time steps:\t" << time << endl;
      cout << "Collective I/O:\t\t" << coll_flg << endl;
    }

  // create the file
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
  assert(fapl >= 0);

  // use the latest file format
  assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >=
         0);

  // use MPI-IO
  assert(H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL) >= 0);

  // create the file
  string fname = "seism-test.h5";
  hid_t file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
  assert(file >= 0);
  assert(H5Pclose(fapl) >= 0);

  // create the dataspace
  // NOTE: time dimension first!
  hsize_t dims[H5S_MAX_RANK];
  dims[0] = time;
  dims[1] = processor[0]*domain[0];
  dims[2] = processor[1]*domain[1];
  dims[3] = processor[2]*domain[2];
  hid_t fspace = H5Screate_simple(4, dims, NULL);
  assert(fspace >= 0);

  // set up chunking
  // NOTE: extent of time dimension is 1
  hsize_t cdims[H5S_MAX_RANK];
  cdims[0] = 1;
  cdims[1] = chunk[0];
  cdims[2] = chunk[1];
  cdims[3] = chunk[2];

  hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
  assert(dcpl >= 0);
  assert(H5Pset_chunk(dcpl, 4, cdims) >= 0);

  // create the dataset
  string dname = "seism-data";
  hid_t dset = H5Dcreate2(file, dname.c_str(), H5T_IEEE_F32LE, fspace,
                          H5P_DEFAULT, dcpl, H5P_DEFAULT);
  assert(dset >= 0);

  // initialize the test data to MPI rank
  vector<float> v((size_t) domain[0]*domain[1]*domain[2], (float) rank);

  // prepare hyperslab selection
  hsize_t start[4], block[4], count[4] = {1,1,1,1};

  // calculate offsets from MPI rank
  start[3] = (hsize_t) rank % processor[2];
  start[2] = (hsize_t) ((rank - start[3])/processor[2]) % processor[1];
  start[1] = (hsize_t) ((rank - start[3])/processor[2] - start[2]) /
    processor[1];

  start[1] *= domain[0];
  start[2] *= domain[1];
  start[3] *= domain[2];

  block[0] = 1;
  block[1] = domain[0];
  block[2] = domain[1];
  block[3] = domain[2];

  // data transfer property list for collective I/O (optional)
  hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
  assert(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) >= 0);

  // create in-memory dataspace
  dims[0] = 1;
  dims[1] = domain[0];
  dims[2] = domain[1];
  dims[3] = domain[2];
  hid_t mspace = H5Screate_simple(4, dims, NULL);
  assert(mspace >= 0);
  assert(H5Sselect_all(mspace) >= 0);

  // start the time stepping

  vector<double> tstamps(time + 1);

  MPI_Barrier(MPI_COMM_WORLD);

  for (size_t it = 0; it < time; ++it)
    {
      tstamps[it] = MPI_Wtime();

      start[0] = (hsize_t) it;

      assert(H5Sselect_hyperslab(fspace, H5S_SELECT_SET, start, NULL, count,
                                 block) >= 0);

      if (coll_flg)
        {
          assert(H5Dwrite(dset, H5T_NATIVE_FLOAT, mspace, fspace, dxpl,
                          &v[0]) >= 0);
        }
      else
        {
          assert(H5Dwrite(dset, H5T_NATIVE_FLOAT, mspace, fspace,
                          H5P_DEFAULT, &v[0]) >= 0);
        }
    }

  // release open handles
  assert(H5Sclose(mspace) >= 0);
  assert(H5Pclose(dxpl) >= 0);
  assert(H5Sclose(fspace) >= 0);
  assert(H5Pclose(dcpl) >= 0);
  assert(H5Dclose(dset) >= 0);
  assert(H5Fclose(file) >= 0);

  MPI_Barrier(MPI_COMM_WORLD);

  tstamps[time] = MPI_Wtime();

  // print timings/throughput
  size_t bytes_written = time * processor[0] * domain[0] * processor[1] *
    domain[1] *  processor[2] * domain[2] * sizeof(float);

  if (rank == 0)
    {
      cout << "Aggregate throughput:\t" << bytes_written /
        (tstamps[time] - tstamps[0]) / ((double) (1<<20)) << " MB/s" <<
        endl;
    }

  MPI_Finalize();

  return 0;
}
