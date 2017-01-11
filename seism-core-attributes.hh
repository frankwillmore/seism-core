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

        seismCoreAttributes(hid_t file_id);

        void writeAttributesToFile(hid_t file_id);

        ~seismCoreAttributes()
        {
        };
};

