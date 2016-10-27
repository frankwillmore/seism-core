#!/bin/bash

mpiexec -n 16 ./seism-core << EOF
processor 2 2 4
chunk 360 128 128
domain 180 128 128
time 3
use_collective
DONE
EOF
