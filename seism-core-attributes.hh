// seism-core-attributes.hh
#include "hdf5.h"

/*
typedef struct {
    char *name;
    unsigned int processor_dims[3];
    unsigned int chunk_dims[3];
    unsigned int domain_dims[3];
    unsigned int simulation_time;
    int collective_write;
    int precreate;
    int set_collective_metadata;
    int early_allocation;
    int never_fill;
} seism_core_attributes_t;
*/

class seismCoreAttributes 
{
    public:

        char *name;
        unsigned int processor_dims[3];
        unsigned int chunk_dims[3];
        unsigned int domain_dims[3];
        unsigned int simulation_time;
        int collective_write;
        int precreate;
        int set_collective_metadata;
        int early_allocation;
        int never_fill;

        seismCoreAttributes
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
        );
        void writeAttributesToFile(hid_t file_id);
} ;


/*/ implementation
seismCoreAttributes::seismCoreAttributes(
        char * attr_name,
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






/*
// turn this into constructor
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
        ) ;

// turn this into constructor, from hid_t for file
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
        ) ;

        */
