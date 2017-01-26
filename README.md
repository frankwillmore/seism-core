# h5perf:  HDF5 performance measurement tool 

---
---

h5perf measures the read and write performance of 1 and 2-dimensional datasets, according to the values of a set of user-specified parameters. Provided options include interleaved access, independent or collective MPI communication, chunked vs contiguous storage layout, block size, datasets per file, MPI-I/O vs MPI-posix driver, as well as buffer sizes and debugging info. 

For example, specifying: 

    mpiexec –n 3 h5perf –B 2 –e 8 –p 3 –P 3 –x 4 –X 4

runs h5perf on 3 processors, using:

 -B 2 --> block size of 2
 -e 8 --> 8 bytes per process per dataset
 -p 3 --> minimum number of processes
 -P 3 --> maximum number of processes
 -x 4 --> minimum transfer size (bytes)
 -X 4 --> maximum transfer size (bytes)

[drawing](figure_2.png )

See the User Guide (pdf)<link> for a more complete list of parameters, examples and figures of the access patterns which are generated.
