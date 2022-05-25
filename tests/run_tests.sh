#/bin/bash

set -e

# Terminal colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

MOUNT_PATH=/tmp/fuse_test

function set_up {
    mkdir -p $MOUNT_PATH
    ./fuse_mount $MOUNT_PATH
    mkdir -p $MOUNT_PATH/test_dir
    echo "foo" > $MOUNT_PATH/foo_file.txt
    cp $MOUNT_PATH/foo_file.txt $MOUNT_PATH/foocreat_file.txt
    echo "read only" > $MOUNT_PATH/norw_file.txt
    chmod a-rw $MOUNT_PATH/norw_file.txt
}

function clean_up {
    echo "cleaning up"
    fusermount -u $MOUNT_PATH || true
}

function run_test {
    printf "Running $1...\n"
    $1
    res=$?
    if [ $res -eq 0 ]; then
        printf "$1... ${GREEN} SUCCESS${NC}\n"
    else
        printf "$1... ${RED} FAILED${NC}\n"
    fi
}

trap clean_up EXIT
set_up

TEST_EXECS=`find tests -name "test_*" -executable -type f`
for TEST in $TEST_EXECS
do
    run_test $TEST
done
