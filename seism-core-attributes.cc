// seism-core-attributes.cc
#include <hdf5.h>
#include "seism-core-attributes.hh"
#include <cassert>

void record_simulation_attributes(
        hid_t file,
        unsigned int *processor_dims,
        unsigned int *domain_dims,
        unsigned int *chunk_dims,
        unsigned int simulation_time,
        int collective_write,
        int precreate,
        int set_collective_metadata,
        int early_allocation,
        int never_fill
        )   
{
    // create the inner array and character types
    hid_t vls_type_c_id = H5Tcopy(H5T_C_S1);
    H5Tset_size(vls_type_c_id, H5T_VARIABLE);
    hsize_t adims[] = {3};
    hid_t dim_h5t = H5Tarray_create(H5T_NATIVE_UINT, 1, adims);

    // create the compound type
    hid_t attributes_h5t = H5Tcreate(H5T_COMPOUND, sizeof(seism_core_attributes_t)); 
    H5Tinsert(attributes_h5t, "name", HOFFSET(seism_core_attributes_t, name), vls_type_c_id);
    H5Tinsert(attributes_h5t, "processor_dims", HOFFSET(seism_core_attributes_t, processor_dims), dim_h5t);
    H5Tinsert(attributes_h5t, "chunk_dims", HOFFSET(seism_core_attributes_t, chunk_dims), dim_h5t);
    H5Tinsert(attributes_h5t, "domain_dims", HOFFSET(seism_core_attributes_t, domain_dims), dim_h5t);
    H5Tinsert(attributes_h5t, "simulation_time", HOFFSET(seism_core_attributes_t, simulation_time), H5T_NATIVE_UINT);
    H5Tinsert(attributes_h5t, "collective_write", HOFFSET(seism_core_attributes_t, collective_write), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "precreate", HOFFSET(seism_core_attributes_t, precreate), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "set_collective_metadata", HOFFSET(seism_core_attributes_t, set_collective_metadata), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "early_allocation", HOFFSET(seism_core_attributes_t, early_allocation), H5T_NATIVE_INT);
    H5Tinsert(attributes_h5t, "never_fill", HOFFSET(seism_core_attributes_t, never_fill), H5T_NATIVE_INT);

    // commit the compound type to HDF5 file
    assert(H5Tcommit(file, "attributes_t", attributes_h5t, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) >= 0 );
    
    // create scalar dataspace for attributes        
    hid_t space_id = H5Screate(H5S_SCALAR);
    hid_t acpl_id = H5P_DEFAULT;
    hid_t aapl_id = H5P_DEFAULT;

    // create the attribute
    hid_t attr_id = H5Acreate( file, "simulation_attributes", attributes_h5t, space_id, acpl_id, aapl_id );

    // crate a buffer, asign values and write attribute
    seism_core_attributes_t attributes_buf;
    attributes_buf.name = (char*)"my_attributes";
    attributes_buf.chunk_dims[0] = chunk_dims[0];
    attributes_buf.chunk_dims[1] = chunk_dims[1];
    attributes_buf.chunk_dims[2] = chunk_dims[2];
    attributes_buf.processor_dims[0] = processor_dims[0];
    attributes_buf.processor_dims[1] = processor_dims[1];
    attributes_buf.processor_dims[2] = processor_dims[2];
    attributes_buf.domain_dims[0] = domain_dims[0];
    attributes_buf.domain_dims[1] = domain_dims[1];
    attributes_buf.domain_dims[2] = domain_dims[2];
    attributes_buf.simulation_time = simulation_time;
    attributes_buf.collective_write = collective_write;
    attributes_buf.precreate = precreate;
    attributes_buf.set_collective_metadata = set_collective_metadata;
    attributes_buf.early_allocation = early_allocation;
    attributes_buf.never_fill = never_fill;
    assert(H5Awrite(attr_id, attributes_h5t, &attributes_buf ) >= 0); 

    // close resources
    H5Aclose(attr_id);
    H5Tclose(attributes_h5t);
    H5Sclose(space_id);
}

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

