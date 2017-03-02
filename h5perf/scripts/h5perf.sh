export HDF5_PARAPREFIX=/home/
mpiexec -n 6 ./h5perf -B 2 -e 8 -p 6 -P 6 -x 4 -X 4 --debug=r,t -d 4 -F 4 -i 40
