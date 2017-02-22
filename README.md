# H5CORE - Creative Use of the HDF5 Core VFD

The main reference for this I/O kernel is Leigh Orf's description
[An I/O strategy for CM1 on Blue Waters](http://orf.media/wp-content/uploads/2015/11/cm1tools-November2015.pdf).
It simulates the write pattern of his CM1-ET5 code. As stated in his description,
his goals are to:

1. reduce the number of files written to a reasonable number
2. minimize the number of times you are doing actual I/O to the Lustre file
   system
3. write big files
4. make it easy for users to read in data for analysis and visualization after
   it is written

He achieves his first goal, by creating one HDF5 file per node (32 cores on
Blue Waters). The second and third goal are achieved by buffering data in
memory using the HDF5 Core Virtual File Driver. That means that until
the HDF5 file is closed all I/O operations are performed in memory.
Upon file closure, the entire file (~40 GB) is written in a single POSIX
write operation to the Luster file system.

## Running `h5core`

The executbale can be run standalone or with mulitple MPI processes.
The executable can be invoked as follows:

    usage: h5core [OPTIONS]
      OPTIONS
           -b      Write file to disk on exit
           -c      Apply ZFP compression
           -i I    Memory buffer increment size in bytes [default: 512 MB]
           -n      Disable write (to disk) paging
           -p P    Page size in bytes [default: 64 MB]
           -t T    Number of iterations [default: 5]

## Sample Output

See (here)[https://bitbucket.hdfgroup.org/projects/NCSABW/repos/h5core/browse/40GB-init.txt]
for a sample output file. The output begins with the parameters chosen for this
run. This is followed by the memory usage before any memory allocation by the
kernel. The time for writing the HDF5 datasets to the (in-memory) HDF5 file
is referred to as *Total time for buffering chunks to memory*.
This is followed by the memory usage before and after closing the HDF5 file, and
the time for `H5Fclose`. Finally, the effective, per-process write-throughput
(total bytes written / total time) is printed.

    Write to disk: NO
    Increment size: 536870912 [bytes]
    Page size: 67108864 [bytes]
    Iterations: 5

    BEFORE RUNNING:
    rank 0000     Memtotal:     128.66 GB
    rank 0000      MemFree:      21.99 GB
    rank 0000      Buffers:     126.48 GB
    rank 0000       Cached:       0.00 GB
    rank 0000   SwapCached:     101.16 GB
    rank 0000       Active:       0.02 GB
    rank 0000     Inactive:      45.62 GB
    Iteration: 0
    rank 0000 Total time for buffering chunks to memory:     25.815358 seconds
    BEFORE FLUSHING:
    rank 0000     Memtotal:     128.66 GB
    rank 0000      MemFree:       0.38 GB
    rank 0000      Buffers:      85.46 GB
    rank 0000       Cached:       0.00 GB
    rank 0000   SwapCached:      81.99 GB
    rank 0000       Active:       0.02 GB
    rank 0000     Inactive:      79.47 GB
    AFTER FLUSHING:
    rank 0000     Memtotal:     128.66 GB
    rank 0000      MemFree:      78.48 GB
    rank 0000      Buffers:     126.62 GB
    rank 0000       Cached:       0.00 GB
    rank 0000   SwapCached:      45.74 GB
    rank 0000       Active:       0.01 GB
    rank 0000     Inactive:      22.74 GB
    rank 0000 Total time for flushing to disk:                   210.72 seconds
    Iteration: 1
    ...
    Iteration: 2
    ...
    Iteration: 3
    ...
    Iteration: 4
    ...
    Total time: 982.978766 s
    Effective bandwidth per process: 0.183366 GB/s
