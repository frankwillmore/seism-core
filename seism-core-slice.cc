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
// An example input script is shown below:
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
#include <sstream>
#include <vector>

using namespace std;

#define CONTIG_DSET_NAME "contiguous"
#define CHUNKED_DSET_NAME "chunked"

////////////////////////////////////////////////////////////////////////////////

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
  hid_t dset = H5Dcreate(file, CONTIG_DSET_NAME, H5T_IEEE_F32LE, fspace,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  assert(dset >= 0);
  assert(H5Dclose(dset) >= 0);
  dset = H5Dcreate(file, CHUNKED_DSET_NAME, H5T_IEEE_F32LE, fspace,
                   H5P_DEFAULT, dcpl, H5P_DEFAULT);
  assert(dset >= 0);
  assert(H5Dclose(dset) >= 0);
  assert(H5Fclose(file) >= 0);
  assert(H5Pclose(fapl) >= 0);
}

////////////////////////////////////////////////////////////////////////////////

void setMPI_Info(MPI_Info& info, const size_t& v_size, int mpi_size)
{
  assert(MPI_Info_create(&info) == MPI_SUCCESS);
  assert(MPI_Info_set( info, "romio_cb_write", "enable" ) == MPI_SUCCESS);
  assert(MPI_Info_set( info, "romio_ds_write", "disable" ) == MPI_SUCCESS);

  ostringstream ost;
  ost << v_size * sizeof(float);
  assert(MPI_Info_set( info, "cb_block_size", ost.str().c_str()) == MPI_SUCCESS);
  assert(MPI_Info_set( info, "cb_buf_size", ost.str().c_str()) == MPI_SUCCESS);
  ost.str("");
  ost << mpi_size;
  assert(MPI_Info_set( info, "cb_nodes", ost.str().c_str() ) == MPI_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////

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
  int mpi_collective_io_int = 0;
  int precreate_int = 0;

  if (mpi_rank==0)
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
          cin >> simulation_time;
        if (!parameter.compare("use_collective"))
          mpi_collective_io_int = true;
        if (!parameter.compare("precreate"))
          precreate_int = true;
        getline(cin, rest_of_line); // read the rest of the line
      }
    }

  assert(MPI_Bcast(&simulation_time, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
         MPI_SUCCESS);
  assert(MPI_Bcast(&processor, 3, MPI_INT, 0, MPI_COMM_WORLD) ==
         MPI_SUCCESS);
  assert(MPI_Bcast(&chunk, 3, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
  assert(MPI_Bcast(&domain, 3, MPI_INT, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
  assert(MPI_Bcast(&mpi_collective_io_int, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
         MPI_SUCCESS);
  assert(MPI_Bcast(&precreate_int, 1, MPI_INT, 0, MPI_COMM_WORLD) ==
         MPI_SUCCESS);
  bool coll_flg = (bool)mpi_collective_io_int;// flg for collective I/O
  bool pre_flg = (bool)precreate_int;

  // check the arguments
  assert(time > 0);
  assert(processor[0]*processor[1]*processor[2] == (hsize_t) mpi_size);
  assert(processor[0] > 1 && processor[1] > 1 && processor[2] > 1);
  assert(chunk[0] > 1 && chunk[1] > 1 && chunk[2] > 1);
  assert(domain[0] > 1 && domain[1] > 1 && domain[2] > 1);

  if (mpi_rank == 0)
    {
      cout << "================================================================================" << endl;
      cout << "Number of processes:\t" << mpi_size << endl;
      cout << "Process layout:\t\t" << processor[0] << " x " <<
        processor[1] << " x " << processor[2] << endl;
      cout << "Per process grid:\t" << domain[0] << " x " << domain[1] <<
        " x " << domain[2] << endl;
      cout << "Chunk dimensions:\t" << chunk[0] << " x " << chunk[1] <<
        " x " << chunk[2] << endl;
      cout << "Number of time steps:\t" << simulation_time << endl;
      cout << "Collective I/O:\t\t" << coll_flg << endl;
      cout << "Pre-create:\t\t" << pre_flg << endl;
      cout << endl;
    } 

  //////////////////////////////////////////////////////////////////////////////
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

  hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
  assert(dcpl >= 0);
  assert(H5Pset_chunk(dcpl, n_dims, cdims) >= 0);

  //////////////////////////////////////////////////////////////////////////////
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

  //////////////////////////////////////////////////////////////////////////////
  // data transfer property list for collective I/O, if selected
  hid_t dxpl = H5P_DEFAULT;
  if (coll_flg)
    {
      dxpl = H5Pcreate(H5P_DATASET_XFER);
      assert(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) >= 0);
    }

  //////////////////////////////////////////////////////////////////////////////
  // create in-memory dataspace
  dims[0] = 1;
  dims[1] = domain[0];
  dims[2] = domain[1];
  dims[3] = domain[2];

  hid_t mspace = H5Screate_simple(n_dims, dims, NULL);
  assert(mspace >= 0);
  assert(H5Sselect_all(mspace) >= 0);

  //////////////////////////////////////////////////////////////////////////////
  // initialize the test data to MPI rank
  vector<float> v((size_t) domain[0]*domain[1]*domain[2], (float) mpi_rank);

  // create the fapl
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
  assert(fapl >= 0);

  // use the latest file format
  assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >=
         0);

  MPI_Info info;
  if (coll_flg)
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
  hid_t file, dset_chunked, dset_contig;

  MPI_Barrier(MPI_COMM_WORLD);

  //////////////////////////////////////////////////////////////////////////////

  // precreate datasets, as needed
  double start_create = MPI_Wtime();

  if (pre_flg)
    {
      if (mpi_rank == 0) // create with process 0, then close & re-open
        {
          precreate_0(fname, fspace, dcpl);
        }
      MPI_Barrier(MPI_COMM_WORLD);
      file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, fapl);
      assert (file >= 0);
      dset_contig = H5Dopen(file, CONTIG_DSET_NAME, H5P_DEFAULT);
      assert(dset_contig >= 0);
      dset_chunked = H5Dopen(file, CHUNKED_DSET_NAME, H5P_DEFAULT);
      assert(dset_chunked >= 0);
    }
  else
    {
      file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
      assert(file >= 0);
      dset_contig = H5Dcreate(file, CONTIG_DSET_NAME, H5T_IEEE_F32LE, fspace,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      assert(dset_contig >= 0);
      dset_chunked = H5Dcreate(file, CHUNKED_DSET_NAME, H5T_IEEE_F32LE, fspace,
                               H5P_DEFAULT, dcpl, H5P_DEFAULT);
      assert(dset_chunked >= 0);
    }

  MPI_Barrier(MPI_COMM_WORLD);
  double stop_create = MPI_Wtime();

  //////////////////////////////////////////////////////////////////////////////
  // write the contiguous dataset

  double start_contig = MPI_Wtime();

  for (size_t it = 0; it < simulation_time; ++it)
    {
      start[0] = (hsize_t) it;
      assert(H5Sselect_hyperslab(fspace, H5S_SELECT_SET, start, NULL, count,
                                 block) >= 0);
      assert(H5Dwrite(dset_contig, H5T_NATIVE_FLOAT, mspace, fspace, dxpl,
                      &v[0]) >= 0);
    }

  MPI_Barrier(MPI_COMM_WORLD);
  double stop_contig = MPI_Wtime();

  // write the chunked dataset
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

  //////////////////////////////////////////////////////////////////////////////

  assert(H5Dclose(dset_chunked) >= 0);
  assert(H5Dclose(dset_contig) >= 0);
  assert(H5Pclose(fapl) >= 0);
  assert(H5Sclose(mspace) >= 0);
  assert(H5Pclose(dxpl) >= 0);
  assert(H5Sclose(fspace) >= 0);
  assert(H5Pclose(dcpl) >= 0);

  MPI_Barrier(MPI_COMM_WORLD);
  double fclose_start = MPI_Wtime();

  assert(H5Fclose(file) >= 0);

  MPI_Barrier(MPI_COMM_WORLD);
  double fclose_stop = MPI_Wtime();

  if (mpi_rank == 0)
    {
      cout << "(Pre-)create/open:\t" <<
        (stop_create - start_create) << " s"<< endl;
      cout << "Contiguous write:\t" <<
        (stop_contig - start_contig) << " s" << endl;
      cout << "Chunked write:\t\t" <<
        (stop_chunked - stop_contig) << " s" << endl;
      cout << "Close file:\t\t" << (fclose_stop - fclose_start) << " s"
           << endl;

      size_t bytes_written = simulation_time * processor[0] * domain[0] *
        processor[1] * domain[1] *  processor[2] * domain[2] * sizeof(float);

      cout << "\nContiguous throughput:\t" << bytes_written /
        (stop_contig - start_contig) / ((double) (1<<20)) << " MB/s"
           << endl;
      cout << "Chunked throughput:\t" << bytes_written /
        (stop_chunked - stop_contig) / ((double) (1<<20)) << " MB/s"
           << endl;
      cout << "Aggregate throughput:\t" << bytes_written /
        (fclose_stop - begin) / ((double) (1<<20)) << " MB/s"
           << endl;
    }

  MPI_Finalize();

  return 0;
}
