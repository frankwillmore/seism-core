!> @file IO.F90
!> @author Donated
!> @brief General IO routines.
!!
!! NOTES:
!!
!!    - 2016-05: Found bug in HDF5 1.10.0 H5AWRITE_F interface routines. They
!!      currently require INTENT(INOUT) for all variables instead of INTENT(IN).
!!      Recheck after first patch to 1.10.0 to see if it is fixed.
MODULE IO

! Required modules.
USE HDF5

IMPLICIT NONE

!> Length of strings containing file names.
INTEGER,PARAMETER,PUBLIC :: FILE_NAME_LENGTH = 256

!> Interface for forming output file names.
INTERFACE FILE_NAME
   MODULE PROCEDURE FILE_NAME_1
   MODULE PROCEDURE FILE_NAME_2
   MODULE PROCEDURE FILE_NAME_3
   MODULE PROCEDURE FILE_NAME_4
   MODULE PROCEDURE FILE_NAME_5
   MODULE PROCEDURE FILE_NAME_6
END INTERFACE FILE_NAME

!> Interface for writing attributes to HDF5 files.
INTERFACE WRITE_ATTRIBUTE
   MODULE PROCEDURE WRITE_ATTRIBUTE_SCALAR
   MODULE PROCEDURE WRITE_ATTRIBUTE_RANK1
   MODULE PROCEDURE WRITE_ATTRIBUTE_CHAR
END INTERFACE WRITE_ATTRIBUTE

CONTAINS

!> Procedure to form filename with root and subdirectories.
!!
!! The file name takes the following form:
!!
!!    rootdir/subdir/froot_num1_num2.h5,
!!
!! where num1 is written with 6 digits, and num2 is written with 2.
!! Currently, num1 is intended for the process id and num2 for the checkpoint
!! number.
!!
!> @param[in] rootdir Root directory for the file.
!> @param[in] subdir Subdirectory for the file.
!> @param[in] froot Root name for the output file.
!> @param[in] num1 First output number for the file.
!> @param[in] num2 Second output number for the file.
!> @param[out] fout Output string with the file name.
SUBROUTINE FILE_NAME_1(rootdir, subdir, froot, num1, num2, fout)
   IMPLICIT NONE
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: rootdir, froot
   INTEGER,INTENT(IN) :: subdir, num1, num2
   CHARACTER(LEN=FILE_NAME_LENGTH),INTENT(OUT) :: fout
   ! Local variables.
   ! String used to form the subdirectory name as an integer with no padding.
   CHARACTER(LEN=6) :: numer
   !
   ! Form the subdirectory name keeping the existing format.
   WRITE(numer,10) subdir
   10 FORMAT (I6)
   !
   ! Form the file name.
   WRITE(fout,20) TRIM(rootdir),TRIM(ADJUSTL(numer)),TRIM(froot),num1,num2
   20 FORMAT (A,'/',A,'/',A,'_',I6.6,'_',I2.2,'.h5')
END SUBROUTINE FILE_NAME_1

!> Procedure to form filename with root and subdirectories.
!!
!! The file name takes the following form:
!!
!!    rootdir/subdir/froot_num.h5,
!!
!! where num is written with 4 digits.
!!
!> @param[in] rootdir Root directory for the file.
!> @param[in] subdir Subdirectory for the file.
!> @param[in] froot Root name for the output file.
!> @param[in] num Output number for the file.
!> @param[out] fout Output string with the file name.
SUBROUTINE FILE_NAME_2(rootdir, subdir, froot, num, fout)
   IMPLICIT NONE
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: rootdir, froot
   INTEGER,INTENT(IN) :: subdir, num
   CHARACTER(LEN=FILE_NAME_LENGTH),INTENT(OUT) :: fout
   ! Local variables.
   ! String used to form the subdirectory name as an integer with no padding.
   CHARACTER(LEN=6) :: numer
   !
   ! Form the subdirectory name keeping the existing format.
   WRITE(numer,10) subdir
   10 FORMAT (I6)
   !
   ! Form the file name.
   WRITE(fout,20) TRIM(rootdir),TRIM(ADJUSTL(numer)),TRIM(froot),num
   20 FORMAT (A,'/',A,'/',A,'_',I4.4,'.h5')
END SUBROUTINE FILE_NAME_2

!> Procedure to form filename with single root directory.
!!
!! The file name takes the following form:
!!
!!    rootdir/froot_num.h5
!!
!! where num is written with 2 digits.
!!
!> @param[in] rootdir Root directory for the file.
!> @param[in] froot Root name for the output file.
!> @param[in] num Output number for the file.
!> @param[out] fout Output string with the file name.
SUBROUTINE FILE_NAME_3(rootdir, froot, num, fout)
   IMPLICIT NONE
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: rootdir, froot
   INTEGER,INTENT(IN) :: num
   CHARACTER(LEN=FILE_NAME_LENGTH),INTENT(OUT) :: fout
   !
   WRITE(fout,20) TRIM(rootdir),TRIM(froot),num
   20 FORMAT (A,'/',A,'_',I2.2,'.h5')
END SUBROUTINE FILE_NAME_3

!> Procedure to form a filename with root directory that is a number.
!!
!! The file name takes the following form:
!!
!!    subdir/froot_num1_num2.h5,
!!
!! where num1 is written with 6 digits, and num2 is written with 2.
!! Currently, num1 is intended for the process id and num2 for the checkpoint
!! number.
!!
!> @param[in] subdir Directory for the file (integer).
!> @param[in] froot Root name for the output file.
!> @param[in] num1 First number for the file (process ID).
!> @param[in] num2 Second number for the file (checkpoint number).
!> @param[out] fout Output string with the file name.
SUBROUTINE FILE_NAME_4(subdir, froot, num1, num2, fout)
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER,INTENT(IN) :: subdir, num1, num2
   CHARACTER(LEN=*),INTENT(IN) :: froot
   CHARACTER(LEN=FILE_NAME_LENGTH),INTENT(OUT) :: fout
   ! Local variables.
   ! String used to form the subdirectory name as an integer with no padding.
   CHARACTER(LEN=6) :: numer
   !
   ! Translate the subdirectory to a string.
   WRITE(numer,10) subdir
   10 FORMAT (I6)
   !
   ! Form the file name.
   WRITE(fout,20) TRIM(ADJUSTL(numer)),TRIM(froot),num1,num2
   20 FORMAT (A,'/',A,'_',I6.6,'_',I2.2,'.h5')
END SUBROUTINE FILE_NAME_4

!> Procedure to form a filename with root path, sub path, and number.
!!
!! The file name takes the following form:
!!
!!    rootdir/subdir/froot_num1.h5
!!
!> @param[in] rootdir Root directory for the file.
!> @param[in] subdir Subdirectory (relative to rootdir) for the file.
!> @param[in] froot Root name for the file.
!> @param[in] num Number for the file (filled to 2 integer width).
!> @param[out] fout Output string with the file name.
SUBROUTINE FILE_NAME_5(rootdir, subdir, froot, num, fout)
   IMPLICIT NONE
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: rootdir, subdir, froot
   INTEGER,INTENT(IN) :: num
   CHARACTER(LEN=FILE_NAME_LENGTH),INTENT(OUT) :: fout
   !
   ! Form the file name.
   WRITE(fout,20) TRIM(ADJUSTL(rootdir)),TRIM(ADJUSTL(subdir)), &
                  TRIM(ADJUSTL(froot)),num
   20 FORMAT (A,'/',A,'/',A,'_',I2.2,'.h5')
END SUBROUTINE FILE_NAME_5

!> Procedure to form filename with single root directory.
!!
!! The file name takes the following form:
!!
!!    rootdir/froot_num.h5
!!
!! where num is written with a user-specified number of digits. This is a
!! generalized version of FILE_NAME_3.
!!
!> @param[in] rootdir Root directory for the file.
!> @param[in] froot Root name for the output file.
!> @param[in] num Output number for the file.
!> @param[in] dig Number of digits to use when writing.
!> @param[out] fout Output string with the file name.
SUBROUTINE FILE_NAME_6(rootdir, froot, num, dig, fout)
   IMPLICIT NONE
   ! Calling arguments.
   CHARACTER(LEN=*),INTENT(IN) :: rootdir, froot
   INTEGER,INTENT(IN) :: num,dig
   CHARACTER(LEN=FILE_NAME_LENGTH),INTENT(OUT) :: fout
   ! Local variables.
   ! Used to form the IO format for the number.
   CHARACTER(LEN=16) :: iotmp,iofmt
   ! Buffer to write num with specified number of digits.
   CHARACTER(LEN=32) :: buf
   !
   WRITE(iotmp,25) dig
   WRITE(iofmt,30) TRIM(iotmp),TRIM(iotmp)
   WRITE(buf,iofmt) num
   !
   WRITE(fout,20) TRIM(rootdir),TRIM(froot),TRIM(buf)
   !
   20 FORMAT (A,'/',A,'_',A,'.h5')
   25 FORMAT (I6)
   30 FORMAT ('(I',A,'.',A,')')
END SUBROUTINE FILE_NAME_6

!> Procedure to return LOG2 of a number. If the number is not a power of 2,
!! the function returns zero.
!!
!> @param[in] n Number for evaluation.
!> @return LOG2(n) if n is a power of two, else 0.
INTEGER FUNCTION POW2(n)
   USE ISO_FORTRAN_ENV,ONLY: REAL32
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER,INTENT(IN) :: n
   ! Local variables.
   ! LOG2 of n as a real.
   REAL(KIND=REAL32) :: p
   !
   ! Calculate the LOG base 2 of the number.
   p = LOG(REAL(n,REAL32))/LOG(2.0_REAL32)
   !
   ! Round to the nearest integer.
   POW2 = NINT(p)
   !
   ! Return 0 if n is not a power of 2.
   IF (2**POW2 .NE. n) THEN
      POW2 = 0
   END IF
END FUNCTION POW2

!> Procedure to write scalar attributes.
!!
!> @param[in] h5_id HDF5 object to which the attribute is attached.
!> @param[in] h5_kind HDF5 kind for the attribute.
!> @param[in] att_name Name for the attribute.
!> @param[in] dat The data to be written.
SUBROUTINE WRITE_ATTRIBUTE_SCALAR(h5_id, h5_kind, att_name, dat)
   USE ISO_C_BINDING,ONLY: C_PTR
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id, h5_kind
   CHARACTER(LEN=*),INTENT(IN) :: att_name
   TYPE(C_PTR),INTENT(IN) :: dat
   ! Local variables.
   ! HDF5 dataspace identifier for the attribute.
   INTEGER(KIND=HID_T) :: aspace_id
   ! HDF5 identifier for the attribute.
   INTEGER(KIND=HID_T) :: attr_id
   ! Error handling.
   INTEGER :: ierr
   !
   ! Create the dataspace for the attribute.
   CALL H5SCREATE_F(H5S_SCALAR_F, aspace_id, ierr)
   !
   ! Create the attribute.
   CALL H5ACREATE_F(h5_id, att_name, h5_kind, aspace_id, attr_id, ierr)
   !
   ! Write to the attribute.
   CALL H5AWRITE_F(attr_id, h5_kind, dat, ierr)
   !
   ! Close the attribute.
   CALL H5ACLOSE_F(attr_id, ierr)
   !
   ! Close the dataspace for the attribute.
   CALL H5SCLOSE_F(aspace_id, ierr)
END SUBROUTINE WRITE_ATTRIBUTE_SCALAR

!> Procedure to write 1D array attributes.
!!
!> @param[in] h5_id HDF5 object to which the attribute is attached.
!> @param[in] h5_kind HDF5 kind for the attribute.
!> @param[in] att_name Name for the attribute.
!> @param[in] dims Dimensions of the data.
!> @param[in] dat The data to be written.
SUBROUTINE WRITE_ATTRIBUTE_RANK1(h5_id, h5_kind, att_name, dims, dat)
   USE ISO_C_BINDING,ONLY: C_PTR
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id, h5_kind
   CHARACTER(LEN=*),INTENT(IN) :: att_name
   INTEGER(KIND=HSIZE_T),DIMENSION(1),INTENT(IN) :: dims
   TYPE(C_PTR),INTENT(IN) :: dat
   ! Local variables.
   ! HDF5 dataspace identifier for the attribute.
   INTEGER(KIND=HID_T) :: aspace_id
   ! HDF5 identifier for the attribute.
   INTEGER(KIND=HID_T) :: attr_id
   ! Error handling.
   INTEGER :: ierr
   !
   ! Create the dataspace for the attribute.
   CALL H5SCREATE_SIMPLE_F(1, dims, aspace_id, ierr)
   !
   ! Create the attribute.
   CALL H5ACREATE_F(h5_id, att_name, h5_kind, aspace_id, attr_id, ierr)
   !
   ! Write to the attribute.
   CALL H5AWRITE_F(attr_id, h5_kind, dat, ierr)
   !
   ! Close the attribute.
   CALL H5ACLOSE_F(attr_id, ierr)
   !
   ! Close the dataspace for the attribute.
   CALL H5SCLOSE_F(aspace_id, ierr)
END SUBROUTINE WRITE_ATTRIBUTE_RANK1

!> Procedure to write a character attribute.
!!
!> @param[in] h5_id HDF5 object to which the attribute is attached.
!> @param[in] att_name Name for the attribute.
!> @param[in] dat The data to be written.
SUBROUTINE WRITE_ATTRIBUTE_CHAR(h5_id, att_name, dat)
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id
   CHARACTER(LEN=*),INTENT(IN) :: att_name
   CHARACTER(LEN=*),INTENT(IN) :: dat
   ! Local variables.
   ! HDF5 dataspace identifier for the attribute.
   INTEGER(KIND=HID_T) :: aspace_id
   ! HDF5 identifier for the attribute.
   INTEGER(KIND=HID_T) :: attr_id
   ! HDF5 identifier for the data type.
   INTEGER(KIND=HID_T) :: type_id
   ! Dimensions of the data to be written.
   INTEGER(KIND=HSIZE_T),DIMENSION(1),PARAMETER :: dims = [1]
   ! Length of the character string.
   INTEGER(KIND=SIZE_T) :: strlen
   ! Error handling.
   INTEGER :: ierr
   !
   ! Create the dataspace for the attribute.
   CALL H5SCREATE_F(H5S_SCALAR_F, aspace_id, ierr)
   !
   ! Get the length of the string figured out.
   strlen = INT(LEN_TRIM(dat), HSIZE_T)
   CALL H5TCOPY_F(H5T_NATIVE_CHARACTER, type_id, ierr)
   CALL H5TSET_SIZE_F(type_id, strlen, ierr)
   !
   ! Create the attribute.
   CALL H5ACREATE_F(h5_id, att_name, type_id, aspace_id, attr_id, ierr)
   !
   ! Write to the attribute.
   CALL H5AWRITE_F(attr_id, type_id, TRIM(dat), dims, ierr)
   !
   ! Close the attribute.
   CALL H5ACLOSE_F(attr_id, ierr)
   !
   ! Close the dataspace for the attribute.
   CALL H5SCLOSE_F(aspace_id, ierr)
END SUBROUTINE WRITE_ATTRIBUTE_CHAR

!> Procedure to read attributes.
!!
!> @param[in] h5_id HDF5 object which holds the attribute.
!> @param[in] h5_kind HDF5 kind for the buffer to hold the attribute.
!> @param[in] att_name Name of the attribute.
!> @param[out] dat Variable to store the attribute.
SUBROUTINE READ_ATTRIBUTE(h5_id, h5_kind, att_name, dat)
   USE ISO_C_BINDING,ONLY: C_PTR
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id, h5_kind
   CHARACTER(LEN=*),INTENT(IN) :: att_name
   TYPE(C_PTR),INTENT(OUT) :: dat
   ! Local variables.
   ! HDF5 identifier for the attribute.
   INTEGER(KIND=HID_T) :: att_id
   ! Error handling.
   INTEGER :: ierr
   !
   ! Open the attribute.
   CALL H5AOPEN_F(h5_id, att_name, att_id, ierr)
   !
   ! Read the attribute.
   CALL H5AREAD_F(att_id, h5_kind, dat, ierr)
   !
   ! Close the attribute.
   CALL H5ACLOSE_F(att_id, ierr)
END SUBROUTINE READ_ATTRIBUTE

!> Procedure to write datasets to file.
!!
!> @param[in] rank Dimensionality of the of the data.
!> @param[in] dims Dimensions of the dataset.
!> @param[in] dname Output name for the dataset.
!> @param[in] h5_id HDF5 ID for the object where data will be written.
!> @param[in] h5_dat HDF5 kind for the buffer holding the data.
!> @param[in] h5_out HDF5 kind to write to file (can differ from h5_dat).
!> @param[in] dat Pointer to the dataset.
!> @param[in,optional] gbool Whether to activate gzip compression.
!> @param[in,optional] level Level of compression for gzip.
!> @param[in,optional] shuff Whether to add the shuffle filter.
!> @param[in,optional] chunk Chunking dimensions.
SUBROUTINE WRITE_DATA(rank, dims, dname, h5_id, h5_dat, h5_out, dat, &
                      gbool, level, shuff, chunk)
   USE ISO_C_BINDING, ONLY: C_PTR
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER,INTENT(IN) :: rank
   INTEGER(KIND=HSIZE_T),DIMENSION(rank),INTENT(IN) :: dims
   CHARACTER(LEN=*),INTENT(IN) :: dname
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id, h5_dat, h5_out
   TYPE(C_PTR),INTENT(IN) :: dat
   LOGICAL,OPTIONAL,INTENT(IN) :: gbool, shuff
   INTEGER,OPTIONAL,INTENT(IN) :: level
   INTEGER(KIND=HSIZE_T),DIMENSION(rank),OPTIONAL,INTENT(IN) :: chunk
   ! Local variables.
   ! HDF5 identifier for the dataspace and dataset.
   INTEGER(KIND=HID_T) :: dataspace_id, dataset_id
   ! HDF5 identifier for the dataset creation property list.
   INTEGER(KIND=HID_T) :: plist_id
   ! Used to handle optional logical arguments.
   LOGICAL :: enable_compress, shuffle_filter, enable_chunking
   INTEGER :: compress_level
   INTEGER(KIND=HSIZE_T) :: chunk_size
   INTEGER(KIND=HSIZE_T),DIMENSION(rank) :: chunk_dims
   ! Error handling.
   INTEGER :: ierr
   !
   ! Handle optional arguments. Compression and shuffle off by default.
   ! Shuffle filter only used if compression is activated.
   IF (PRESENT(gbool)) THEN
      enable_compress = gbool
      !
      ! Turn off shuffle filter by default.
      IF (PRESENT(shuff)) THEN
         shuffle_filter = shuff
      ELSE
         shuffle_filter = .FALSE.
      END IF
      !
      ! Set gzip compression level of 9 by default.
      IF (PRESENT(level)) THEN
         compress_level = level
      ELSE
         compress_level = 9
      END IF
   ELSE
      enable_compress = .FALSE.
      shuffle_filter = .FALSE.
   END IF
   !
   ! Check chunking options. Turned off by default. When compression is
   ! enable chunking must be used. We start with a chunking size of 32 and
   ! and reduce it by factors of 2 until the final chunk size works for all
   ! dimensions of the dataset.
   IF (PRESENT(chunk)) THEN
      enable_chunking = .TRUE.
      chunk_dims(:) = chunk(:)
   ELSE IF (enable_compress) THEN
      enable_chunking = .TRUE.
      chunk_size = 32_HSIZE_T
      DO WHILE (chunk_size .GT. MINVAL(dims))
         chunk_size = chunk_size/2_HSIZE_T
      END DO
      chunk_dims(:) = chunk_size
   ELSE
      enable_chunking = .FALSE.
   END IF
   !
   ! Create the dataspace for the data set.
   CALL H5SCREATE_SIMPLE_F(rank, dims, dataspace_id, ierr)
   !
   ! Use property list to control dataset creation, if needed.
   IF (enable_compress .OR. enable_chunking) THEN
      !
      ! Make the property list.
      CALL H5PCREATE_F(H5P_DATASET_CREATE_F, plist_id, ierr)
      !
      ! Add the chunking option (this if statement is redundant).
      IF (enable_chunking) THEN
         CALL H5PSET_CHUNK_F(plist_id, rank, chunk_dims, ierr)
      END IF
      !
      ! Add the shuffle filter.
      IF (shuffle_filter) THEN
         CALL H5PSET_SHUFFLE_F(plist_id, ierr)
      END IF
      !
      ! Add the compression options.
      IF (enable_compress) THEN
         CALL H5PSET_DEFLATE_F(plist_id, compress_level, ierr)
      END IF
      !
      ! Make the dataset with the property list.
      CALL H5DCREATE_F(h5_id, dname, h5_out, dataspace_id, dataset_id, &
                       ierr, dcpl_id=plist_id)
      !
      ! Destroy the property list.
      CALL H5PCLOSE_F(plist_id, ierr)
   ELSE
      !
      ! Create a dataset in hdf5_id with default properties.
      CALL H5DCREATE_F(h5_id, dname, h5_out, dataspace_id, dataset_id, ierr)
   END IF
   !
   ! Write to the dataset.
   CALL H5DWRITE_F(dataset_id, h5_dat, dat, ierr)
   !
   ! Close the dataset.
   CALL H5DCLOSE_F(dataset_id, ierr)
   !
   ! Close the dataspace.
   CALL H5SCLOSE_F(dataspace_id, ierr)
END SUBROUTINE WRITE_DATA

!> Procedure to write datasets in parallel.
!!
!> @param[in] rank Dimensionality of the of the data.
!> @param[in] dname Output name for the dataset.
!> @param[in] h5_id HDF5 ID for the object where data will be written.
!> @param[in] h5_dat HDF5 kind for the buffer holding the data.
!> @param[in] h5_out HDF5 kind to write to file (can differ from h5_dat).
!> @param[in] dimT_proc Total array size for data on this processor.
!> @param[in] dimW_proc Dimensions of data on this process to write out.
!> @param[in] oset_proc Offset for the data to write from this process.
!> @param[in] dimT_file Total size of the data to write out.
!> @param[in] oset_file Offset for the data to be written from this process.
!> @param[in] dat Pointer to dataset for each process.
!> @param[in,optional] chunk Chunking dimensions for the dataset.
SUBROUTINE WRITE_DATA_PARALLEL(rank, dname, h5_id, h5_dat, h5_out, &
                               dimT_proc, dimW_proc, oset_proc, &
                               dimT_file, oset_file, dat, chunk)
   USE ISO_C_BINDING,ONLY: C_PTR
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER,INTENT(IN) :: rank
   CHARACTER(LEN=*),INTENT(IN) :: dname
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id, h5_dat, h5_out
   INTEGER(KIND=HSIZE_T),DIMENSION(rank),INTENT(IN) :: dimT_proc, dimW_proc,&
                                                       oset_proc, dimT_file,&
                                                       oset_file
   TYPE(C_PTR),INTENT(IN) :: dat
   INTEGER(KIND=HSIZE_T),DIMENSION(rank),OPTIONAL,INTENT(IN) :: chunk
   ! Local variables.
   ! HDF5 dataspace identifier for the file space.
   INTEGER(KIND=HID_T) :: fle_space
   ! HDF5 dataspace identifier for the memory space on this process.
   INTEGER(KIND=HID_T) :: mem_space
   ! HDF5 property list identifier.
   INTEGER(KIND=HID_T) :: plist_id
   ! HDF5 dataset identifier.
   INTEGER(KIND=HID_T) :: dset_id
   ! Used to handle optional chunking parameters.
   INTEGER(KIND=HSIZE_T),DIMENSION(rank) :: chunk_dims
   ! Error handling.
   INTEGER :: ierr

   ! Handle optional chunking parameter.
   IF (PRESENT(chunk)) THEN
      chunk_dims(:) = chunk(:)
   ELSE
      chunk_dims(:) = dimW_proc(:)
   END IF

   ! Create the dataspace for the memory on this process.
   CALL H5SCREATE_SIMPLE_F(rank,dimT_proc,mem_space,ierr)
   !
   ! Select the region of the memory space to write out.
   CALL H5SSELECT_HYPERSLAB_F(mem_space,H5S_SELECT_SET_F,oset_proc, &
                              dimW_proc,ierr)

   ! Create the dataspace for the file.
   CALL H5SCREATE_SIMPLE_F(rank,dimT_file,fle_space,ierr)
   !
   ! Create the dataset with chunking based on the process-based data size.
   ! Note that this assumes all processes have the same array dimensions,
   ! which is the case in our code.
   CALL H5PCREATE_F(H5P_DATASET_CREATE_F,plist_id,ierr)
   CALL H5PSET_CHUNK_F(plist_id,rank,chunk_dims,ierr)
   CALL H5DCREATE_F(h5_id,dname,h5_out,fle_space,dset_id,ierr,plist_id)
   CALL H5PCLOSE_F(plist_id,ierr)
   !
   ! Select the region of the filespace dataspace for this process.
   CALL H5SSELECT_HYPERSLAB_F(fle_space,H5S_SELECT_SET_F,oset_file, &
                              dimW_proc,ierr)

   ! Create the property list for collective dataset writing.
   CALL H5PCREATE_F(H5P_DATASET_XFER_F,plist_id,ierr)
   CALL H5PSET_DXPL_MPIO_F(plist_id,H5FD_MPIO_COLLECTIVE_F,ierr)
   !
   ! Write the data collectively.
   CALL H5DWRITE_F(dset_id,h5_dat,dat,ierr, &
                   mem_space_id=mem_space, &
                   file_space_id=fle_space, &
                   xfer_prp=plist_id)

   ! Close the property list.
   CALL H5PCLOSE_F(plist_id,ierr)
   !
   ! Close the dataset.
   CALL H5DCLOSE_F(dset_id,ierr)
   !
   ! Close the dataspaces.
   CALL H5SCLOSE_F(fle_space,ierr)
   CALL H5SCLOSE_F(mem_space,ierr)
END SUBROUTINE WRITE_DATA_PARALLEL

!> Procedure to read n-dimensional hyperslabs of data into n-dimensional
!! hyperslabs of buffer arrays.
!!
!> @param[in] rank Dimensionality of the data.
!> @param[in] dname Output name for the dataset.
!> @param[in] h5_id HDF5 identifier to the object containing the dataset.
!> @param[in] h5_dat HDF5 kind for the buffer to hold the data.
!> @param[in] dims_buff Total dimensions of the buffer to hold data.
!> @param[in] dims_read Amount of data to read in.
!> @param[in] oset_buff Offset in the buffer to store the data.
!> @param[in] oset_read Offset in the file to start reading data.
!> @param[in,out] dat Pointer to array to hold the data.
SUBROUTINE READ_DATA(rank, dname, h5_id, h5_dat, dims_buff, dims_read, &
                     oset_buff, oset_read, dat)
   USE ISO_C_BINDING,ONLY: C_PTR
   IMPLICIT NONE
   ! Calling arguments.
   INTEGER,INTENT(IN) :: rank
   CHARACTER(LEN=*),INTENT(IN) :: dname
   INTEGER(KIND=HID_T),INTENT(IN) :: h5_id, h5_dat
   INTEGER(KIND=HSIZE_T),DIMENSION(rank),INTENT(IN) :: dims_buff,dims_read
   INTEGER(KIND=HSIZE_T),DIMENSION(rank),INTENT(IN) :: oset_buff,oset_read
   TYPE(C_PTR),INTENT(INOUT) :: dat
   ! Local variables.
   ! HDF5 identifiers for the memory and file dataspaces, and datasets.
   INTEGER(KIND=HID_T) :: fspace_id, mspace_id, dset_id
   ! Error handling.
   INTEGER :: ierr
   !
   ! Open the dataset.
   CALL H5DOPEN_F(h5_id, dname, dset_id, ierr)
   !
   ! Get the datasets datspace identifier.
   CALL H5DGET_SPACE_F(dset_id, fspace_id, ierr)
   !
   ! Select the hyperslab in the dataset.
   CALL H5SSELECT_HYPERSLAB_F(fspace_id, H5S_SELECT_SET_F, oset_read, &
                              dims_read, ierr)
   !
   ! Create the memory dataspace.
   CALL H5SCREATE_SIMPLE_F(rank, dims_buff, mspace_id, ierr)
   !
   ! Select the hyperslab in memory.
   CALL H5SSELECT_HYPERSLAB_F(mspace_id, H5S_SELECT_SET_F, &
                              oset_buff, dims_read, ierr)
   !
   ! Read data from hyperslab in the file into the hyperslab in memory.
   CALL H5DREAD_F(dset_id, h5_dat, dat, ierr, mspace_id, fspace_id)
   !
   ! Close the memory space.
   CALL H5SCLOSE_F(mspace_id, ierr)
   !
   ! Close the file space dataspace.
   CALL H5SCLOSE_F(fspace_id, ierr)
   !
   ! Close the dataset.
   CALL H5DCLOSE_F(dset_id, ierr)
END SUBROUTINE READ_DATA

END MODULE IO
