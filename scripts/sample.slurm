#!/bin/bash

#SBATCH --nodes=4
#SBATCH --ntasks=144
#SBATCH --time=00:20:00
#SBATCH --job-name seism-core_4_144
#SBATCH --output=%j-%x.out

export SEISM_SCRATCH=~/scratch/seism-core

mkdir -p ${SEISM_SCRATCH}
cd ${SEISM_SCRATCH}
rm -f seism-test.h5

export TIME_STEPS=10

ml gcc mpich seism-core

# seism-core test script as a heredoc, starts from here:
mpirun seism-core << EOF
processor 3 3 16
chunk 180 128 128
domain 360 128 128
lfs_stripe_count 2
time ${TIME_STEPS}
collective_write
never_fill
DONE
EOF
# end of test script

# clean up
rm -f seism-test.h5
