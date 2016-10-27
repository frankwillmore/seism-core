#!/bin/bash

# run a parameter sweep over processor count, size, and shapes (equant, prolong, oblong, bladed) 
# fixed shape (equant) and vary chunk size
# fix size and vary shape
# fix shape and vary # of processors

mpiexec -n 16 ./seism-core << EOF
processor 4 2 2
chunk 16 16 16
domain 180 64 128
time 3
#use_collective
DONE
EOF

# elongate in one direction 
mpiexec -n 16 ./seism-core << EOF
processor 4 2 2
chunk 180 16 16
domain 180 64 128
time 3
DONE
EOF

# elongate in one direction 
mpiexec -n 16 ./seism-core << EOF
processor 4 2 2
chunk 360 16 16
domain 180 64 128
time 3
DONE
EOF

# scale chunk size isotropically 
mpiexec -n 16 ./seism-core << EOF
processor 4 2 2
chunk 32 32 32
domain 180 64 128
time 3
DONE
EOF

# scale chunk size isotropically 
mpiexec -n 16 ./seism-core << EOF
processor 4 2 2
chunk 64 64 64
domain 180 64 128
time 3
DONE
EOF

# scale chunk size isotropically 
mpiexec -n 16 ./seism-core << EOF
processor 4 2 2
chunk 180 64 128
domain 180 64 128
time 3
DONE
EOF


