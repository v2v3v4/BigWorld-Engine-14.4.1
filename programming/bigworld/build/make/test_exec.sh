#!/bin/sh

# Tests whether the directory specified in args supports executables (eg.
# mounted with noexec)
# Exit code is 0 if we can.

if [ -z "$1" ]; then
	echo "$0: No directory specified"
	exit 1
fi

if [ ! -d "$1" ]; then
	echo "$0: invalid directory in argument 1: $1"
	exit 1
fi

testExecFileName="$1/bw_executable_testfile"

if touch ${testExecFileName} && chmod +x ${testExecFileName} && \
		${testExecFileName} 2> /dev/null ;
then
	exit 0
fi

echo "It appears you are building from a directory that doesn't allow running executables."
echo "It was probably mounted with noexec. Set BW_LOCAL_DIR to a directory that allows "
echo "running executables in either your: Makefile.user or command line, eg: "
echo "'BW_LOCAL_DIR=/home/user/bwtemp make'"
exit 1

