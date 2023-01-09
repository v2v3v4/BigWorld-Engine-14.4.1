#!/usr/bin/env python

# This file is used by the build system to determine the build revision

import os
import sys

import p4revision
import svnrevision


def main():
	args = sys.argv

	if len( args ) < 3:
		print "usage: %s input_file output_file" % sys.argv[0]
		return 1

	input_file = args[1]
	output_file = args[2]

	revisionNumber = 0
	try:
		revisionNumber = p4revision.loadRevisionInfo()
	except Exception:
		pass

	if not revisionNumber:
		try:
			revisionNumber = svnrevision.loadRevisionInfo()
		except Exception:
			pass

	if not revisionNumber:
		revisionNumber = -1

	try:
		f = open( input_file, "r" )
		d = os.path.dirname( output_file )
		if not os.path.exists( d ):
			os.makedirs( d )
		w = open( output_file, 'w' )
		generated_str = f.read()
		generated_str = generated_str.replace( '@REVISION_NUMBER@',
			str( revisionNumber ) )
		w.write( generated_str )
		f.close()
		w.close()
	except IOError:
		print "IOError: fail to write the output file %s" % output_file
		return 1

	return 0


if __name__ == "__main__":
	sys.exit( main() )

# build.py