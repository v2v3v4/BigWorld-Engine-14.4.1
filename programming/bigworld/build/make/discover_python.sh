#!/bin/sh

isKnownPython2_4() {

	# Check explicitly if we are a CentOS or RHEL machine.
	grep --quiet "CentOS" /etc/redhat-release
	isCentOS=$(( ! $? ))

	if [[ $isCentOS -ne 1 ]]; then
		grep --quiet "Red Hat" /etc/redhat-release
		isCentOS=$(( ! $? ))
	fi

	if [[ $isCentOS -eq 1 ]]; then
		grep --quiet "release [5]" /etc/redhat-release
		isSupported=$(( ! $? ))
	fi

	return $(( $isSupported ))
}


PYTHON_VERSION=2.4

PATH=$PATH:/usr/bin:/usr/local/bin
export PATH
PY_CONFIG=`which python-config 2>/dev/null`
ret=$?
if [ $ret == 0 ]; then
	$PY_CONFIG $1
else
	isKnownPython2_4
	isSupported=$?

	if [[ $isSupported -eq 1 ]]; then
		case "$1" in

		  --libs)
			echo "-lpthread -ldl -lutil -lm -lpython${PYTHON_VERSION}"
			;;

		  --includes)
			echo "-I/usr/include/python${PYTHON_VERSION} -I/usr/include/python${PYTHON_VERSION}"
			;;

		  --cflags)
			;;

		  --ldflags)
			echo "-lpthread -ldl -lutil -lm -lpython${PYTHON_VERSION}"
			;;
		esac
	else
		echo "ERROR: Unable to locate 'python-config' in PATH=$PATH"
		exit $ret
	fi
fi

