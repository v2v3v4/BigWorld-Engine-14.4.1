#!/usr/bin/env python

import os
import re
import sys
import platform
import subprocess

BUILD_TYPE_CONTINUOUS = 1
BUILD_TYPE_FULL = 2
BUILD_TYPE_NIGHTLY = BUILD_TYPE_FULL
ROOT_PATH = "../../../../.."

def run( cmd, returnLines=False ):
	outputs = []
	exitStatusOk = False
	pipe = os.popen( "%s 2>&1" % cmd )
	line = pipe.readline()
	try:
		while line:
			print( line.rstrip() )
			if returnLines == True:
				outputs.append( line )
			line = pipe.readline()
		exitStatusOk = ( pipe.close() == None )
	except IOError, e:
		print "Error while terminating build script pipe."
		print str( e )

	return ( exitStatusOk, outputs )


#Special run procedure using the subprocess (better with command spaces)
def subprocessRun( args ):
	print "running %s " % ( args )
	outputs = []
	process = subprocess.Popen( args, stderr=subprocess.PIPE, stdout=subprocess.PIPE )
	pollResult = process.poll()
	while pollResult == None:
		( stdoutdata, stderrdata ) = process.communicate()
		sys.stdout.write( stdoutdata )
		sys.stderr.write( stderrdata )
		pollResult = process.poll()
	print "here %s " % pollResult
	return pollResult == 0

def fail():
		print
		print( "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" )
		print( "!!! Aborting due to previous errors !!!" )
		sys.exit( 1 )
		
def list_dir( dir, action ):
	""" This function runs an action on all files in a specific path """
	if not os.path.isdir( dir ):
		print "%s is not a directory" % ( dir )
		return
	for file in os.listdir( dir ):
		path = os.path.join( dir, file )
		if os.path.isdir( path ):
			list_dir( path, action )
		else:
			#run the action
			action( path )
		
def isContinuousBuild( buildType ):
	return buildType == BUILD_TYPE_CONTINUOUS


def stringToBuildType( buildTypeString ):
	if buildTypeString in [ "full", "nightly" ]:
		return BUILD_TYPE_FULL
	elif buildTypeString == "continuous":
		return BUILD_TYPE_CONTINUOUS
	else:
		raise ValueError( "Unknown build type '%s'" % buildTypeString )


def validateVisualStudioVersion( vsVersion ):
	if vsVersion not in [ "2005", "2008", "2010", "2012" ]:
		raise ValueError( "Expected 2005, 2008, 2010, or 2012" )


#use to force running even if DEBUG_COMMANDS=1
forceRunning = False

def isWindows():
	if platform.platform().find( "Windows" ) != -1:
		return True

	return False


# Configure the common build script directory
if isWindows():
	sys.path.append( os.path.normpath( "c:/scripts/common" ) )

else:
	if os.environ.has_key( "USER" ) and \
		os.path.exists( "/home/%s/myscripts/common" % os.environ[ "USER" ] ):

		userName = os.environ[ "USER" ]
	else:
		userName = "hudson"


	addedPath= "/home/%s/myscripts/common" % userName
	print "Appending to PYTHON_PATH: '%s'" % addedPath
	sys.path.append(addedPath)

def chdir( directory ):
	print "build_common.chdir: changing directory to '%s'" % directory

	if not os.environ.has_key( "DEBUG_COMMANDS" ) or \
		not os.environ[ "DEBUG_COMMANDS" ] == "1":

		os.chdir( directory )


def runCmd( cmd, useSpecial=False, allowFailure=False ):
	ret = True
	outputs = []
	usedCmd = cmd

	print "* Executing build_common.runCmd( '%s' )" % usedCmd

	if not os.environ.has_key( "DEBUG_COMMANDS" ) or \
		not os.environ[ "DEBUG_COMMANDS" ] == "1" or \
		forceRunning:

		if useSpecial:
			ret = subprocessRun( usedCmd )
			outputs = []
		else:
			ret, outputs = run( usedCmd, True )

		if not ret and not allowFailure:
			print "ERROR: build_common.runCmd: failed '%s'" % usedCmd
			fail()

	return (ret, outputs)

def revertFile( file ):
	revertCommand = "svn revert \"%s\"" % file
	runCmd( revertCommand )


def replaceLineInFile( srcFile, destFile, origStr, replStr ):

	sourceFile = open(srcFile, "r")
	sourceLines = sourceFile.readlines()
	sourceFile.close()

	os.unlink(destFile)
	destFile = open(destFile, "w")

	for line in sourceLines:
		# Check for any potential substitutions
		if re.search( origStr, line ):
			line = re.sub( origStr, replStr, line)
		destFile.write( line )

	destFile.close()

def getRootPath():
	appdir = os.path.dirname( os.path.abspath( __file__ ) )
	
	appdir += ROOT_PATH

	root = os.path.abspath( appdir )

	if isWindows():
		root = root.replace("\\", "/")
		
	return root
# build_common.py
