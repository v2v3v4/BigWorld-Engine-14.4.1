#!/usr/bin/env python

import os
import shutil
import sys
import build_common

UNIT_TEST_DIR = os.path.normpath(
					os.path.join( build_common.getRootPath(), "../game/bin/unit_tests", "hybrid" )
				)

def cleanOldTests():
	print "* winMakeUnitTests.cleanOldTests() from", UNIT_TEST_DIR

	try:
		shutil.rmtree( UNIT_TEST_DIR )
		if not os.path.exists( UNIT_TEST_DIR ):
			os.makedirs( UNIT_TEST_DIR )
	except Exception, e:
		print "* Not removing test dir:", str( e )


def _runExe( fileName ):
	if fileName.find( ".exe" ) != -1:
		if fileName.find( ".manifest" ) == -1:
			print
			print "* Running unit test '%s'" % fileName
			build_common.runCmd( fileName )


def runUnitTests():
	build_common.list_dir( UNIT_TEST_DIR, _runExe )
	return True


if __name__ == "__main__":
	sys.exit( not main() )

# winMakeUnitTests.py
