#!/bin/bash

set -e

#sudo perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations ./a.out

MOUNT_PATH=/tmp/fuse_test

PERF_PID=0

function set_up {
    make -B -j4 PERF=true
    mkdir -p $MOUNT_PATH
    perf record -e cpu-clock,faults ./fuse_mount -f $MOUNT_PATH &
    PERF_PID=$!
}

function clean_up {
    kill $PERF_PID
}

trap clean_up EXIT
set_up

for i in {1..100}
do
    ./example.sh $MOUNT_PATH
done
