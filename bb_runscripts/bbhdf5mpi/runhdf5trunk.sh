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
#git clone --branch master 'ssh://git@bitbucket.hdfgroup.org:7999/hdffv/hdf5.git' . --progress
git clone --branch master 'https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git' . --progress
cd ..
python ./doftp.py $HOST $DIRN scripts . bbtconf.json
#
mkdir autotools
cd autotools
python ../../doftp.py $HOST $DIRN hdf5trunk/ . bbtconf.json
python ../../doftp.py $HOST $DIRN scripts . doFilesftp.py
python ../../doftp.py $HOST $DIRN scripts . doATconfig.py
python ../../doftp.py $HOST $DIRN scripts . doATmake.py
python ../../doftp.py $HOST $DIRN scripts . doATcheck.py
python ../../doftp.py $HOST $DIRN scripts . doATinstall.py
python ../../doftp.py $HOST $DIRN scripts . doATinstallcheck.py
python ../../doftp.py $HOST $DIRN scripts . doATpackage.py
python ../../doftp.py $HOST $DIRN scripts . dobtftpup.py
python ../../doftp.py $HOST $DIRN scripts . doATuninstall.py
#
python ./doFilesftp.py $HOST $DIRN $PLATFORM $PLATFORM $CONFIG bbparams autotools DTP/extra bbtconf.json
python ./doATconfig.py bbtconf.json $CONFIG bbparams $PLATFORM schedule C Fortran Java
python ./doATmake.py bbtconf.json $CONFIG bbparams $PLATFORM schedule C Fortran Java
python ./doATcheck.py bbtconf.json $CONFIG bbparams $PLATFORM schedule C Fortran Java
python ./doATinstall.py bbtconf.json $CONFIG bbparams $PLATFORM schedule C Fortran Java
python ./doATinstallcheck.py bbtconf.json $CONFIG bbparams $PLATFORM schedule C Fortran Java
python ./doATpackage.py bbtconf.json $CONFIG bbparams $PLATFORM schedule
python ./dobtftpup.py $HOST $DIRN $PLATFORM $OSSIZE $PLATFORM $CONFIG bbparams autotools "$GENERATOR" hdf5trunk bbtconf.json
python ./doATuninstall.py bbtconf.json $CONFIG bbparams $PLATFORM schedule C Fortran Java
cd ../..

