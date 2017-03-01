# h5perf:  HDF5 performance measurement tool 

---
---

h5perf measures the read and write performance of 1 and 2-dimensional datasets, according to the values of a set of user-specified parameters. Provided options include interleaved access, independent or collective MPI communication, chunked vs contiguous storage layout, block size, datasets per file, MPI-I/O vs MPI-posix driver, as well as buffer sizes and debugging info. 

For example, specifying: 

    mpiexec –n 3 h5perf –B 2 –e 8 –p 3 –P 3 –x 4 –X 4

runs h5perf on 3 processors, using:

 -B 2 --> block size of 2 <br>
 -e 8 --> 8 bytes per process per dataset <br>
 -p 3 --> minimum number of processes <br>
 -P 3 --> maximum number of processes <br>
 -x 4 --> minimum transfer size (bytes) <br>
 -X 4 --> maximum transfer size (bytes) <br>

![drawing](figure_2.png )

In this figure taken from the user guide, we see the numbers themselves indicate which process is writing the byte in the position illustrated. Blue/non-bold numbers indicate what is written in the first file access, red/bold numbers indicate the second file access. 

![drawing](figure_3.png )

To contrast, this figure shows the interleaved (option -I) pattern. Because processes are competing to access contiguous space in memory, interleaved access patterns are typically less performant. 

---

## Sample output

    $ mpiexec -n 3 h5perf -B 2 -e 8 -p 3 -P 3 -x 4 -X 4
    HDF5 Library: Version 1.10.0-patch1
    rank 0: ==== Parameters ====
    rank 0: IO API=posix mpiio phdf5 
    rank 0: Number of files=1
    rank 0: Number of datasets=1
    rank 0: Number of iterations=1
    rank 0: Number of processes=3:3
    rank 0: Number of bytes per process per dataset=8
    rank 0: Size of dataset(s)=24:24
    rank 0: File size=24:24
    rank 0: Transfer buffer size=4:4
    rank 0: Block size=2
    rank 0: Block Pattern in Dataset=Contiguous
    rank 0: I/O Method for MPI and HDF5=Independent
    rank 0: Geometry=1D
    rank 0: VFL used for HDF5 I/O=MPI-IO driver
    rank 0: Data storage method in HDF5=Contiguous
    rank 0: Env HDF5_PARAPREFIX=not set
    rank 0: Dumping MPI Info Object(469762048) (up to 1024 bytes per item):
    object is MPI_INFO_NULL
    rank 0: ==== End of Parameters ====

    Number of processors = 3
    Transfer Buffer Size: 4 bytes, File size: 0.00 MBs
          # of files: 1, # of datasets: 1, dataset size: 0.00 MB
            IO API = POSIX
                Write (1 iteration(s)):
                    Maximum Throughput:   0.64 MB/s
                    Average Throughput:   0.64 MB/s
                    Minimum Throughput:   0.64 MB/s
                Write Open-Close (1 iteration(s)):
                    Maximum Throughput:   0.07 MB/s
                    Average Throughput:   0.07 MB/s
                    Minimum Throughput:   0.07 MB/s
                Read (1 iteration(s)):
                    Maximum Throughput:   1.96 MB/s
                    Average Throughput:   1.96 MB/s
                    Minimum Throughput:   1.96 MB/s
                Read Open-Close (1 iteration(s)):
                    Maximum Throughput:   0.27 MB/s
                    Average Throughput:   0.27 MB/s
                    Minimum Throughput:   0.27 MB/s
            IO API = MPIO
                Write (1 iteration(s)):
                    Maximum Throughput:   0.26 MB/s
                    Average Throughput:   0.26 MB/s
                    Minimum Throughput:   0.26 MB/s
                Write Open-Close (1 iteration(s)):
                    Maximum Throughput:   0.01 MB/s
                    Average Throughput:   0.01 MB/s
                    Minimum Throughput:   0.01 MB/s
                Read (1 iteration(s)):
                    Maximum Throughput:   0.79 MB/s
                    Average Throughput:   0.79 MB/s
                    Minimum Throughput:   0.79 MB/s
                Read Open-Close (1 iteration(s)):
                    Maximum Throughput:   0.04 MB/s
                    Average Throughput:   0.04 MB/s
                    Minimum Throughput:   0.04 MB/s
            IO API = PHDF5 (w/MPI-IO driver)
                Write (1 iteration(s)):
                    Maximum Throughput:   0.05 MB/s
                    Average Throughput:   0.05 MB/s
                    Minimum Throughput:   0.05 MB/s
                Write Open-Close (1 iteration(s)):
                    Maximum Throughput:   0.00 MB/s
                    Average Throughput:   0.00 MB/s
                    Minimum Throughput:   0.00 MB/s
                Read (1 iteration(s)):
                    Maximum Throughput:   0.09 MB/s
                    Average Throughput:   0.09 MB/s
                    Minimum Throughput:   0.09 MB/s
                Read Open-Close (1 iteration(s)):
                    Maximum Throughput:   0.01 MB/s
                    Average Throughput:   0.01 MB/s
                    Minimum Throughput:   0.01 MB/s

---

See the User Guide (pdf)<link> for a more complete list of parameters, examples and figures of the access patterns which are generated.

