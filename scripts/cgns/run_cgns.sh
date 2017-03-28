#!/bin/bash
#
#
# This script will build CGNS, and get performance numbers, for all the currently released versions of HDF5.
# 
# Output all the results in the cgns-timings file.
# Does not test parallel.
#

# List of all the HDF5 versions to run through
VER_HDF5="8_1 8_2 8_3-patched 8_4-patch1 8_5-patch1 8_6 8_7 8_8 8_9 8_10-patch1 8_11 8_12 8_13 8_14 8_15-patch1 8_16 8_17 8_18 10_0-patch1 10_1"
 
export CC="gcc"
export LIBS="-Wl,--no-as-needed -ldl"

git clone https://brtnfld@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git

for i in ${VER_HDF5}

do

# Build HDF5
    cd hdf5
    git checkout tags/hdf5-1_$i
    rm -fr build
    mkdir build
    cd build

    HDF5=$PWD
    ../configure --disable-fortran --disable-hl
    make -i -j 16
    make -i -j 16 install

# Build CGNS
    cd ../../
    git clone https://github.com/CGNS/CGNS.git
    cd CGNS/src

    ./configure \
	--with-fortran \
	--with-hdf5=$HDF5/hdf5 \
	--enable-lfs \
	--disable-shared \
	--enable-debug \
	--disable-cgnstools \
        --with-zlib=/usr/include,/usr/lib64/ \
	--enable-64bit

    make -j 16
    make -j 16 check
    { /usr/bin/time -f "%e real" make check ; } 2> results
    { echo -n "1_$i " & grep -i "real" results; } > ../../cgns_$i
    cd ../../
    rm -fr hdf5/build
    rm -fr CGNS

done

# Combine the timing numbers to a single file

cat cgns_* > cgns-timings
sed -i 's/real//g' cgns-timings
sed -i 's/_/./g' cgns-timings

rm -f cgns_*

