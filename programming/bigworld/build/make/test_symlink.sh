#!/bin/sh

# Tests whether the directory specified in args supports symlinks (eg. is not a
# windows mount)
# Exit code is 0 if we can.

if [ -z "$1" ]; then
	echo "$0: No directory specified"
	exit 1
fi

if [ ! -d "$1" ]; then
	echo "$0: invalid directory in argument 1: $1"
	exit 1
fi

testFileName="bw_symlink_testfile"
testSymlinkSource="$1/${testFileName}.tmp"
testSymlinkTarget="$1/${testFileName}.sym"

if touch ${testSymlinkSource} && \
		ln -s -T -f ${testSymlinkSource} ${testSymlinkTarget} > /dev/null 2>&1;
then
	exit 0
fi

echo "It appears you are building from a directory that doesn't support symlinks."
echo "Set BW_LOCAL_DIR to a local filesystem location in either your:"
echo "Makefile.user or command line, eg: 'BW_LOCAL_DIR=/home/user/bwtemp make'"
exit 1

# test_symlink.sh
