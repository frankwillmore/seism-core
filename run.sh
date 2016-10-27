#!/bin/bash

# run a parameter sweep over processor count, size, and shapes (equant, prolong, oblong, bladed) 
# fixed shape (equant) and vary chunk size
# fix size and vary shape
# fix shape and vary # of processors

mpiexec -n 8 ./seism-core << EOF
processor 2 2 2
chunk 180 64 64
domain 180 64 64
time 2
DONE
EOF

mpiexec -n 16 ./seism-core << EOF
processor 2 2 4
chunk 180 128 128
domain 180 128 128
time 10
use_collective
DONE
EOF


