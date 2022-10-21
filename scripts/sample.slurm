#!/bin/bash

#SBATCH --nodes=4
#SBATCH --ntasks=144
#SBATCH --time=00:20:00
#SBATCH --job-name seism-core_4_144
#SBATCH --output=%j-%x.out


mkdir -p ~/scratch/seism-core
cd ~/scratch/seism-core

export TIME_STEPS=10

ml gcc mpich hdf5 seism-core

mpirun seism-core << EOF
processor 4 4 4
chunk 180 128 128
domain 360 128 128
time ${TIME_STEPS}
collective_write
never_fill
DONE
EOF

