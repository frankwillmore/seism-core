#!/bin/bash

# run a parameter sweep over processor count, size, and shapes (equant, prolong, oblong, bladed) 
# fixed shape (equant) and vary chunk size
# fix size and vary shape
# fix shape and vary # of processors

mpiexec -n 16 ./seism-core << EOF
processor 2 2 4
chunk 16 16 16
domain 180 64 128
time 10
DONE
EOF

mpiexec -n 16 ./seism-core << EOF
processor 2 2 4
chunk 180 64 128
domain 180 64 128
time 10
use_collective
DONE
EOF

mpiexec -n 16 ./seism-core << EOF
processor 2 2 4
chunk 180 64 128
domain 180 64 128
time 10
DONE
EOF
