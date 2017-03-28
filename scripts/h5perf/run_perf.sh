#!/bin/bash

export CC="gcc"
  
for i in 8_1 8_2 8_3-patched 8_4-patch1 8_5-patch1 8_6 8_7 8_8 8_9 8_10-patch1 8_11 8_12 8_13 8_14 8_15-patch1 8_16 8_17 10_0-patch1
do
    svn co https://svn.hdfgroup.org/hdf5/tags/hdf5-1_$i
    svn co https://svn.hdfgroup.org/hdf5/tags/hdf5-1_8_14/perform

    cd hdf5-1_$i
    HDF5_DIR=$PWD/hdf5
     
    ./configure --disable-fortran --disable-hl
    make -i -j 16
    make -i -j 16 install

    cd ../perform
    $HDF5_DIR/bin/h5cc -DSTANDALONE sio_perf.c sio_engine.c sio_timer.c -o h5perf_serial
    ./h5perf_serial -e 18000,19000 -i 4 > ../tim_1_$i

    cd ../
    rm -fr perform hdf5-1_$i

done


