#!/usr/bin/env python

import os
import sys
import optparse
import build_common


def buildRPM( mfRoot, directory, isIndieBuild ):

	origDir = os.getcwd()
	build_common.chdir( "%s/%s" % (mfRoot, directory) )

	if isIndieBuild:
		build_common.runCmd( "make rpm BIGWORLD_INDIE=1" )
	else:
		build_common.runCmd( "make rpm" )

	build_common.chdir( origDir )	


def main():
	opt = optparse.OptionParser()

	opt.add_option( "-i", "--indie",
					dest = "indie", default = False,
					action = "store_true",
					help = "Build the Indie version of the RPMS" )

	(options, args) = opt.parse_args()

	mfRoot = build_common.getMFRoot()
	buildRPM( mfRoot, "/bigworld/src/server", options.indie )


if __name__ == "__main__":
	sys.exit( main() )

# makeRPM.py
