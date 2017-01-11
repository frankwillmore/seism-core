// seism-core-attributes.cc
#include <hdf5.h>
#include "seism-core-attributes.hh"
#include <cassert>
#include <iostream>

using namespace std;

void seismCoreAttributes::writeAttributesToFile(hid_t file_id)
{
    // create the inner array and character types
    hid_t vls_type_c_id = H5Tcopy(H5T_C_S1);
    H5Tset_size(vls_type_c_id, H5T_VARIABLE);
    hsize_t adims[] = {3};
    hid_t dim_h5t = H5Tarray_create(H5T_NATIVE_UINT, 1, adims);

    // create the compound type
    hid_t attributes_h5t = H5Tcreate(H5T_COMPOUND, sizeof(seismCoreAttributes)); 
    H5Tinsert(attributes_h5t, "name", HOFFSET(seismCoreAttributes, name), vls_type_c_id);
    H5Tinsert(attributes_h5t, "processor_dims", HOFFSET(seismCoreAttributes, processor_dims), dim_h5t);
    H5Tinsert(attributes_h5t, "chunk_dims", HOFFSET(seismCoreAttributes, chunk_dims), dim_h5t);
    H5Tinsert(attributes_h5t, "domain_dims", HOFFSET(seismCoreAttributes, domain_dims), dim_h5t);
    H5Tinsert(attributes_h5t, "simulation_time", HOFFSET(seismCoreAttributes, simulation_time), H5T_NATIVE_UINT);
    H5Tinsert(attributes_h5t, "collective_write", HOFFSET(seismCoreAttributes, collective_write), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "precreate", HOFFSET(seismCoreAttributes, precreate), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "set_collective_metadata", HOFFSET(seismCoreAttributes, set_collective_metadata), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "early_allocation", HOFFSET(seismCoreAttributes, early_allocation), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "never_fill", HOFFSET(seismCoreAttributes, never_fill), H5T_NATIVE_INT);

    // commit the compound type to HDF5 file
    assert(H5Tcommit(file_id, "attributes_t", attributes_h5t, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) >= 0 );
    
    // create scalar dataspace for attributes        
    hid_t space_id = H5Screate(H5S_SCALAR);
    hid_t acpl_id = H5P_DEFAULT;
    hid_t aapl_id = H5P_DEFAULT;

    // create the attribute
    hid_t attr_id = H5Acreate( file_id, "simulation_attributes", attributes_h5t, space_id, acpl_id, aapl_id );

    // crate a buffer, asign values and write attribute
//    char *attr_name = (char*)"my_attributes";
//    seismCoreAttributes attributes_buf(attr_name, processor_dims, chunk_dims, domain_dims, simulation_time, collective_write, precreate, set_collective_metadata, early_allocation, never_fill);
//    assert(H5Awrite(attr_id, attributes_h5t, &attributes_buf ) >= 0); 
    std::cout << "name:" << this->name << std::endl;
    std::cout << "processor_dims:" << this->processor_dims << std::endl;
    assert(H5Awrite(attr_id, attributes_h5t, this) >= 0); 

    // close resources
    H5Aclose(attr_id);
    H5Tclose(attributes_h5t);
    H5Sclose(space_id);
}


seismCoreAttributes::seismCoreAttributes
(
    char * _name,
    unsigned int *_processor_dims,
    unsigned int *_chunk_dims,
    unsigned int *_domain_dims,
    unsigned int _simulation_time,
    int _collective_write,
    int _precreate,
    int _set_collective_metadata,
    int _early_allocation,
    int _never_fill
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
    early_allocation = _early_allocation;
    never_fill = _never_fill;
}

/*
// All params passed by reference, except file which is already a reference
void recall_simulation_attributes(
        hid_t file,
        unsigned int **processor_dims,
        unsigned int **domain_dims,
        unsigned int **chunk_dims,
        unsigned int *simulation_time,
        int *collective_write,
        int *precreate,
        int *set_collective_metadata,
        int *early_allocation,
        int *never_fill
        ) 
{

} 

// turn this into constructor
seismCoreAttributes::seismCoreAttributes
(
    hid_t file,
    unsigned int *_processor_dims,
    unsigned int *_chunk_dims,
    unsigned int *_domain_dims,
    unsigned int _simulation_time,
    int _collective_write,
    int _precreate,
    int _set_collective_metadata,
    int _early_allocation,
    int _never_fill
)   
{
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
    early_allocation = _early_allocation;
    never_fill = _never_fill;
}

*/
