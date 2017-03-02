mkdir hdfsrc
cd hdfsrc
git clone --branch master 'ssh://git@bitbucket.hdfgroup.org:7999/~byrn/performance_adb.git' . --progress
cd ..
mkdir build
cd build
python ../doftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ hdf5perf/ . 19bbtconf.json
python ../dobtftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ centos7 centos7 StdMPITrunk bbparams autotools 'Unix Makefiles' insbin 19bbtconf.json
python ../dobtuncompress.py centos7 centos7 StdMPITrunk bbparams autotools 'Unix Makefiles' insbin 19bbtconf.json
python ../doatlin.py insbin 19bbtconf.json
#combust_io test
python ../doFilesftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ centos7 centos7 StdMPITrunk combust_io autotools DTP/extra 19bbtconf.json
python ../doATconfig.py 19bbtconf.json StdMPITrunk combust_io centos7 kituo-slave C Fortran Java MPI
python ../doATmake.py 19bbtconf.json StdMPITrunk combust_io centos7 kituo-slave C Fortran Java MPI
python ../doATcheck.py 19bbtconf.json StdMPITrunk combust_io centos7 kituo-slave C Fortran Java MPI
#h5core test
python ../doFilesftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ centos7 centos7 StdMPITrunk h5core autotools DTP/extra 19bbtconf.json
python ../doATconfig.py 19bbtconf.json StdMPITrunk h5core centos7 kituo-slave C Fortran Java MPI
python ../doATmake.py 19bbtconf.json StdMPITrunk h5core centos7 kituo-slave C Fortran Java MPI
python ../doATcheck.py 19bbtconf.json StdMPITrunk h5core centos7 kituo-slave C Fortran Java MPI
#h5perf test
python ../doFilesftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ centos7 centos7 StdMPITrunk h5perf autotools DTP/extra 19bbtconf.json
python ../doATconfig.py 19bbtconf.json StdMPITrunk h5perf centos7 kituo-slave C Fortran Java MPI
python ../doATmake.py 19bbtconf.json StdMPITrunk h5perf centos7 kituo-slave C Fortran Java MPI
python ../doATcheck.py 19bbtconf.json StdMPITrunk h5perf centos7 kituo-slave C Fortran Java MPI
#seism-core test
python ../doFilesftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ centos7 centos7 StdMPITrunk seism-core autotools DTP/extra 19bbtconf.json
python ../doATconfig.py 19bbtconf.json StdMPITrunk seism-core centos7 kituo-slave C Fortran Java MPI
python ../doATmake.py 19bbtconf.json StdMPITrunk seism-core centos7 kituo-slave C Fortran Java MPI
python ../doATcheck.py 19bbtconf.json StdMPITrunk seism-core centos7 kituo-slave C Fortran Java MPI
