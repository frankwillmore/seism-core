# mpiexec -n 16 ./seism-core 

# run a parameter sweep over processor count, size, and shapes (equant, prolong, oblong, bladed) 

# fixed shape (equant) and vary chunk size

# fix size and vary shape

# fix shape and vary # of processors

# make three measurements for each
# for p in 
    mpiexec -n 8 ./seism-core << EOF
processor 2 2 2
chunk 16 16 16
domain 180 64 64
time 2
DONE
EOF
