!> @file KERNEL.F90
!> @author Donated
!> @brief Program to mimic IO patterns in a large-scale simulation code.
!!
!! This code writes/reads 3D datasets using the strategies employed by a user
!! group on Blue Waters. In the code a 3D dataset is partitioned among
!! MPI processors arranged in a 3D domain decomposition. To limit the number of
!! files used to store the dataset, we write data collectively using parallel
!! HDF5 in one computational direction. This effectively collapses the process
!! layout from a 3D domain decomposition to a 2D domain decomposition.
!! Schematics of how the dataset is stored in memory and disk are shown below:
!!
!!         3D DOMAIN DECOMPOSITION     2D DOMAIN DECOMPOSITION
!!            STORED IN MEMORY            STORED ON DISK
!!                 ---------                   ---------
!!                /   /   /|                  /   /   /|
!!               /-------/ |                 /   /   / |
!!              /   /   /| |                /   /   /  |
!!             --------- |/|               ---------  /|
!!             |   |   | / |               |   |   | / |
!!             |   |   |/| /               |   |   |/  /
!!             --------- |/                ---------  /
!!             |   |   | /                 |   |   | /
!!             |   |   |/                  |   |   |/
!!             ---------                   ---------
!!
!!    - In the left schematic, each MPI process is represented by a subvolume of
!!      the large dataset.
!!    - In the right schematic, each file stored on disk is represented by a
!!      subvolume (a "pencil") of the large dataset.
!!
!! To make the reading process when restarting with a different MPI process
!! layout simpler (in production simulations), we also write out a virtual
!! dataset (VDS) during IO that logically arranges the pencils into the larger
!! dataset that they comprise. When reading in data, all processes go through
!! the VDS to access their data from the pencil files.
!!
!! The kernel code is set up with parameters provided by the file "input". An
!! example file is shown below.
!!
!! UPDATE 2016-12-11: Writing now controlled by input flag. This will allow us
!! to conduct tests in which we read on a different number of processors than
!! were used to write the data. Also added chunking dimensions as an input
!! parameter to add more flexibility in how we can write out data.
!!
!!      nx,ny,nz,ng
!!         128,256,512,2
!!      iprc,jprc,kprc
!!         2,16,4
!!      relay,relay size
!!         1,32
!!      write flag
!!         1
!!      chunk dims
!!         -1,-1,-1
!!
!! Description of input parameters:
!!
!!    nx: Number of grid points in the x direction
!!    ny: Number of grid points in the y direction
!!    nz: Number of grid points in the z direction
!!    ng: Number of ghost layers in all coordinate directions
!!    iprc: Number of MPI tasks in the x direction
!!    jprc: Number of MPI tasks in the y direction
!!    kprc: Number of MPI tasks in the z direction
!!    relay: Whether to use the relay scheme when reading data. relay <= 0 turns
!!       relay scheme off, relay > 0 turns it on.
!!    relay size: When using the relay scheme, relay_size is the number of
!!       processors that are allowed to simultaneously read data. This must be a
!!       factor of the total process count (e.g., with 16 MPI processors a
!!       relay_size of 4 is acceptable, but 7 is not).
!!    write flag: 0 to turn off writing, anything else to turn on writing
!!    chunk dims: chunking dimensions when writing out datasets. set any to -1
!!       deactivate manual chunking and use the default options.
!!
!! Important notes to users:
!!
!!    - Ghost layers are not written out to file. We use HDF5 partial IO to
!!      avoid writing them.
!!
!!    - Data is stored in the "data" directory, which should be in the same
!!      directory as the executable. Inside the "data" directory files are
!!      stored in further subdirectories numbered 0, 1, 2, etc. The number of
!!      subdirectories is determined by the product jprc*kprc (which is the
!!      number of files to be written out). Specifically, take the square root
!!      of jprc*kprc and round down to the closest power of 2 to get the number
!!      of subdirectories. For example, with jprc=4 and kprc=4, the data
!!      directory should have 4 subdirectories 0, 1, 2, and 3. You would use the
!!      same number of subdirectories for the cases jprc=8 kprc=4 and jprc=4
!!      krpc=8. We provide a script "prep_data" to make the subdirectories
!!      for you. For the cases described above, you can use
!!
!!          # prep_data 4
!!
!!    - You must use HDF5 1.10.0 or greater with this code.
!!
!!    - We typically use the relay scheme when the number of processors is above
!!      ~32K (K=1024) on BW. It is very important to use it when using >=128K
!!      processors to avoid stressing the file system. For large process counts
!!      (e.g., 256K) we typically limit the number of tasks simultaneously
!!      reading data to 4K or 8K.
!!
!!    - We've only tested this code using powers of 2 for the grid extents and
!!      process counts in each direction.
!!
PROGRAM KERNEL

! Required modules.
USE ISO_FORTRAN_ENV
USE MPI
USE HDF5
USE IO

IMPLICIT NONE

!> Working precision for the code.
INTEGER,PARAMETER :: RWP = REAL64
!> Pi.
REAL(KIND=RWP),PARAMETER :: PI = ACOS(-1.0_RWP)

! Main user inputs to the code. To be read from the input file.
!
!> Size of rectlinear grid and number of ghost layers.
INTEGER :: nx,ny,nz,ng
!> Number of processors in the three coordinate directions.
INTEGER :: iprc,jprc,kprc
!> Used to control the relay scheme when reading in data.
INTEGER :: relay,relay_size
!> Used to control if data is written out.
INTEGER :: write_flag
!> Used to control chunking parameters in HDF5 datasets.
INTEGER,DIMENSION(3) :: chunk_in
INTEGER(KIND=HSIZE_T),DIMENSION(3) :: chunk_h5

! MPI related variables.
!
!> Number of processors and rank in main communicator.
INTEGER :: nprc,rank
!> Coordinates of this process in the 3D process layout.
INTEGER :: ipid,jpid,kpid
!> Communicator used for parallel IO.
INTEGER :: io_comm
!> Number of processors and rank in IO communicator.
INTEGER :: io_nprc,io_rank
!> MPI type corresponding to RWP set above.
INTEGER :: MPIREAL
!> Miscellaneous variables used during MPI setup.
INTEGER :: rank_tmp,color
INTEGER,DIMENSION(10) :: ibuff

! Other program variables.
!
!> Amount of data on each process (determined from nx, iprc, etc.).
INTEGER :: nxp,nyp,nzp
!> Starting index for this process in each coordinate direction.
INTEGER :: ist,jst,kst
!> Working array.
REAL(KIND=RWP),DIMENSION(:,:,:),ALLOCATABLE :: phi
!> Used for file IO units.
INTEGER :: iounit
!> Logical when checking file status, etc.
LOGICAL :: exs
!> Used to initialize and check the signal.
REAL(KIND=RWP) :: dx,dy,dz,xloc,yloc,zloc
!> Used when checking the error in the signal we read in.
REAL(KIND=RWP) :: err,err_prc,err_max,exact
!> Looping indices, etc.
INTEGER :: i,j,k
!> Error handling.
INTEGER :: ierr

! Initialize MPI.
CALL MPI_INIT(ierr)
CALL MPI_COMM_SIZE(MPI_COMM_WORLD,nprc,ierr)
CALL MPI_COMM_RANK(MPI_COMM_WORLD,rank,ierr)

! Set MPIREAL based on RWP.
IF (RWP .EQ. REAL32) THEN
   MPIREAL = MPI_REAL4
ELSE IF (RWP .EQ. REAL64) THEN
   MPIREAL = MPI_REAL8
END IF

! Read the input file.
IF (rank .EQ. 0) THEN
   INQUIRE(FILE='input',EXIST=exs)
   IF (exs) THEN
      OPEN(NEWUNIT=iounit,FILE='input',ACTION='READ')
      READ(iounit,*)
      READ(iounit,*) nx,ny,nz,ng
      READ(iounit,*)
      READ(iounit,*) iprc,jprc,kprc
      READ(iounit,*)
      READ(iounit,*) relay,relay_size
      READ(iounit,*)
      READ(iounit,*) write_flag
      READ(iounit,*)
      READ(iounit,*) (chunk_in(i),i=1,3)
      CLOSE(UNIT=iounit)
      !
      WRITE(*,200) 'USER INPUTS TO CODE'
      WRITE(*,250) '        NX=',nx
      WRITE(*,250) '        NY=',ny
      WRITE(*,250) '        NZ=',nz
      WRITE(*,250) '        NG=',ng
      WRITE(*,250) '     IPROC=',iprc
      WRITE(*,250) '     JPROC=',jprc
      WRITE(*,250) '     KPROC=',kprc
      WRITE(*,250) '     RELAY=',relay
      WRITE(*,250) 'RELAY SIZE=',relay_size
      WRITE(*,250) 'WRITE FLAG=',write_flag
      WRITE(*,260) 'CHUNK DIMS=',(chunk_in(i),i=1,3)
      !
      ! Check if the number of processors matches the user's request.
      IF (iprc*jprc*kprc .NE. nprc) THEN
         WRITE(*,200) 'Running with incorrect number of processors. Halting.'
         CALL MPI_ABORT(MPI_COMM_WORLD,9,ierr)
      END IF
      !
      ! Check for errors in the relay parameters.
      IF (relay .GT. 0) THEN
         IF (MOD(nprc,relay_size) .NE. 0) THEN
            WRITE(*,200) 'Invalid relay size for relay scheme. Halting.'
            CALL MPI_ABORT(MPI_COMM_WORLD,9,ierr)
         END IF
      END IF
   ELSE
      WRITE(*,200) 'File input not present. Halting.'
      CALL MPI_ABORT(MPI_COMM_WORLD,8,ierr)
   END IF
   !
   ! Pack buffer for broadcast.
   ibuff(:) = [nx,ny,nz,ng,iprc,jprc,kprc,relay,relay_size,write_flag]
END IF
!
! Root broadcasts input variables to other processors.
CALL MPI_BCAST(ibuff,10,MPI_INTEGER,0,MPI_COMM_WORLD,ierr)
CALL MPI_BCAST(chunk_in,3,MPI_INTEGER,0,MPI_COMM_WORLD,ierr)
nx = ibuff(1)
ny = ibuff(2)
nz = ibuff(3)
ng = ibuff(4)
iprc = ibuff(5)
jprc = ibuff(6)
kprc = ibuff(7)
relay = ibuff(8)
relay_size = ibuff(9)
write_flag = ibuff(10)
chunk_h5 = INT(chunk_in,HSIZE_T)

! Set the 3D process layout. We do not use Cartesian communicator routines
! because they have row-major ordering.
ipid = -1
jpid = -1
kpid = -1
rank_tmp = 0
DO k = 0,kprc-1
   DO j = 0,jprc-1
      DO i = 0,iprc-1
         IF (rank .EQ. rank_tmp) THEN
            ipid = i
            jpid = j
            kpid = k
         END IF
         rank_tmp = rank_tmp + 1
      END DO
   END DO
END DO
!
IF ((ipid .LT. 0) .OR. (jpid .LT. 0) .OR. (kpid .LT. 0)) THEN
   WRITE(*,400) 'Process',rank,'not assigned ipid/jpid/kpid. Halting.'
   CALL MPI_ABORT(MPI_COMM_WORLD,11,ierr)
END IF

! Size of the grid and starting index on each process.
nxp = nx/iprc
nyp = ny/jprc
nzp = nz/kprc
!
ist = ipid*nxp
jst = jpid*nyp
kst = kpid*nzp

! Make communicators used in parallel IO.
color = jpid + jprc*kpid
CALL MPI_COMM_SPLIT(MPI_COMM_WORLD,color,rank,io_comm,ierr)
CALL MPI_COMM_RANK(io_comm,io_rank,ierr)
CALL MPI_COMM_SIZE(io_comm,io_nprc,ierr)

! Memory allocation for signal.
ALLOCATE(phi(-ng+1:nxp+ng,-ng+1:nyp+ng,-ng+1:nzp+ng))
phi(:,:,:) = 0.0_RWP

! Grid spacing of physical grid. Used to set up and later check the signal.
dx = 2.0_RWP*PI/REAL(nx,RWP)
dy = 2.0_RWP*PI/REAL(ny,RWP)
dz = 2.0_RWP*PI/REAL(nz,RWP)

! Write out data to file, if requested.
IF (write_flag .NE. 0) THEN
   !
   ! Only loop over the physical portion of the arrays (not the ghost layers).
   DO k = 1,nzp
      zloc = REAL(kst+k-1,RWP)*dz
      DO j = 1,nyp
         yloc = REAL(jst+j-1,RWP)*dy
         DO i = 1,nxp
            xloc = REAL(ist+i-1,RWP)*dx
            !
            phi(i,j,k) = xloc + 7.0_RWP*yloc + 23.0_RWP*zloc + &
                         COS(3.0_RWP*xloc)*SIN(5.0_RWP*yloc)*COS(zloc)
         END DO
      END DO
   END DO
   !
   ! Write out the checkpoint.
   CALL MPI_BARRIER(MPI_COMM_WORLD,ierr)
   CALL WRITE_CHECKPOINT_2D(phi)
END IF

! Set phi with some garbage values before reading the dataset from file.
phi(:,:,:) = -1.37_RWP

! Read in the checkpoint using the VDS.
CALL MPI_BARRIER(MPI_COMM_WORLD,ierr)
IF (relay .GT. 0) THEN
   CALL READ_CHECKPOINT_VDS_RELAY('data',1,relay_size,phi)
ELSE
   CALL READ_CHECKPOINT_VDS('data',1,phi,.FALSE.)
END IF

! Check if the signal was read in properly.
err_prc = 0.0_RWP
DO k = 1,nzp
   zloc = REAL(kst+k-1,RWP)*dz
   DO j = 1,nyp
      yloc = REAL(jst+j-1,RWP)*dy
      DO i = 1,nxp
         xloc = REAL(ist+i-1,RWP)*dx
         !
         exact = xloc + 7.0_RWP*yloc + 23.0_RWP*zloc + &
                 COS(3.0_RWP*xloc)*SIN(5.0_RWP*yloc)*COS(zloc)
         !
         err = ABS(exact - phi(i,j,k))
         IF (err .GT. err_prc) err_prc = err
      END DO
   END DO
END DO
!
! Reduce maximum error over all processors and print to user.
CALL MPI_REDUCE(err_prc,err_max,1,MPIREAL,MPI_MAX,0,MPI_COMM_WORLD,ierr)
IF (rank .EQ. 0) THEN
   WRITE(*,300) 'MAXIMUM ERROR AFTER READING DATA:',err_max
END IF

! Free memory, etc.
DEALLOCATE(phi)

! Free additional communicators.
CALL MPI_COMM_FREE(io_comm,ierr)

! Finalize MPI and stop execution.
CALL MPI_FINALIZE(ierr)

! IO formatting.
200 FORMAT (A)
250 FORMAT (3X,A,1X,I4)
260 FORMAT (3X,A,1X,3I4)
300 FORMAT (A,1X,ES12.5)
400 FORMAT (A,1X,I6,1X,A,3I4)

CONTAINS

!> Subroutine to write the checkpoint in parallel with a 2D process layout.
!!
!! Variables related to the grid size, MPI process decomposition, etc. are taken
!! from the scope of the main program. This code was placed in a separate
!! subroutine to improve the readability of the main program, and so we can add
!! additional writing routines in the future and choose which one to use (e.g.,
!! writing with a 3D process decomposition instead of a 2D decomposition).
!!
!! Note that in the subroutine we use partial IO to avoid writing out the ghost
!! layers.
!!
!> @param[in] buf Array under the 3D process decomposition to write out.
SUBROUTINE WRITE_CHECKPOINT_2D(buf)
   USE ISO_C_BINDING,ONLY: C_LOC
   IMPLICIT NONE
   ! Calling arguments.
   REAL(KIND=RWP),DIMENSION(-ng+1:nxp+ng,-ng+1:nyp+ng,-ng+1:nzp+ng), &
      INTENT(IN) :: buf
   ! Local variables.
   ! HDF5 identifier for the files.
   INTEGER(KIND=HID_T) :: file_id
   ! HDF5 property list handle.
   INTEGER(KIND=HID_T) :: plist_id
   ! Variable used when making the property list.
   INTEGER :: info
   ! Arrays to define process memory layout and appropriate offset.
   INTEGER(KIND=HSIZE_T),DIMENSION(3) :: dimT_proc,dimW_proc,oset_proc
   ! Arrays to define communicator dataset and offset for this process.
   INTEGER(KIND=HSIZE_T),DIMENSION(3) :: dimT_comm,oset_comm
   ! Arrays for the overall checkpoint dataset.
   INTEGER(KIND=HSIZE_T),DIMENSION(3) :: dimT_vds,oset_vds
   ! HDF5 dataspace identifiers.
   INTEGER(KIND=HID_T) :: srcspc_id,vspace_id
   ! HDF5 dataset identifiers.
   INTEGER(KIND=HID_T) :: dset_id
   ! HDF5 precision type corresponding to the Fortran precision being used.
   INTEGER(KIND=HID_T) :: h5_real
   ! Number of files that will be written.
   INTEGER :: nfiles
   ! File name for each i_world communicator.
   INTEGER :: fnum
   ! Output file name.
   CHARACTER(LEN=FILE_NAME_LENGTH) :: fname
   ! Variables used to determine which directory this task writes to.
   INTEGER :: power,ndirs,dirnm
   ! Starting indices in j and k computational directions.
   INTEGER :: jst_comm,kst_comm
   ! Timers for IO performance.
   REAL(KIND=REAL64) :: timr,tmin,tmax,tavg
   ! Character arrays to hold the date and time stamps.
   CHARACTER(LEN=8) :: date
   CHARACTER(LEN=10) :: clock
   ! Looping indices.
   INTEGER :: j,k
   ! Error handling.
   INTEGER :: ierr

   ! Start overall timer for the IO process.
   timr = MPI_WTIME()

   ! Initialize the Fortran HDF5 interface.
   CALL H5OPEN_F(ierr)

   ! Determine HDF5 types corresponding to precisions used in the code.
   h5_real = H5KIND_TO_TYPE(RWP,H5_REAL_KIND)

   ! Set up file access property list with parallel I/O access.
   info = MPI_INFO_NULL
   CALL H5PCREATE_F(H5P_FILE_ACCESS_F,plist_id,ierr)
   CALL H5PSET_FAPL_MPIO_F(plist_id,io_comm,info,ierr)
   !
   ! Number of files to write and number of directories.
   nfiles = jprc*kprc
   power = POW2(nfiles)
   ndirs = 2**(power/2)
   !
   ! Each io_comm communicator writes to the same file. Form unique file name
   ! based on jpid and kpid. Directory chosen based on this number.
   fnum = jpid + jprc*kpid
   dirnm = MOD(fnum,ndirs)
   !
   ! Collectively open the file.
   CALL FILE_NAME('data',dirnm,'DATA',fnum,1,fname)
   CALL H5FCREATE_F(fname,H5F_ACC_TRUNC_F,file_id,ierr,access_prp=plist_id)
   !
   ! Close the file access property list.
   CALL H5PCLOSE_F(plist_id, ierr)

   ! Write attributes summarizing data and simulation setup and testing date.
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NPRC',C_LOC(nprc))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'IPRC',C_LOC(iprc))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'JPRC',C_LOC(jprc))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'KPRC',C_LOC(kprc))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NX',C_LOC(nx))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NY',C_LOC(ny))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NZ',C_LOC(nz))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NG',C_LOC(ng))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'JPID',C_LOC(jpid))
   CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'KPID',C_LOC(kpid))
   CALL DATE_AND_TIME(DATE=date,TIME=clock)
   CALL WRITE_ATTRIBUTE(file_id,'DATE',date)
   CALL WRITE_ATTRIBUTE(file_id,'TIME',clock)

   ! Memory size and offsets for the data on this process.
   dimT_proc = [INT(nxp+2*ng,HSIZE_T), &
                INT(nyp+2*ng,HSIZE_T), &
                INT(nzp+2*ng,HSIZE_T)]
   dimW_proc = [INT(nxp,HSIZE_T),INT(nyp,HSIZE_T),INT(nzp,HSIZE_T)]
   oset_proc = [INT(ng,HSIZE_T),INT(ng,HSIZE_T),INT(ng,HSIZE_T)]
   !
   ! Communicator-based file size and offsets for this process.
   dimT_comm = [INT(nx,HSIZE_T),INT(nyp,HSIZE_T),INT(nzp,HSIZE_T)]
   oset_comm = [INT(ist,HSIZE_T),0_HSIZE_T,0_HSIZE_T]
   !
   ! Write out the scalar field in parallel.
   IF (ANY(chunk_in .LT. 0)) THEN
      CALL WRITE_DATA_PARALLEL(3,'PHI',file_id,h5_real,h5_real, &
                               dimT_proc,dimW_proc,oset_proc, &
                               dimT_comm,oset_comm,C_LOC(buf))
   ELSE
      CALL WRITE_DATA_PARALLEL(3,'PHI',file_id,h5_real,h5_real, &
                               dimT_proc,dimW_proc,oset_proc, &
                               dimT_comm,oset_comm,C_LOC(buf), &
                               chunk=chunk_h5)
   END IF

   ! Close the HDF5 file.
   CALL H5FCLOSE_F(file_id,ierr)

   ! Root process makes a virtual dataset file to link all files together.
   IF (rank .EQ. 0) THEN
      ! Create the file for the virtual dataset.
      !
      CALL FILE_NAME('data','DATA',1,fname)
      CALL H5FCREATE_F(TRIM(fname),H5F_ACC_TRUNC_F,file_id,ierr)

      ! Add standard attributes for this dataset.
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NPRC',C_LOC(nprc))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'IPRC',C_LOC(iprc))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'JPRC',C_LOC(jprc))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'KPRC',C_LOC(kprc))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NX',C_LOC(nx))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NY',C_LOC(ny))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NZ',C_LOC(nz))
      CALL WRITE_ATTRIBUTE(file_id,H5T_NATIVE_INTEGER,'NG',C_LOC(ng))
      CALL DATE_AND_TIME(DATE=date,TIME=clock)
      CALL WRITE_ATTRIBUTE(file_id,'DATE',date)
      CALL WRITE_ATTRIBUTE(file_id,'TIME',clock)

      ! Create the dataspace for the virtual dataset.
      dimT_vds = [INT(nx,HSIZE_T),INT(ny,HSIZE_T),INT(nz,HSIZE_T)]
      CALL H5SCREATE_SIMPLE_F(3,dimT_vds,vspace_id,ierr)
      !
      ! VDS creation property list.
      CALL H5PCREATE_F(H5P_DATASET_CREATE_F,plist_id,ierr)
      CALL H5PSET_FILL_VALUE_F(plist_id,h5_real,-999.999_RWP,ierr)
      !
      ! The dataspace for all files (source dataspaces) is the same.
      CALL H5SCREATE_SIMPLE_F(3,dimT_comm,srcspc_id,ierr)

      ! Loop over each communicator-based file to link to the VDS.
      DO k = 0,kprc-1
         kst_comm = k*nzp
         !
         DO j = 0,jprc-1
            jst_comm = j*nyp
            !
            ! Number for this communicator-based checkpoint.
            fnum = j + jprc*k
            !
            ! Figure out the name of the file for this dataset.
            dirnm = MOD(fnum,ndirs)
            CALL FILE_NAME('data',dirnm,'DATA',fnum,1,fname)
            !
            ! Form the offset array for this dataset in the VDS.
            oset_vds = [0_HSIZE_T,INT(jst_comm,HSIZE_T),INT(kst_comm,HSIZE_T)]
            !
            ! Select the hyperslab from the VDS for this comm. file.
            CALL H5SSELECT_HYPERSLAB_F(vspace_id,H5S_SELECT_SET_F, &
                                       oset_vds,dimT_comm,ierr)
            !
            ! Map the hyperslab to the dataset in the comm-based HDF5 file.
            CALL H5PSET_VIRTUAL_F(plist_id,vspace_id,fname,'PHI', &
                                  srcspc_id,ierr)
         END DO
      END DO

      ! Now that the mapping is complete, create the VDS.
      CALL H5DCREATE_F(file_id,'PHI',h5_real,vspace_id,dset_id,ierr,plist_id)

      ! Close the dataset.
      CALL H5DCLOSE_F(dset_id,ierr)
      !
      ! Close the source dataspace.
      CALL H5SCLOSE_F(srcspc_id,ierr)
      !
      ! Close the virtual dataspace.
      CALL H5SCLOSE_F(vspace_id,ierr)
      !
      ! Close the property list.
      CALL H5PCLOSE_F(plist_id,ierr)
      !
      ! Close the HDF5 file for the VDS.
      CALL H5FCLOSE_F(file_id,ierr)
   END IF
   !
   ! Close the Fortran-HDF5 interface.
   CALL H5CLOSE_F(ierr)

   ! Report IO timings.
   timr = MPI_WTIME() - timr
   IF ((rank .EQ. 0) .OR. (rank .EQ. nprc-1) .OR. (rank .EQ. nprc/2-1)) THEN
      WRITE(*,300) 'PROCESS',rank,'IO 2D WRITE TIMING:',timr
   END IF
   CALL MPI_REDUCE(timr,tavg,1,MPI_REAL8,MPI_SUM,0,MPI_COMM_WORLD,ierr)
   CALL MPI_REDUCE(timr,tmin,1,MPI_REAL8,MPI_MIN,0,MPI_COMM_WORLD,ierr)
   CALL MPI_REDUCE(timr,tmax,1,MPI_REAL8,MPI_MAX,0,MPI_COMM_WORLD,ierr)
   IF (rank .EQ. 0) THEN
      tavg = tavg/REAL(nprc,REAL64)
      WRITE(*,350) 'MIN/AVG/MAX IO 2D WRITE TIMINGS:',tmin,tavg,tmax
   END IF

   ! IO formats.
   300 FORMAT (A,1X,I6,1X,A,1X,ES12.5)
   350 FORMAT (A,3ES12.5)
END SUBROUTINE WRITE_CHECKPOINT_2D

!> Subroutine to read in a checkpoint through the VDS.
!!
!> @param[in] path Path to the checkpoint.
!> @param[in] num Checkpoint number to read.
!> @param[in,out] buf Array to hold the data.
!> @param[in,optional] relay Parameter to indicate if the relay scheme is used.
SUBROUTINE READ_CHECKPOINT_VDS(path,num,buf,relay)
   USE ISO_C_BINDING,ONLY: C_LOC
   IMPLICIT NONE
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: path
   INTEGER,INTENT(IN) :: num
   REAL(KIND=RWP),DIMENSION(-ng+1:nxp+ng,-ng+1:nyp+ng,-ng+1:nzp+ng), &
        INTENT(INOUT), TARGET :: buf
   LOGICAL,OPTIONAL,INTENT(IN) :: relay
   ! Local variables.
   ! HDF5 identifier for the file.
   INTEGER(KIND=HID_T) :: file_id
   ! HDF5 precision type corresponding to the Fortran precision being used.
   INTEGER(KIND=HID_T) :: h5_real
   ! Pointer to the buffer array.
   TYPE(C_PTR) :: buf_ptr
   ! Name of the HDF5 file.
   CHARACTER(LEN=FILE_NAME_LENGTH) :: fname
   ! Arrays for holding sizes of arrays in memory and in the file.
   INTEGER(KIND=HSIZE_T),DIMENSION(3) :: dims_buff,dims_read
   ! Arrays for holding HDF5 offsets in memory and in the file.
   INTEGER(KIND=HSIZE_T),DIMENSION(3) :: oset_buff,oset_read
   ! Timers.
   REAL(KIND=REAL64) :: timr,tmin,tmax,tavg
   ! Relay flag. If active, do not attempt to reduce IO timings.
   LOGICAL :: relay_flag
   ! Error handling.
   INTEGER :: ierr

   ! Handle optional arguments.
   IF (PRESENT(relay)) THEN
      relay_flag = relay
   ELSE
      relay_flag = .FALSE.
   END IF

   ! IO timer.
   timr = MPI_WTIME()

   ! Open the Fortran HDF5 interface.
   CALL H5OPEN_F(ierr)

   ! Determine HDF5 types corresponding to precisions used in the code.
   h5_real = H5KIND_TO_TYPE(RWP,H5_REAL_KIND)

   ! Open up the HDF5 file.
   CALL FILE_NAME(TRIM(ADJUSTL(path)),'DATA',num,fname)
   CALL H5FOPEN_F(fname,H5F_ACC_RDONLY_F,file_id,ierr)

   ! Dimensions of the buffer holding the data.
   dims_buff = [INT(nxp+2*ng,HSIZE_T), &
                INT(nyp+2*ng,HSIZE_T), &
                INT(nzp+2*ng,HSIZE_T)]
   !
   ! Dimensions of the data to be read.
   dims_read = [INT(nxp,HSIZE_T),INT(nyp,HSIZE_T),INT(nzp,HSIZE_T)]
   !
   ! The offset in the us array to hold the data.
   oset_buff = [INT(ng,HSIZE_T),INT(ng,HSIZE_T),INT(ng,HSIZE_T)]
   !
   ! The offset in the VDS for this process.
   oset_read = [INT(ist,HSIZE_T),INT(jst,HSIZE_T),INT(kst,HSIZE_T)]

   ! Read in the data for this process.
   buf_ptr = C_LOC(buf)
   CALL READ_DATA(3,'PHI',file_id,h5_real,dims_buff,dims_read, &
                  oset_buff,oset_read,buf_ptr)

   ! Close the HDF5 file.
   CALL H5FCLOSE_F(file_id,ierr)

   ! Close the Fortran HDF5 interface.
   CALL H5CLOSE_F(ierr)

   ! Report IO timings.
   timr = MPI_WTIME() - timr
   IF ((rank .EQ. 0) .OR. (rank .EQ. nprc-1) .OR. (rank .EQ. nprc/2-1)) THEN
      WRITE(*,800) 'PROCESS',rank,'IO READ TIMING:',timr
   END IF

   ! Only reduce timings if relay is NOT active.
   IF (.NOT. relay_flag) THEN
      CALL MPI_REDUCE(timr,tavg,1,MPI_REAL8,MPI_SUM,0,MPI_COMM_WORLD,ierr)
      CALL MPI_REDUCE(timr,tmin,1,MPI_REAL8,MPI_MIN,0,MPI_COMM_WORLD,ierr)
      CALL MPI_REDUCE(timr,tmax,1,MPI_REAL8,MPI_MAX,0,MPI_COMM_WORLD,ierr)
      IF (rank .EQ. 0) THEN
         tavg = tavg/REAL(nprc,REAL64)
         WRITE(*,900) 'MIN/AVG/MAX IO READ TIMINGS:',tmin,tavg,tmax
      END IF
   END IF
   !
   ! IO formats.
   800 FORMAT (A,1X,I6,1X,A,1X,ES12.5)
   900 FORMAT (A,3ES12.5)
END SUBROUTINE READ_CHECKPOINT_VDS

!> Subroutine to read the checkpoint data with a relay scheme.
!!
!! For very large process counts we use a relay scheme to limit the number of
!! processors trying to simultaneously read data.
!!
!> @param[in] path Path to the checkpoint.
!> @param[in] num Checkpoint number to read.
!> @param[in] mread Number of processors to read data simultaneously.
!> @param[in,out] buf Array to hold the data.
SUBROUTINE READ_CHECKPOINT_VDS_RELAY(path,num,mread,buf)
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: path
   INTEGER,INTENT(IN) :: num,mread
   REAL(KIND=RWP),DIMENSION(-ng+1:nxp+ng,-ng+1:nyp+ng,-ng+1:nzp+ng), &
      INTENT(INOUT) :: buf
   ! Local variables.
   ! Used to determine number of processes to read at once.
   INTEGER :: nread,iread
   ! Signal sent from one process to the next in the relay.
   INTEGER :: rid
   ! MPI status for RECV.
   INTEGER,DIMENSION(MPI_STATUS_SIZE) :: stat
   ! MPI error handling.
   INTEGER :: ierr

   ! Determine number of batch reads and identify the starting processes.
   nread = nprc/mread
   iread = rank/mread
   IF (rank .EQ. 0) THEN
      WRITE(*,50) 'READING WITH RELAY SYSTEM. MREAD/NREAD/IREAD=', &
                  mread,nread,iread
   END IF

   ! Read in the data with the relay scheme.
   IF (iread .EQ. 0) THEN
      CALL READ_CHECKPOINT_VDS(path,num,buf,relay=.TRUE.)
   ELSE
      ! Wait for a signal from other process that has finished reading data
      ! before beginning to read data.
      CALL MPI_RECV(rid,1,MPI_INTEGER,rank-mread,rank-mread, &
                    MPI_COMM_WORLD,stat,ierr)
      !
      CALL READ_CHECKPOINT_VDS(path,num,buf,relay=.TRUE.)
   END IF

   ! When a process finishes reading data send signal to next process to
   ! start reading data.
   IF (iread .LT. (nread-1)) THEN
      CALL MPI_SSEND(rank,1,MPI_INTEGER,rank+mread,rank,MPI_COMM_WORLD,ierr)
   END IF

   ! IO formats.
   50 FORMAT (A,1X,3I7)
END SUBROUTINE READ_CHECKPOINT_VDS_RELAY

END PROGRAM KERNEL
