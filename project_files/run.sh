#!/bin/bash

# Check if the correct number of arguments is passed
if [ $# -lt 2 ] || [ $# -gt 2 ]; then
    echo "Script usage: $0 <N> <testfile>"
    exit 1
fi

N=$1
NP=$((N*N + (N*N*N)/2 + 1))
TESTFILE=$2
NUM_LIBS=$((N*N))

echo "Fixing the host_file in case it has invisible characters..."
sed -i 's/\r//' host_file

echo "Running with NP=$NP, NUM_LIBS=$NUM_LIBS"

# Run with the testfile0
mpirun -np $NP --hostfile host_file ./main $NUM_LIBS $TESTFILE

