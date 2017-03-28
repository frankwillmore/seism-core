#!/bin/bash

export CC="mpicc"
##for i in 8_1 8_2 8_3-patched 8_4-patch1 8_5-patch1 8_6 8_7 8_8 8_9 8_10-patch1 8_11 8_12 8_13 8_14 8_15-patch1 8_16 8_17 10_0-patch1

for i in 8_1
do
    svn co https://svn.hdfgroup.org/hdf5/tags/hdf5-1_$i
    git clone https://github.com/LLNL/ior.git

    cd hdf5-1_$i
    HDF5_DIR=$PWD/hdf5
    
    ./configure --disable-fortran --disable-hl --disable-parallel
    make -i -j 16
    make -i -j 16 install

    export LIBS="-L$HDF5_DIR/lib -lhdf5 -I$HDF5_DIR/include" 
    cd ../ior
    ./bootstrap
    ./configure --with-mpiio=no --with-posix --with-hdf5 --with-lustre=no


    cd ../
#    rm -fr perform hdf5-1_$i

done


