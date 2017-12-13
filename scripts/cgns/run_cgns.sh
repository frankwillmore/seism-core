#!/bin/bash
#
#
# This script will build CGNS, and get performance numbers, for all the currently released versions of HDF5.
#
red=$'\e[1;31m'
grn=$'\e[1;32m'
yel=$'\e[1;33m'
blu=$'\e[1;34m'
mag=$'\e[1;35m'
cyn=$'\e[1;36m'
nc='\033[0m' # No Color

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"
PARALLEL=0
case $key in
    --enable-parallel)
    PARALLEL=1
    shift
    ;;
    --default)
    PARALLEL=0
    shift
    ;;
    *)    # unknown option
    ;;
esac
done

host=$HOSTNAME

OPTS=""
if [[ $PARALLEL != 1 ]]; then
   echo -e "${red}Enabled Parallel: FALSE${nc}"
   export CC="gcc"
   export FC="gfortran"
   export F77="gfortran"
else
   echo -e "${green}Enabled Parallel: TRUE${nc}"
   OPTS="--enable-parallel"
   export CC="mpicc"
   export FC="mpif90"
   export F77="mpif90"

   if [[ "$host" == *"cetus"* || "$host" == *"mira"* ]]; then
       export MPIEXEC="runjob -n 256 -p 16 --block $COBALT_PARTNAME :"
   else
       export MPIEXEC="mpiexec -n 4"
   fi
fi

# Output all the results in the cgns-timings file.
#

# List of all the HDF5 versions to run through
#VER_HDF5="8_1 8_2 8_3-patched 8_4-patch1 8_5-patch1 8_6 8_7 8_8 8_9 8_10-patch1 8_11 8_12 8_13 8_14 8_15-patch1 8_16 8_17 8_18 8_19 8_20 10_0-patch1 10_1"

VER_HDF5="8_20"
export LIBS="-ldl"
export FLIBS="-ldl"
#export LIBS="-Wl,--no-as-needed -ldl"

git clone https://brtnfld@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git

j=0
for i in ${VER_HDF5}

do
    status=0
    j=$[j + 1]
# Build HDF5
    cd hdf5
    git checkout tags/hdf5-1_$i
    rm -fr build
    mkdir build
    cd build

    if [[ $VER_HDF5 == 1* ]]; then
	HDF5_OPTS="--enable-build-mode=production $OPTS"	
    else
	HDF5_OPTS="--enable-production $OPTS"
    fi
	

    HDF5=$PWD
    ../configure --disable-fortran --disable-hl $HDF5_OPTS
    make -i -j 16
    status=$?
    if [[ $status != 0 ]]; then
	echo "HDF5 make #FAILED"
	exit $status
    fi
    make -i -j 16 install
    status=$?
    if [[ $status != 0 ]]; then
	echo "HDF5 make install #FAILED"
	exit $status
    fi

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
	--enable-64bit $OPTS

    make -j 16
    status=$?
    if [[ $status != 0 ]]; then
	echo "CGNS make #FAILED"
	exit $status
    fi
    if [[ $PARALLEL != 1 ]]; then
      # compile the tests
      make -j 16 check
      status=$?
      if [[ $status != 0 ]]; then
	  echo "CGNS make check #FAILED"
	  exit $status
      fi
      # Time make check (does not include the complilation time)
      { /usr/bin/time -f "%e real" make check ; } 2> results
      { echo -n "1_$i " & grep -i "real" results; } > ../../cgns_$j
      cd ../../
      rm -fr hdf5/build
      rm -fr CGNS
    else
      cd ptests
      make -j 16
      status=$?
      if [[ $status != 0 ]]; then
	  echo "PCGNS make #FAILED"
	  exit $status
      fi
      # Time make check (does not include the complilation time)
      { /usr/bin/time -f "%e real" make test ; } 2> results
      { echo -n "1_$i " & grep -i "real" results; } > ../../../cgns_$j
      cd ../../../
      rm -fr hdf5/build
      rm -fr CGNS
    fi

done

# Combine the timing numbers to a single file

cat cgns_* > cgns-timings
sed -i 's/real//g' cgns-timings
sed -i 's/_/./g' cgns-timings

rm -f cgns_*

