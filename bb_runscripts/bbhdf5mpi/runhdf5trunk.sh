mkdir hdfsrc
cd hdfsrc
git clone --branch develop 'ssh://git@bitbucket.hdfgroup.org:7999/~byrn/hdf5_adb.git' . --progress
cd ..
mkdir build
cd build
python ../doftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ hdf5trunk/ . bbtconf.json
python ../doFilesftp.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ fedora25 fedora StdMPIShar bbparams ctest DTP/extra bbtconf.json
python ../doCTconfig.py bbtconf.json StdMPIShar bbparams ctest fedora schedule C Fortran Java MPI
python ../doCTrun.py bbtconf.json StdMPIShar bbparams ctest fedora 'Unix Makefiles'
python ../doCTpackage.py bbtconf.json StdMPIShar bbparams ctest fedora25 fedora 'Unix Makefiles' local-worker
python ../dobtftpup.py jam.hdfgroup.uiuc.edu /mnt/ftp/pub/outgoing/QATEST/ fedora25 fedora StdMPIShar bbparams ctest 'Unix Makefiles' hdf5trunk bbtconf.json
