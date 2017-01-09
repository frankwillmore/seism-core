// seism-core-attributes.hh

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


