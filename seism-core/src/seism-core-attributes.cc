// seism-core-attributes.cc
#include <hdf5.h>
#include "seism-core-attributes.hh"
#include <cassert>
#include <iostream>

using namespace std;

void seismCoreAttributes::init()
{
    // create the inner array and character types
    vls_t = H5Tcopy(H5T_C_S1);
    H5Tset_size(vls_t, H5T_VARIABLE);
//    fls_t = H5Tcopy(H5T_C_S1);
//    H5Tset_size(fls_t, 256); // fixed length for use_function_argv
    hsize_t adims[] = {3};
    dim3_t = H5Tarray_create(H5T_NATIVE_ULLONG, 1, adims);
    
    attributes_t = H5Tcreate(H5T_COMPOUND, sizeof(seismCoreAttributes)); 
    H5Tinsert(attributes_t, "name", HOFFSET(seismCoreAttributes, name), 
              vls_t);
    H5Tinsert(attributes_t, "processor_dims", HOFFSET(seismCoreAttributes,
              processor_dims), dim3_t);
    H5Tinsert(attributes_t, "chunk_dims", HOFFSET(seismCoreAttributes, 
              chunk_dims), dim3_t);
    H5Tinsert(attributes_t, "domain_dims", HOFFSET(seismCoreAttributes, 
              domain_dims), dim3_t);
    H5Tinsert(attributes_t, "simulation_time", HOFFSET(seismCoreAttributes, 
              simulation_time), H5T_NATIVE_UINT);
    H5Tinsert(attributes_t, "collective_write", HOFFSET(seismCoreAttributes,
              collective_write), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "precreate", HOFFSET(seismCoreAttributes, 
              precreate), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "set_collective_metadata", HOFFSET(
              seismCoreAttributes, set_collective_metadata), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "never_fill", HOFFSET(seismCoreAttributes, 
              never_fill), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "deflate", HOFFSET(seismCoreAttributes, 
              deflate), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "zfp", HOFFSET(seismCoreAttributes, 
              zfp), H5T_NATIVE_INT);

    // data types that are in the class... 
    // H5Tinsert(attributes_h5t, "vls_type_c_id", 
    //     HOFFSET(seismCoreAttributes, vls_type_c_id), H5T_NATIVE_INT);
    // H5Tinsert(attributes_h5t, "dim_h5t", 
    //     HOFFSET(seismCoreAttributes, dim_h5t), H5T_NATIVE_INT);
    // H5Tinsert(attributes_h5t, "attributes_h5t", 
    //     HOFFSET(seismCoreAttributes, attributes_h5t), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "use_function_lib", HOFFSET(seismCoreAttributes, 
              use_function_lib), vls_t);
    H5Tinsert(attributes_t, "use_function_name", HOFFSET(seismCoreAttributes, 
              use_function_name), vls_t);
    H5Tinsert(attributes_t, "use_function_argc", HOFFSET(seismCoreAttributes,
              use_function_argc), H5T_NATIVE_INT);
    H5Tinsert(attributes_t, "use_function_argv", HOFFSET(seismCoreAttributes, 
              use_function_argv), vls_t);
}

// the object has been created and initialized before calling this 
void seismCoreAttributes::writeAttributesToFile(hid_t file_id)
{
    // commit the compound type to HDF5 file
    assert(H5Tcommit(file_id, "seismCoreAttributes", attributes_t, 
                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) >= 0);
    
    // create scalar dataspace for attributes        
    hid_t space_id = H5Screate(H5S_SCALAR);
    hid_t acpl_id = H5P_DEFAULT;
    hid_t aapl_id = H5P_DEFAULT;

    // create the attribute
    hid_t attr_id = H5Acreate( file_id, "simulation_attributes", 
            attributes_t, space_id, acpl_id, aapl_id );

    // write attribute
    assert(H5Awrite(attr_id, attributes_t, this) >= 0); 

    // close resources 
    H5Aclose(attr_id);
    H5Sclose(space_id);
}

// constructor to create attributes object from current sim data
seismCoreAttributes::seismCoreAttributes
(
    char * _name,
    hsize_t *_processor_dims,
    hsize_t *_chunk_dims,
    hsize_t *_domain_dims,
    unsigned int _simulation_time,
    int _collective_write,
    int _precreate,
    int _set_collective_metadata,
    int _never_fill,
    int _deflate,
    int _zfp,
    char* _use_function_lib,
    char* _use_function_name,
    int _use_function_argc,
    char* _use_function_argv 
)   
{
    name = _name;
    processor_dims[0] = _processor_dims[0];
    processor_dims[1] = _processor_dims[1];
    processor_dims[2] = _processor_dims[2];
    chunk_dims[0] = _chunk_dims[0];
    chunk_dims[1] = _chunk_dims[1];
    chunk_dims[2] = _chunk_dims[2];
    domain_dims[0] = _domain_dims[0];
    domain_dims[1] = _domain_dims[1];
    domain_dims[2] = _domain_dims[2];
    simulation_time = _simulation_time;
    collective_write = _collective_write;
    precreate = _precreate;
    set_collective_metadata = _set_collective_metadata;
    never_fill = _never_fill;
    deflate = _deflate;
    zfp = _zfp;
    use_function_lib = _use_function_lib;
    use_function_name = _use_function_name;
    use_function_argc = _use_function_argc;
    use_function_argv = _use_function_argv;

    init();
}

// constructor for loading seismCoreAttributes from file
seismCoreAttributes::seismCoreAttributes(hid_t file_id)
{
    init();

    // stash the values of attributes_h5t, vls_type_c_id, and dim_h5t
    // before overwriting with values from file
    hid_t _attributes_t = attributes_t;
    hid_t _vls_t = vls_t;
//    hid_t _fls_t = fls_t;
    hid_t _dim3_t = dim3_t;

    // open the attribute, and read info into 'this'
    hid_t aapl_id = H5P_DEFAULT;
    hid_t lapl_id = H5P_DEFAULT;
    hid_t attr_id = H5Aopen_by_name( file_id, "/", "simulation_attributes", 
            aapl_id, lapl_id );
    assert(attr_id >= 0);
    assert( H5Aread(attr_id, attributes_t, this ) >= 0);

    // pop the stashed values
    attributes_t = _attributes_t;
    vls_t = _vls_t;
//    fls_t = _fls_t;
    dim3_t = _dim3_t;

    H5Aclose(attr_id);
}

seismCoreAttributes::~seismCoreAttributes()
{
    // close resources 
    H5Tclose(attributes_t);
    H5Tclose(vls_t); 
//    H5Tclose(fls_t); 
    H5Tclose(dim3_t); 
}
 
