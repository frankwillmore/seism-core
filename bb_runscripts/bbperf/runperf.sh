# PLATFORM list
# "fedora",
# "debian",
# "ubuntu",
# "opensuse",
# "centos6",
# "centos7",
# "cygwin",
# "ppc64",
# "osx109",
# "osx1010",
# "osx1011",

USAGE()
{
cat << EOF
Usage: $0 <platform>
   Run tests for <platform>

EOF

}


if [ $# != 1 ]; then
    USAGE
    exit 1
fi

PLATFORM="$1"

HOST='206.221.145.51'
DIRN='/mnt/ftp/pub/outgoing/QATEST/'

GENERATOR='Unix Makefiles'
CONFIG=StdMPITrunk
PRODUCT=hdf5perf
OSSIZE=64

mkdir build
cd build
#
mkdir hdfsrc
cd hdfsrc
#git clone --branch master 'ssh://git@bitbucket.hdfgroup.org:7999/hdffv/performance.git' . --progress
git clone --branch master 'https://git@bitbucket.hdfgroup.org/scm/hdffv/performance.git' . --progress
cd ..
python ../doftp.py $HOST $DIRN scripts . bbtconf.json
#
mkdir autotools
cd autotools
python ../../doftp.py $HOST $DIRN hdf5perf/ . bbtconf.json
python ../../doftp.py $HOST $DIRN scripts . doDistributeGet.py
python ../../doftp.py $HOST $DIRN scripts . dobtftp.py
python ../../doftp.py $HOST $DIRN scripts . dobtuncompress.py
python ../../doftp.py $HOST $DIRN scripts . doatlin.py
python ../../doftp.py $HOST $DIRN scripts . doFilesftp.py
python ../../doftp.py $HOST $DIRN scripts . doATconfig.py
python ../../doftp.py $HOST $DIRN scripts . doATmake.py
python ../../doftp.py $HOST $DIRN scripts . doATcheck.py
#
python ./doDistributeGet.py $HOST $DIRN $PLATFORM $PLATFORM $CONFIG bbparams bbtconf.json
python ./dobtftp.py $HOST $DIRN $PLATFORM $OSSIZE $PLATFORM $CONFIG bbparams autotools "$GENERATOR" insbin bbtconf.json
python ./dobtuncompress.py $PLATFORM $OSSIZE $PLATFORM $CONFIG bbparams autotools "$GENERATOR" insbin bbtconf.json
python ./doatlin.py insbin bbtconf.json
#combust_io test
python ./doFilesftp.py $HOST $DIRN $PLATFORM $PLATFORM $CONFIG combust_io autotools DTP/extra bbtconf.json
python ./doATconfig.py bbtconf.json $CONFIG combust_io $PLATFORM schedule C Fortran MPI
python ./doATmake.py bbtconf.json $CONFIG combust_io $PLATFORM schedule C Fortran MPI
python ./doATcheck.py bbtconf.json $CONFIG combust_io $PLATFORM schedule C Fortran MPI
#h5core test
python ./doFilesftp.py $HOST $DIRN $PLATFORM $PLATFORM $CONFIG h5core autotools DTP/extra bbtconf.json
python ./doATconfig.py bbtconf.json $CONFIG h5core $PLATFORM schedule C Fortran MPI
python ./doATmake.py bbtconf.json $CONFIG h5core $PLATFORM schedule C Fortran MPI
python ./doATcheck.py bbtconf.json $CONFIG h5core $PLATFORM schedule C Fortran MPI
#h5perf test
python ./doFilesftp.py $HOST $DIRN $PLATFORM $PLATFORM $CONFIG h5perf autotools DTP/extra bbtconf.json
python ./doATconfig.py bbtconf.json $CONFIG h5perf $PLATFORM schedule C Fortran Java MPI
python ./doATmake.py bbtconf.json $CONFIG h5perf $PLATFORM schedule C Fortran Java MPI
python ./doATcheck.py bbtconf.json $CONFIG h5perf $PLATFORM schedule C Fortran Java MPI
#seism-core test
python ./doFilesftp.py $HOST $DIRN $PLATFORM $PLATFORM $CONFIG seism-core autotools DTP/extra bbtconf.json
python ./doATconfig.py bbtconf.json $CONFIG seism-core $PLATFORM schedule C Fortran MPI
python ./doATmake.py bbtconf.json $CONFIG seism-core $PLATFORM schedule C Fortran MPI
python ./doATcheck.py bbtconf.json $CONFIG seism-core $PLATFORM schedule C Fortran MPI
#
cd ../..

