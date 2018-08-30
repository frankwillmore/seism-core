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

PARALLEL=0
HDF5BUILD=1
CGNSBUILD=1
TEST=1
HDF5=""

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    --enable-parallel)
    PARALLEL=1
    shift
    ;;
    --hdf5)
    HDF5="$2" # root install directory
    shift # past argumente
    shift # past value
    ;;
    --hdf5_nobuild)
    HDF5BUILD=0
    shift
    ;;
    --cgns_nobuild)
    CGNSBUILD=0
    shift
    ;;
    --notest)
    TEST=0
    shift
    ;;
    --default)
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
   echo -e "${grn}Enabled Parallel: TRUE${nc}"
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
VER_HDF5="8_1 8_2 8_3-patched 8_4-patch1 8_5-patch1 8_6 8_7 8_8 8_9 8_10-patch1 8_11 8_12 8_13 8_14 8_15-patch1 8_16 8_17 8_18 8_19 8_20 8_21 10_0-patch1 10_1 10_2 10_3 develop"

#VER_HDF5="develop"
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
    if [  $HDF5BUILD = 1 ]; then
	cd hdf5

	if [[ $i == d* ]]; then
	    git checkout develop
	    ./autogen.sh
	    rm -fr build_develop
	    mkdir build_develop
	    cd build_develop
	else
	    git checkout tags/hdf5-1_$i
	    rm -fr build_1_$i
	    mkdir build_1_$i
	    cd build_1_$i
	fi
	
	if [[ $i == 1* || $i == d* ]]; then
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
	cd ../../
    else
	if [[ $i == d* ]]; then
	    HDF5=hdf5/build_develop
	else
	    HDF5=hdf5/build_1_$i
	fi
    fi

# Build CGNS

    if [ $CGNSBUILD = 1 ]; then

	git clone https://github.com/CGNS/CGNS.git
	cd CGNS/src

#    rm -fr build_1_${VER_HDF5}
#    mkdir build_1_${VER_HDF5}
#    cd build_1_${VER_HDF5}


	CONFIG_CMD="./configure \
	--with-fortran \
	--with-hdf5=$HDF5/hdf5 \
	--enable-lfs \
	--disable-shared \
	--enable-debug \
	--disable-cgnstools \
        --with-zlib=/usr/include,/usr/lib64/ \
	--enable-64bit $OPTS"

	echo "$CONFIG_CMD"
	$CONFIG_CMD
	
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
	    /usr/bin/time -v -f "%e real" -o "results" make check
            { echo -n "1.$i " & grep "Elapsed" results | sed -n -e 's/^.*ss): //p' | awk -F: '{ print ($1 * 60) + $2 }'; } > ../../cgns_time_$j
	    { echo -n "1.$i " & grep "Maximum resident" results | sed -n -e 's/^.*bytes): //p'; } > ../../cgns_mem_$j
	    cd ../../
	else
	    cd ptests
	    make -j 16
	    status=$?
	    if [[ $status != 0 ]]; then
		echo "PCGNS make #FAILED"
		exit $status
	    fi
      # Time make check (does not include the complilation time)
	    /usr/bin/time -v -f "%e real" -o "results" make test
            { echo -n "1.$i " & grep "Elapsed" results | sed -n -e 's/^.*ss): //p' | awk -F: '{ print ($1 * 60) + $2 }'; } > ../../cgns_time_$j
	    { echo -n "1.$i " & grep "Maximum resident" results | sed -n -e 's/^.*bytes): //p'; } > ../../cgns_mem_$j
	    cd ../../../
	fi
#      rm -fr $HDF5
       rm -fr CGNS

    fi

done

# Combine the timing numbers to a single file
if [ $CGNSBUILD = 1 ]; then

    cat cgns_time_* > cgns-timings
    cat cgns_mem_* > cgns-memory
    sed -i 's/_/./g' cgns-timings
    sed -i 's/_/./g' cgns-memory

    rm -f cgns_*

fi

