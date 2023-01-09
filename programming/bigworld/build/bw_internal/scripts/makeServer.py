#!/usr/bin/env python

import os
import sys
import optparse
import platform
import build_common

# Scripts from the 'common' build script directory imported in build_common


def makeWithConfig( MF_CONFIG, command="" ):
	build_common.runCmd( "export MF_CONFIG=%s; make %s" % (MF_CONFIG, command) )


def cleanAllConfigsWithPrefix( configPrefix ):
	makeWithConfig( configPrefix, "clean" )

	if configPrefix == 'Debug' or configPrefix == 'Debug64' or \
		configPrefix == 'Hybrid' or configPrefix == 'Hybrid64':

		extraConfig = "%s_SingleThreaded" % configPrefix
		makeWithConfig( extraConfig, "clean" )

		extraConfig="%s_SystemPython" % configPrefix
		makeWithConfig( extraConfig, "clean" )


def build( mfRoot, directory, shouldClean, usedConfigs, doUnitTests ):

	origDir = os.getcwd()
	build_common.chdir( "%s/%s" % (mfRoot, directory) )

	# For every specified config
	for config in usedConfigs:

		# Clean up the config if required
		if shouldClean:
			cleanAllConfigsWithPrefix( config )

		else:
			# Otherwise do the build
			makeWithConfig( config )

			# And perform the unit tests
			if doUnitTests:
				if config.find( "64" ) != -1:
					makeWithConfig( config, "unit_tests" )

	build_common.chdir( origDir )	


def main():

	opt = optparse.OptionParser()
	opt.add_option( "-c", "--clean",
					dest = "clean", default = False,
					action = "store_true",
					help = "Clean the server build directories" )

	opt.add_option( "-b", "--build_configurations",
					dest = "build_configurations", default="all",
					help = "The build type [Hybrid|all|Debug64_GCOV]" )

	(options, args) = opt.parse_args()

	mfRoot = build_common.getMFRoot()

	# choose which configurations to build
	if options.build_configurations == "Hybrid":
		tempConfigs=["Hybrid"]

	elif options.build_configurations == "Debug64_GCOV":
		tempConfigs=["Debug64_GCOV"]

	elif options.build_configurations == "all":
		tempConfigs=["Hybrid", "Debug"]

	else:
		print "ERROR: Invalid build configuration specified: '%s'" % \
			options.build_configurations
		sys.exit( 1 )


	# Check the architecture to use with MF_CONFIG
	if platform.machine() == 'x86_64':
		architecture = '64'
	else:
		architecture = ''

	usedConfigs = []
	for config in tempConfigs:
		if architecture == '64' and config.find( "64" ) == -1:
			config = "%s%s" % (config, architecture)

		usedConfigs.append( config )

	print "usedConfigs %s " % usedConfigs

	build( mfRoot, "/bigworld/src/server",
			options.clean, usedConfigs, True )
	build( mfRoot, "/bigworld/src/tools/res_packer",
			options.clean, usedConfigs, False )


if __name__ == "__main__":
	sys.exit( main() )

# makeServer.py
