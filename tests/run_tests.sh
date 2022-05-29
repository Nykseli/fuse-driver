#/bin/bash

set -e

# Terminal colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

MOUNT_PATH=/tmp/fuse_test

function read_dir {
    mkdir -p $MOUNT_PATH/read_dir/nest1/nest2
    touch $MOUNT_PATH/read_dir/empty.txt
    echo "foo" > $MOUNT_PATH/read_dir/foo.txt
    echo "bar1" > $MOUNT_PATH/read_dir/nest1/bar.txt
    echo "zar12" > $MOUNT_PATH/read_dir/nest1/nest2/zar.txt
}

function unlink_setup {
    mkdir -p $MOUNT_PATH/unlink/{unlink,unlinkat,rmdir,no-rm}
    echo "foo" > $MOUNT_PATH/unlink/foo.txt
    echo "fooat" > $MOUNT_PATH/unlink/fooat.txt
    echo "bar" > $MOUNT_PATH/unlink/unlink/bar.txt
    echo "bar2" > $MOUNT_PATH/unlink/unlinkat/bar2.txt
    echo "zar" > $MOUNT_PATH/unlink/unlinkat/zar.txt
    echo "har" > $MOUNT_PATH/unlink/rmdir/har.txt
    echo "no" > $MOUNT_PATH/unlink/no-rm/bar.txt
}

function set_up {
    mkdir -p $MOUNT_PATH
    ./fuse_mount $MOUNT_PATH
    mkdir -p $MOUNT_PATH/test_dir
    read_dir
    unlink_setup
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

if [[ -z $1 ]]; then
    TEST_EXECS=`find tests -name "test_*" -executable -type f`
else
    TEST_EXECS=tests/$1.test
fi

for TEST in $TEST_EXECS
do
    run_test $TEST
done
