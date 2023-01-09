#!/usr/bin/env python

import os
import sys
import time
import datetime
import optparse
import build_common
import project_manager

import ctypes
GetShortPathName = ctypes.windll.kernel32.GetShortPathNameW

def convertPathToShortPath( path ):
	bufSize = 260
	buf = ctypes.create_unicode_buffer( bufSize )
	retVal = GetShortPathName( unicode( path ), buf, bufSize )

	if retVal == 0:
		raise ValueError( "Unable to get the short path name" )

	return buf.value
	

def buildProject( root, projectIter, devEnvCom, shouldClean,
	shouldUseIncrediBuild ):

	print

	projectStart = datetime.datetime.fromtimestamp( time.time() )

	origDir = os.getcwd()

	for project, config in projectIter:
		projectDir = os.path.dirname( project )
		projectSln = os.path.basename( project )

		print
		print "* Building %s in config %s" % (projectSln, config)

		os.chdir( os.path.join( root, projectDir ) )

		if shouldUseIncrediBuild:
			tag = ''
			if config.find('|') == -1:
				tag = '|Win32'
			configCmd = '/cfg="%s%s"' % ( config, tag )
		else:
			configCmd = '"%s"' % config

		# The '@@' is used to substitute the /clean or /build as required
		baseCommand = '%s %s @@ %s' % (devEnvCom, projectSln, configCmd)

		if shouldClean:
			cmd = baseCommand.replace( "@@", "/clean" )

		else:
			cmd = baseCommand.replace( "@@", "/build" )

		build_common.runCmd( cmd )

		os.chdir( origDir )

	projectEnd = datetime.datetime.fromtimestamp( time.time() )
	print "* Total project build time for %s (%s) - %s" % \
		(projectSln, config, projectEnd-projectStart)


# Make a dumb assumption about where IncrediBuild is installed until
# we need to know otherwise.
def getIncrediBuildPath():

	potentialLocations = [
		"C:\\Program Files\\Xoreax\\IncrediBuild",
		"C:\\Program Files (x86)\\Xoreax\\IncrediBuild" ]
	for path in potentialLocations:
		if os.path.isdir( path ):
			return path

	return None


def build( vsVersion, devEnvComLong, buildType, shouldClean,
	shouldUseIncrediBuild, projectConfigPath ):

	if shouldUseIncrediBuild:
		pathToIncrediBuild = getIncrediBuildPath()
		if not pathToIncrediBuild:
			print "ERROR: Unable to locate IncrediBuild installation."
			return False
			
		devEnvComLong = "%s\\BuildConsole.exe" % pathToIncrediBuild

	# Convert the development environment to windows short path names as
	# os.popen complains about the spaces in C:\Program Files\...
	devEnvCom = convertPathToShortPath( devEnvComLong ).encode( "utf8" )
	
	print
	print "==== winMakeClient ===="
	print " Build Type    :", buildType
	print " Visual Studio :", vsVersion
	print " IncrediBuild  :", shouldUseIncrediBuild
	print " Dev Env       :", devEnvComLong
	print " Dev Env short :", devEnvCom
	print " Config Path   :", projectConfigPath
	print

	root = build_common.getRootPath()
	projectConfigPath = os.path.join( root, projectConfigPath )
	
	# Client projects
	projectList = project_manager.getProjects( vsVersion, buildType, projectConfigPath )
	# Build all projects
	for projectIter in projectList:
		buildProject( root, projectIter, devEnvCom, shouldClean,
						shouldUseIncrediBuild )

	return True


def main():
	opt = optparse.OptionParser()
	opt.add_option( "-c", "--clean",
					dest = "clean", default = False,
					action = "store_true",
					help = "Clean away previous build files" )

	opt.add_option( "-t", "--type",
					dest = "build_type", default="continuous",
					help = "The build type [continuous|nightly]" )

	opt.add_option( "-d", "--devEnvCom",
					dest = "devEnvCom", default = None,
					help = "The Visual Studio devenv.com file location" )

	opt.add_option( "-v", "--visual_studio",
					dest = "visual_studio", default = None,
					help = "Visual Studio version" )

	opt.add_option( "-i", "--incredibuild",
					dest = "incredibuild", default = False,
					action = "store_true",
					help = "Use incredibuild" )
	
	opt.add_option( "-b", "--buildconfig",
					dest = "config", default = None,
					help = "Path to the project config xml file" )
					
	(options, args) = opt.parse_args()

	try:
		build_common.validateVisualStudioVersion( options.visual_studio )
	except ValueError, ve:
		print "ERROR: Visual Studio version invalid. %s" % str( ve )
		return False

	if not options.incredibuild and options.devEnvCom == None:
		print "ERROR: Visual Studio devenv.com file must be specified."
		return False

	try:
		buildType = build_common.stringToBuildType( options.build_type )
	except ValueError, ve:
		print "ERROR: %s" % str( ve )
		return False
		
	if options.config == None:
		print "ERROR: Please specify the path to the project config xml file"
		return False

	return build( options.visual_studio, options.devEnvCom,
					options.build_type, options.clean, options.incredibuild, options.config )

	
if __name__ == "__main__":
	sys.exit( main() )

# winMakeClient.py
