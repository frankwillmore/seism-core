# A Kernel to mimic IO patterns in a large-scale simulation code.

This code writes/reads 3D datasets using the strategies employed by a user
group on Blue Waters. In the code a 3D dataset is partitioned among
MPI processors arranged in a 3D domain decomposition. To limit the number of
files used to store the dataset, we write data collectively using parallel
HDF5 in one computational direction. This effectively collapses the process
layout from a 3D domain decomposition to a 2D domain decomposition.
Schematics of how the dataset is stored in memory and disk are shown below:
```
         3D DOMAIN DECOMPOSITION     2D DOMAIN DECOMPOSITION
            STORED IN MEMORY            STORED ON DISK
                 ---------                   ---------
                /   /   /|                  /   /   /|
               /-------/ |                 /   /   / |
              /   /   /| |                /   /   /  |
             --------- |/|               ---------  /|
             |   |   | / |               |   |   | / |
             |   |   |/| /               |   |   |/  /
             --------- |/                ---------  /
             |   |   | /                 |   |   | /
             |   |   |/                  |   |   |/
             ---------                   ---------
```

- In the left schematic, each MPI process is represented by a subvolume of
  the large dataset.
- In the right schematic, each file stored on disk is represented by a
  subvolume (a "pencil") of the large dataset.

To make the reading process when restarting with a different MPI process
layout simpler (in production simulations), we also write out a virtual
dataset (VDS) during IO that logically arranges the pencils into the larger
dataset that they comprise. When reading in data, all processes go through
the VDS to access their data from the pencil files.

The kernel code is set up with parameters provided by the file "input". An
example file is shown below.

*UPDATE 2016-12-11:* Writing now controlled by input flag. This will allow us
to conduct tests in which we read on a different number of processors than
were used to write the data. Also added chunking dimensions as an input
parameter to add more flexibility in how we can write out data.

```
      nx,ny,nz,ng
         128,256,512,2
      iprc,jprc,kprc
         2,16,4
      relay,relay size
         1,32
      write flag
         1
      chunk dims
         -1,-1,-1
```

Description of input parameters:

```
    nx: Number of grid points in the x direction
    ny: Number of grid points in the y direction
    nz: Number of grid points in the z direction
    ng: Number of ghost layers in all coordinate directions
    iprc: Number of MPI tasks in the x direction
    jprc: Number of MPI tasks in the y direction
    kprc: Number of MPI tasks in the z direction
    relay: Whether to use the relay scheme when reading data. relay <= 0 turns
       relay scheme off, relay > 0 turns it on.
    relay size: When using the relay scheme, relay_size is the number of
       processors that are allowed to simultaneously read data. This must be a
       factor of the total process count (e.g., with 16 MPI processors a
       relay_size of 4 is acceptable, but 7 is not).
    write flag: 0 to turn off writing, anything else to turn on writing
    chunk dims: chunking dimensions when writing out datasets. set any to -1
       deactivate manual chunking and use the default options.
```

Important notes to users:

- Ghost layers are not written out to file. We use HDF5 partial IO to
  avoid writing them.

- Data is stored in the ``"data"`` directory, which should be in the same
  directory as the executable. Inside the ``"data"`` directory files are
  stored in further subdirectories numbered 0, 1, 2, etc. The number of
  subdirectories is determined by the product ``jprc*kprc`` (which is the
  number of files to be written out). Specifically, take the square root
  of ``jprc*kprc`` and round down to the closest power of 2 to get the number
  of subdirectories. For example, with ``jprc=4`` and ``kprc=4``, the data
  directory should have 4 subdirectories 0, 1, 2, and 3. You would use the
  same number of subdirectories for the cases ``jprc=8 kprc=4`` and
  ``jprc=4 krpc=8``. We provide a script ``"prep_data"`` to make the
  subdirectories for you. For the cases described above, you can use
  ```
          # prep_data 4
  ```

- You must use HDF5 1.10.0 or greater with this code.

- We typically use the relay scheme when the number of processors is above
  ~32K (K=1024) on BW. It is very important to use it when using >=128K
  processors to avoid stressing the file system. For large process counts
  (e.g., 256K) we typically limit the number of tasks simultaneously
  reading data to 4K or 8K.

- We've only tested this code using powers of 2 for the grid extents and
  process counts in each direction.
