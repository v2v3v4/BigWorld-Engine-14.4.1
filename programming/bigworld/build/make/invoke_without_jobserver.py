#!/usr/bin/env python

"""
Script to attempt to re-invoke make without a job server.

This is nescessary due to make stipulating that in both recursive and
non-recursive invokations of $(MAKE), the active job server is communicated to
sub processes via the MAKEFLAGS variable.
"""

import optparse
import os
import subprocess
import sys


def main():

	opt = optparse.OptionParser()
	opt.add_option( "-m", "--make",
					default = "make",
					help = "The 'make' command to use (eg: $(MAKE))." )
	options, args = opt.parse_args()

	if not args:
		print "No make arguments provided to %s" % sys.argv[ 0 ]
		sys.exit( 1 )
	
	newEnvironment = dict( os.environ )

	# Find all the environment variables associated with make and remove them
	# so when we re-invoke make the jobserver is not re-used.
	keysToRemove = []

	for key, val in newEnvironment.iteritems():
		if key.startswith( "MAKE" ):
			keysToRemove.append( key )
		if key.startswith( "MFLAGS" ):
			keysToRemove.append( key )

	for key in keysToRemove:
		newEnvironment.pop( key )

	commandArgs = [ options.make ]
	commandArgs.extend( args )

	#print "Running command '%s'" % str( commandArgs )

	processObject = subprocess.Popen( commandArgs, env = newEnvironment,
		close_fds = True )
	return processObject.wait()


if __name__ == "__main__":
	try:
		sys.exit( main() )
	except KeyboardInterrupt:
		sys.exit( 1 )

# invoke_without_jobserver.py
