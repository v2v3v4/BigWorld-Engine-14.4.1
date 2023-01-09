#!/usr/bin/env python

import os
import sys
import copy
import glob
import time
import pprint
import shutil
import stat
import datetime
import optparse
import build_common
import re

### Edit these for the builds you want ###
'''
Build configs which are whitelisted to build
CMake will automatically detect all of the BWConfiguration_*.cmake files,
but will only run the ones on this list. You can set it to build certain
projects in the continuous and nightly builds using the CONTINUOUS_BUILD and
NIGHTLY_BUILD lists below.

Projects can also be excluded from this list on the command line, use --help.

Project configs take the form of a dictionary.
name: "name of the project"
  ie. the project name in BWConfiguration_<project-name>.cmake
disabledArchitectures: [list of blacklisted architectures]
  eg. "Win32", "Win64"
disabledBuildConfigs: [list of blacklisted build configs]
  eg. "Debug", "Release"
'''
ALL_PROJECTS = \
[
	{
		'name':'assetprocessor',
		'disabledArchitectures':['Win64'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'client',
		'disabledArchitectures':['Win64'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'extras',
		'disabledArchitectures':['Win32'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'exporters',
		'disabledArchitectures':[],
		'disabledBuildConfigs':[]
	},
	{
		'name':'navgen',
		'disabledArchitectures':['Win32'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'tools',
		'disabledArchitectures':['Win32'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'bwentity',
		'disabledArchitectures':[''],
		'disabledBuildConfigs':[]
	}
]

BLOB_PROJECTS = \
[
	{
		'name':'assetprocessor',
		'disabledArchitectures':['Win64'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'client_blob',
		'disabledArchitectures':['Win64'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'extras',
		'disabledArchitectures':['Win32'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'exporters_blob',
		'disabledArchitectures':[],
		'disabledBuildConfigs':[]
	},
	{
		'name':'navgen',
		'disabledArchitectures':['Win32'],
		'disabledBuildConfigs':[]
	},
	{
		'name':'tools_blob',
		'disabledArchitectures':['Win32'],
		'disabledBuildConfigs':[]
	}
]

PC_LINT_DIR = "c:\\lint"
PC_LINT_EXCLUDED_PROJECTS = ["ALL_BUILD", "ZERO_CHECK", "RUN_TESTS", "libpng",
						"CppUnitLite2", "nedalloc", "open_automate", "libpython" ]
SRC_DIR = build_common.getRootPath() + "/" + "bigworld/lib"

### Edit these for the continuous builds you want ###
CONTINUOUS_BUILD = BLOB_PROJECTS

### Edit these for the nightly builds you want ###
NIGHTLY_BUILD = ALL_PROJECTS

## Edit these for the architectures you want to build ##
# Note: the architecture must exist in CMAKE_ARCHITECTURES
BUILD_ARCHITECTURES = ["Win32", "Win64"]

# Set to true if you want to test the script and not build
verbose = False

### Do not edit unless you are upgrading CMake ###
CMAKE_GENERATORS = [
	("Visual Studio 11", "vc110", "v110_xp"),
	("Visual Studio 10", "vc100", None),
	("Visual Studio 9 2008", "vc90", None),
]

### Do not edit unless you are upgrading CMake ###
CMAKE_ARCHITECTURES = [
	("Win32", "", "_win32"),
	("Win64", " Win64", "_win64"),
]

# We require a pre-release build of cmake for VS2012 XP
# support, so use our bundled version.
CMAKE_EXE = os.path.normpath( build_common.getRootPath() + '/bigworld/third_party/cmake-win32-x86/bin/cmake.exe' )

# Build directory where CMake puts its generated files
# and where object files go
# Relative to the build's root
# Pick something short to prevent long filename errors
BUILD_DIR_NAME = "../../out/"

### Do not edit ###
# No cleaning
CLEAN_NONE = 0
# Clean CMake cache before building
CLEAN_CMAKE_CACHE = (1<<0)
# Clean unit test dirs
CLEAN_CMAKE_UNIT_TESTS = (1<<1)
# Clean entire build directory
CLEAN_ALL = (1<<2)
# Run IncrediBuild with /clean
CLEAN_OBJECT_FILES = (1<<3)
# Exit before building
EXIT_AFTER_CLEAN = (1<<4)

# Delimiter for command line configs
DELIM = ","
EXPECTED_CONFIG = "name" + DELIM + "disabledBuildConfig"
def check_config( option, opt, value ):
	'''Checks build configs passed in from the command line'''
	try:
		# Match the build config strings
		groups = value.split( DELIM )
		if len( groups ) != 2:
			raise ValueError

		# Build the dictionary
		buildConfig = {}
		buildConfig["name"] = groups[0]
		buildConfig["disabledBuildConfig"] = groups[1]
		
		# Done
		value = buildConfig

	except ValueError:
		raise optparse.OptionValueError(
			"option %s: invalid value: '%s' (\"%s\" expected)" %
			( opt, value, EXPECTED_CONFIG ) )
	return value
	
EXPECTED_ARCH = "name" + DELIM + "disabledArchitecture"
def check_arch( option, opt, value ):
	'''Checks build architectures passed in from the command line'''
	try:
		# Match the build config strings
		groups = value.split( DELIM )
		if len( groups ) != 2:
			raise ValueError

		# Build the dictionary
		buildConfig = {}
		buildConfig["name"] = groups[0]
		buildConfig["disabledArchitecture"] = groups[1]

		# Check that architecture name is valid
		if buildConfig["disabledArchitecture"] not in BUILD_ARCHITECTURES:
			print( "Unknown architecture name '" +
				buildConfig["disabledArchitecture"] +
				"'" )
			raise ValueError
		
		# Done
		value = buildConfig

	except ValueError:
		raise optparse.OptionValueError(
			"option %s: invalid value: '%s' (\"%s\" expected)" %
			( opt, value, EXPECTED_ARCH ) )
	return value

class ExtendedOptions( optparse.Option ):
	'''Command line argument options for excluding build configs'''
	'''and architectures'''
	TYPES = optparse.Option.TYPES + ('config','arch')
	TYPE_CHECKER = copy.copy( optparse.Option.TYPE_CHECKER )
	TYPE_CHECKER['config'] = check_config
	TYPE_CHECKER['arch'] = check_arch

class ExtendedOptionParser( optparse.OptionParser ):
	'''Command line argument parser for excluding build configs'''
	'''and architectures'''
	def __init__( self, *args, **kwargs ):
		optparse.OptionParser.__init__(
			self, option_class=ExtendedOptions, *args, **kwargs )

# shutil.rmtree callback to remove read_only flag
def del_rw( file_function, filename, exc ):
	# Remove Read-only flag from Windows file
	os.chmod( filename, stat.S_IWRITE )
	file_function( filename )

def build( vsVersion,
	devEnvComLong,
	buildType,
	cleanType,
	generateOnly,
	shouldUseIncrediBuild,
	includedProjects,
	includedArchitectures,
	excludedConfigs,
	excludedArchitectures,
	excludedProjects,
	pcLint):
	'''Run the build'''

	if shouldUseIncrediBuild:
		pathToIncrediBuild = getIncrediBuildPath()
		if not pathToIncrediBuild:
			print "ERROR: Unable to locate IncrediBuild installation."
			return False
			
		devEnvComLong = "%s\\BuildConsole.exe" % pathToIncrediBuild

	# Convert the development environment to windows short path names as
	# os.popen complains about the spaces in C:\Program Files\...
	devEnvCom = convertPathToShortPath( devEnvComLong ).encode( "utf8" )

	root = build_common.getRootPath() + "/bigworld/"
	if verbose:
		print "root ", root

	projectPath = root + "build/"
	if verbose:
		print "projectPath ", projectPath

	BUILD_DIRECTORY = os.path.normpath( root + BUILD_DIR_NAME )

	# delete everything from the build directory for nightly builds
	if buildType == "nightly":
		shutil.rmtree( BUILD_DIRECTORY, True, del_rw )
		
	try:
		os.mkdir( BUILD_DIRECTORY )
	except OSError, e:
		pass
	os.chdir( BUILD_DIRECTORY )
	
	# Win64 builds are only work for 2010
	buildArchitectures = list( BUILD_ARCHITECTURES )
	if (vsVersion == "2005") or (vsVersion == "2008"):
		print
		print "%s does not support Win64 builds, disabling" % (vsVersion,)
		buildArchitectures.remove( "Win64" )

	print
	print "==== %s ====" % sys.argv[ 0 ]
	print " Build Type    :", buildType
	print " Visual Studio :", vsVersion
	print " Win64 enabled :", ("Win64" in buildArchitectures)
	print " IncrediBuild  :", shouldUseIncrediBuild
	print " Dev Env       :", devEnvComLong
	print " Dev Env short :", devEnvCom
	print " DirectX SDK   :", os.getenv( "DXSDK_DIR" )
	print " Build dir     :", BUILD_DIRECTORY
	print

	# Find .cmake targets. Client, tools etc
	detectedTargets = findCMakeTargets( projectPath )
	print
	print "CMake detected targets: "
	for t in detectedTargets:
		print t

	# VS version
	if vsVersion == "2012":
		generatorName, generatorToken, generatorToolset = CMAKE_GENERATORS[0]
	elif vsVersion == "2010":
		generatorName, generatorToken, generatorToolset = CMAKE_GENERATORS[1]
	elif vsVersion == "2008":
		generatorName, generatorToken, generatorToolset = CMAKE_GENERATORS[2]
	else:
		print "Error: No CMake script for Visual Studio ", vsVersion
		sys.exit( 1 )

	# Targets
	buildTargets = None
	if buildType == "continuous":
		buildTargets = list( CONTINUOUS_BUILD )
	elif buildType == "nightly":
		buildTargets = list( NIGHTLY_BUILD )
	else:
		print "No known target ", buildType
		sys.exit( 1 )

	# Include only projects listed on the command line
	if includedProjects is not None:
		for t in buildTargets[:]:
			if not t["name"] in includedProjects:
				buildTargets.remove( t )
			else:
				if verbose:
					print " * Including ", t["name"]

	# Exclude projects listed on the command line
	if excludedProjects is not None:
		for excludedTarget in excludedProjects:
			for t in buildTargets:
				if t["name"] == excludedTarget:
					if verbose:
						print " * Excluding", excludedTarget
					buildTargets.remove( t )
					break
					
	print
	print "Whitelisted targets: "
	for t in buildTargets:
		print t["name"]
	
	# Check list of targets is not empty
	if not buildTargets:
		raise RuntimeError( "Error: no targets to build" )

	# Add included builds from the command line options to the
	# included build configs
	if includedArchitectures is not None:
		for includedTarget in includedArchitectures:

			# Get matching build target
			buildTarget = None
			for t in buildTargets:
				if t["name"] == includedTarget["name"]:
					buildTarget = t
					break

			# No matching build target, go to next exclusion rule
			if buildTarget is None:
				continue
				
			enable = includedTarget["disabledArchitecture"]
			if enable in buildTarget["disabledArchitectures"]:
				buildTarget["disabledArchitectures"].remove(enable)

		
	# Add excluded builds from the command line options to the
	# disabled build configs
	if excludedArchitectures is not None:
		for excludedTarget in excludedArchitectures:

			# Get matching build target
			buildTarget = None
			for t in buildTargets:
				if t["name"] == excludedTarget["name"]:
					buildTarget = t
					break

			# No matching build target, go to next exclusion rule
			if buildTarget is None:
				continue

			disabled = excludedTarget["disabledArchitecture"]
			if disabled not in buildTarget["disabledArchitectures"]:
				buildTarget["disabledArchitectures"].append( disabled )

	if excludedConfigs is not None:
		for excludedTarget in excludedConfigs:
		
			# Get matching build target
			buildTarget = None
			for t in buildTargets:
				if t["name"] == excludedTarget["name"]:
					buildTarget = t
					break

			# No matching build target, go to next exclusion rule
			if buildTarget is None:
				continue

			disabled = excludedTarget["disabledBuildConfig"]
			if disabled not in buildTarget["disabledBuildConfigs"]:
				buildTarget["disabledBuildConfigs"].append( disabled )

	# Queue of things to compile
	buildQueue = []

	# Build target client, tools etc
	for buildTarget in buildTargets:
	
		if verbose:
			print "buildTarget", buildTarget

		targetName = buildTarget['name']

		# Check CMake can build it
		if not (targetName in detectedTargets):
			print "No CMake target for ", targetName
			sys.exit( 1 )
			
		if verbose:
			print "building target ", targetName
			
		# Have to run CMake separately for 32 and 64 bit
		for arch in buildArchitectures:
		
			# Check if architecture is disabled
			if arch in buildTarget['disabledArchitectures']:
				print
				print "* Skipping architecture", targetName, arch, "*"
				continue

			# Get CMake info about architecture
			if arch == "Win32":
				(architectureName,
				architectureToken,
				architectureOutputSuffix) = CMAKE_ARCHITECTURES[0]
			elif arch == "Win64":
				(architectureName,
				architectureToken,
				architectureOutputSuffix) = CMAKE_ARCHITECTURES[1]
			else:
				print "Error, unknown architecture ", arch
				sys.exit( 1 )

			if verbose:
				print "arch ", architectureName

			outputDir = "%s_%s%s" % (targetName,
				generatorToken,
				architectureOutputSuffix)
			outputDir = os.path.normpath(
				os.path.join( BUILD_DIRECTORY, outputDir ) )

			if verbose:
				print "outputDir ", outputDir

			# Clean entire CMake directory
			if (cleanType & CLEAN_ALL) != 0:
				fullClean( outputDir )

			# Otherwise cleaning parts of it
			else:
				# Clean CMake cache
				if (cleanType & CLEAN_CMAKE_CACHE) != 0:
					flushCMakeCache( outputDir )

				# Clean CMake unit test objects
				if (cleanType & CLEAN_CMAKE_UNIT_TESTS) != 0:
					flushCMakeUnitTests( outputDir )

			# Clean only - no building
			if (cleanType & EXIT_AFTER_CLEAN) != 0:
				# Skip build
				break

			# Create build directory
			if not os.path.isdir( outputDir ):
				os.makedirs( outputDir )

			targetDir = os.path.normpath( os.path.join(SRC_DIR,"..")) 
			cmd = r'%s %s -G"%s%s" -DBW_CMAKE_TARGET=%s' % \
				(CMAKE_EXE, targetDir, generatorName, architectureToken, targetName)
			if generatorToolset:
				cmd = cmd + r' -T %s' % generatorToolset

			print
			print ']', cmd

			result, availableBuildConfigs = runCMake( cmd, outputDir )
			if result != 0:
				sys.exit( result )

			if verbose:
				print "availableBuildConfigs", availableBuildConfigs

			# Initially bigworld CMake solutions were always named bigworld.sln
			slnName = os.path.join( outputDir, 'bigworld.sln' )
			targetSlnName = os.path.join( outputDir, '%s%s.sln' % (targetName, architectureOutputSuffix) )
			if os.path.exists( slnName ):
				print "deprecated: bigworld.sln found"
				if os.path.exists( targetSlnName ):
					os.unlink( targetSlnName )
				os.rename( slnName, targetSlnName )
			
			if verbose:
				print "targetSlnName ", targetSlnName

			# Build Debug, Release etc
			configs = availableBuildConfigs
			for config in configs:
				# Check the given build has not been blacklisted
				if config in buildTarget['disabledBuildConfigs']:
					print
					print "* Skipping build config", targetName, arch, config
				else:
					build = {
						'solution' : targetSlnName,
						'configuration' : config,
						'architecture' : architectureName
						}
					buildQueue.append( build )
			
			if pcLint:
				# This part must run after the out folder and all the solutions
				# have been created
				generateOnly = True
				if not os.path.exists(os.path.join( PC_LINT_DIR, "lint-nt.exe" )):
						print "Cannot find ", os.path.join( PC_LINT_DIR, "lint-nt.exe" )
						sys.exit( 1 )
				pc_lint_exe = os.path.join( PC_LINT_DIR, "lint-nt" )
				
				#copy pc lint option files to the pc lint directory
				if os.path.exists(os.path.join( PC_LINT_DIR, "pc_lint_options.lnt" )):
					os.remove(os.path.join( PC_LINT_DIR, "pc_lint_options.lnt" )) 
				shutil.copy(os.path.join( build_common.getRootPath(),"bigworld",
							"build", "bw_internal", "scripts", "pc_lint_options.lnt" ), 
							os.path.join( PC_LINT_DIR, "pc_lint_options.lnt" ))
				
				#copy pc lint std file to the pc lint directory
				pc_lint_std = os.path.join( PC_LINT_DIR, "pc_lint_std.lnt" )
				if os.path.exists(pc_lint_std):
					os.remove(pc_lint_std) 
				shutil.copy(os.path.join( build_common.getRootPath(),"bigworld", "build", "bw_internal", 
									"scripts", "pc_lint_std.lnt" ),pc_lint_std)	
				
				errors = False
				for root, dirs, files in os.walk(os.path.join(os.path.dirname(targetSlnName),"lib")	):
					for file in [f for f in files if f.endswith('.vcxproj')]:
						file_name = os.path.splitext(os.path.basename(file))[0]
						if file_name in PC_LINT_EXCLUDED_PROJECTS:
							continue
						#generate a list of files from solution
						cmd = pc_lint_exe + " +linebuf " + os.path.join( root, file ) \
								+ " >" + os.path.join( root, file_name + "_project.lnt" ) + " 2>NULL"
						os.system(cmd)
						#running pc lint
						#set the include file
						#add "-iDIRECTORY" to add include to pc lint
						# "-i" + os.path.join( root, "bigworld" )
						# "-i" + os.path.join( root, "out" ) + "\n"
						# "-i" + os.path.dirname(targetSlnName) + "\n"
						cmd = pc_lint_exe + " -i" + SRC_DIR + " -i" + PC_LINT_DIR +" " + pc_lint_std \
								+ " " + os.path.join( root, file_name + "_project.lnt" )\
								+ " >" + os.path.join( root, file_name + "_project_output.txt") + " 2>NULL"
						os.system(cmd)
						
						#print errors
						file = open(os.path.join( root, file_name + "_project_output.txt" ),'r')
						lines = ""
						for l in file:
							lines += l
						file.close()
						
						#ignore errors about pch.hpp not used
						lines = re.sub(r'--- Module:   [\S]+[\s]+\(C[\+\+]*\)[\s]+--- ' \
								+ 'Wrap-up for Module: [\S\s*]+[\s]+([\S]+\.lnt\([\d]+\)[\s]+ ' \
								+ ': Info 766: Header file \'[\S]+pch.hpp\'[\s]+)+' \
								+ 'not used in module \'[\S\s*]+\'', '', lines.rstrip())
						#ignore messages which are not errors
						lines = re.sub(r'--- Module:   [\S]+\.cpp[\s]+\(C\+\+\)','', lines.rstrip())
						lines = re.sub(r'--- Module:   [\S]+\.c[\s]+\(C*c*\)','', lines.rstrip())
						lines = re.sub(r'\s*\n','\n', lines.rstrip())
						
						if "Warning" in lines or "Error" in lines or "Info" in lines:
							errors = True

						if lines.strip() != '':
							print lines
						
				if errors:
					sys.exit( 1 )
	
	buildTimes = []
	if not generateOnly:
		for build in buildQueue:
			buildTime = buildProject( 
				build['solution'],
				build['configuration'],
				build['architecture'],
				devEnvCom,
				cleanType,
				shouldUseIncrediBuild )
			buildTimes.append( buildTime )

		print
		print "All build times:"
		for buildTime in buildTimes:
			print formatBuildTime( *buildTime )

	return buildTimes

def runCMake( command, outputDir ):

	# Run cmake and capture output
	import subprocess
	child = subprocess.Popen(
		command, cwd=outputDir, shell=False, stdout=subprocess.PIPE)
	output, errors = child.communicate()
	print output
	if errors is not None:
		print errors
		return 1, None
		
	# Process output to get a list of available build configurations
	availableBuildConfigs = []
	try:
		substr = output

		# Grab the line:
		# -- Build configurations: Debug;Release
		buildConfigString = "-- Build configurations: "
		startSubstrIndex = substr.index( buildConfigString )
		endSubstrIndex = substr.index( "\n", startSubstrIndex )
		substr = substr[startSubstrIndex:endSubstrIndex]

		# Remove
		# "-- Build configurations: "
		startSubstrIndex = substr.index( ":" ) + 1
		endSubstrIndex = len( substr )
		substr = substr[startSubstrIndex:endSubstrIndex]

		# Grab configs
		configs = substr.split( ";" )
		
		# Strip whitespace from configs
		for config in configs:
			config = config.strip()
			availableBuildConfigs.append( config )

	except ValueError, e:
		print "Could not detect build configurations"
		return 1, None

	return 0, availableBuildConfigs
	
def buildProject( targetSlnName,
	config,
	arch,
	devEnvCom,
	cleanType,
	shouldUseIncrediBuild ):

	root = targetSlnName

	print

	projectStart = datetime.datetime.fromtimestamp( time.time() )

	origDir = os.getcwd()

	project = targetSlnName
	projectDir = os.path.dirname( project )
	projectSln = os.path.basename( project )
	
	if verbose:
		print "projectDir ", projectDir
		print "projectSln ", projectSln

	print
	print "* Building %s in config %s" % (projectSln, config)

	os.chdir( os.path.join( root, projectDir ) )
	
	# If arch matches "Win64"
	# Change "Win64" to "x64" (because Visual Studio calls it "x64")
	# Otherwise, use arch
	vcArch = {"Win64":"x64"}.get( arch, arch )

	if shouldUseIncrediBuild:
		configCmd = '/cfg="%s|%s"' % ( config, vcArch )
	else:
		configCmd = '"%s"' % config

	# The '@@' is used to substitute the /clean or /build as required
	baseCommand = '%s %s @@ %s' % (devEnvCom, projectSln, configCmd)

	if (cleanType & CLEAN_OBJECT_FILES) != 0:
		cmd = baseCommand.replace( "@@", "/clean" )

	else:
		cmd = baseCommand.replace( "@@", "/build" )

	build_common.runCmd( cmd )

	os.chdir( origDir )

	projectEnd = datetime.datetime.fromtimestamp( time.time() )
	buildTime = ( projectEnd-projectStart, projectSln, config )
	print formatBuildTime( *buildTime )
	return buildTime

SKIP_CMAKE_FLUSH = False
CMAKE_DIR_TO_FLUSH = "CMakeFiles"
CMAKE_CACHE_FILE_NAME = "CMakeCache.txt"
CMAKE_FILES_TO_FLUSH = [ "cmake", "vcproj", "vcxproj", "sln" ]
def flushCMakeCache( path ):
	'''
	Remove the files generated by CMake,
	but leave any object files intact.
	'''
	if SKIP_CMAKE_FLUSH:
		print
		print "* Skipping CMake flush"
		return False

	if not os.path.exists( path ):
		print
		print "* Skipping CMake flush: path not found", path
		return False

	print
	print "* Flushing CMake cache"
	if verbose:
		print "walking", path
	walkPaths = os.walk( path )

	for (dirpath, dirnames, filenames) in walkPaths:
		
		# Directories to remove
		if CMAKE_DIR_TO_FLUSH in dirnames:
			if verbose:
				print "Removing directory tree", \
					os.path.join( dirpath, CMAKE_DIR_TO_FLUSH )
			shutil.rmtree( os.path.join( dirpath, CMAKE_DIR_TO_FLUSH ), onerror=del_rw )

		# Files to remove
		for filename in filenames:

			# Cache file
			if filename == CMAKE_CACHE_FILE_NAME:
				if verbose:
					print "Removing file", os.path.join( dirpath, filename )
				os.remove( os.path.join( dirpath, filename ) )
				
			# Files matching extensions
			else:
				fileExt = filename.split('.')[-1]
				if fileExt in CMAKE_FILES_TO_FLUSH:
					if verbose:
						print "Removing file", os.path.join( dirpath, filename )
					os.remove( os.path.join( dirpath, filename ) )
	return True

def flushCMakeUnitTests( path ):

	print
	print "* Flushing CMake unit test dirs"

	for (dirpath, dirnames, filenames) in os.walk( path ):
		if "unit_test.dir" in dirpath:
			if verbose:
				print "Removing", dirpath
			shutil.rmtree( os.path.normpath( dirpath ), onerror=del_rw )

	return True

def fullClean( outputDir ):
	'''
	Remove build directory.
	CMake generated files and object files.
	'''
	print
	print "* Deleting directory", outputDir
	if os.path.exists( outputDir ):
		shutil.rmtree( outputDir, onerror=del_rw )

def findCMakeTargets( path ):
	def _stripName( n ):
		# Glob uses backslash?
		stripPath = path + "cmake\BWConfiguration_"
		return n.replace( stripPath, "" ).replace( ".cmake", "" )
	return [ _stripName(x) for x in glob.glob(
		path + "cmake/BWConfiguration_*.cmake" ) ]
	
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
	
def convertPathToShortPath( path ):
	import ctypes
	GetShortPathName = ctypes.windll.kernel32.GetShortPathNameW

	bufSize = 260
	buf = ctypes.create_unicode_buffer( bufSize )
	retVal = GetShortPathName( unicode( path ), buf, bufSize )

	if retVal == 0:
		raise ValueError( "Unable to get the short path name" )

	return buf.value

def parseOptions():

	# Build the option parser
	opt = ExtendedOptionParser()
	opt.add_option( "-c", "--clean",
					dest = "clean", default = False,
					action = "store_true",
					help = "Clean away the build directory then exit" )

	opt.add_option( "-f", "--flush",
					dest = "flush", default = False,
					action = "store_true",
					help = "Flush away the previous CMake files then exit" )
					
	opt.add_option( "-e", "--codeCoverage",
					dest = "codeCoverage", default = False,
					action = "store_true",
					help = "Run code coverage on the debug unit tests" )
					
	opt.add_option( "-p", "--pcLint",
					dest = "pcLint", default = False,
					action = "store_true",
					help = "Run Pc Lint on the source code then exit" )

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

	opt.add_option( "-g", "--generate-only",
					dest = "generate_only", default = False,
					action = "store_true",
					help = "Perform CMake project generation and exit" )

	opt.add_option( "--changelist",
					dest = "changelist", default = 0,
					help = "Specify p4 changelist number" )
					
	opt.add_option( "--submit-stats-mysql",
					dest = "mysql", default = False,
					action = "store_true",
					help = "Store build times in mysql" )
					
	opt.add_option( "--submit-stats-graphite",
					dest = "graphite", default = False,
					action = "store_true",
					help = "Store build times in graphite" )
					
	opt.add_option( "--disable_unit_test",
					dest = "disableUnitTest", default = False,
					action = "store_true",
					help = "Skip the unit tests" )

	EXCLUDE_CONFIG_NAME = "--exclude-config"
	EXCLUDE_CONFIG_HELP = (
		"Project config to exclude as a '%s' separated list, eg. %s %s" %
		(DELIM, EXCLUDE_CONFIG_NAME, EXPECTED_CONFIG)
	)
	opt.add_option( EXCLUDE_CONFIG_NAME,
					dest = "excludedConfigs",
					action = "append",
					type = "config",
					help = EXCLUDE_CONFIG_HELP )

	EXCLUDE_ARCH_NAME = "--exclude-arch"
	EXCLUDE_ARCH_HELP = (
		"Project architecture to exclude as a '%s' separated list, eg. %s %s" %
		(DELIM, EXCLUDE_ARCH_NAME, EXPECTED_ARCH)
	)
	opt.add_option( EXCLUDE_ARCH_NAME,
					dest = "excludedArchitectures",
					action = "append",
					type = "arch",
					help = EXCLUDE_ARCH_HELP )
					
	EXCLUDE_PROJ_NAME = "--exclude-proj"
	EXCLUDE_PROJ_HELP = (
		"Project to exclude eg. %s 'project-name'" % (EXCLUDE_PROJ_NAME,)
	)
	opt.add_option( EXCLUDE_PROJ_NAME,
					dest = "excludedProjects",
					action = "append",
					help = EXCLUDE_PROJ_HELP )

	INCLUDE_PROJ_NAME = "--include-proj"
	INCLUDE_PROJ_HELP = (
		"White list a project to build e.g. %s 'project-name'" % (INCLUDE_PROJ_NAME)
	)
	opt.add_option( INCLUDE_PROJ_NAME,
					dest = "includedProjects",
					action = "append",
					help = INCLUDE_PROJ_HELP )
					
	INCLUDE_ARCH_NAME = "--include-arch"
	INCLUDE_ARCH_HELP = (
		"Project architecture to include as a '%s' separated list, eg. %s %s" %
		(DELIM, INCLUDE_ARCH_NAME, EXPECTED_ARCH)
	)
	opt.add_option( INCLUDE_ARCH_NAME,
					dest = "includedArchitectures",
					action = "append",
					type = "arch",
					help = INCLUDE_ARCH_HELP )
					
	# Parse the options
	(options, args) = opt.parse_args()

	# Validate the options
	try:
		build_common.validateVisualStudioVersion( options.visual_studio )
	except ValueError, ve:
		print "ERROR: Visual Studio version invalid. %s" % str( ve )
		return False

	try:
		buildType = build_common.stringToBuildType( options.build_type )
	except ValueError, ve:
		print "ERROR: %s" % str( ve )
		return False
		
	# If no dev env was specified, lookup what we expect to find
	if options.devEnvCom == None:
		vsVerMap = { "2005": "80",
					"2008": "90",
					"2010": "100",
					"2012" : "110" }
		vsToolsEnvVar = "VS%sCOMNTOOLS" % vsVerMap[ options.visual_studio ]
		vsTools = os.getenv( vsToolsEnvVar )
		if not vsTools:
			print "ERROR: Unable to discover Visual Studio using env '%s'" % \
				vsToolsEnvVar
			print "Try specifying the dev env with --devenv"

		vsToolsDir = os.path.abspath(
				os.path.join( vsTools, os.path.join( "..", ".." ) ) )
		options.devEnvCom = os.path.join(
							vsToolsDir, "Common7", "IDE", "devenv.com" )

	if not options.incredibuild and options.devEnvCom == None:
		print "ERROR: Visual Studio devenv.com file must be specified."
		return False

	return (options, args)

def formatBuildTime( duration, name, config ):
	return "* %s - %s (%s)" % \
		(duration, name, config )

def main():
	(options, args) = parseOptions()
	if options.clean:
		clean( options, args )
	else:
		run( options, args )

def run( options, args ):
	return build( options.visual_studio,
		options.devEnvCom,
		options.build_type,
		CLEAN_NONE,
		options.generate_only,
		options.incredibuild,
		options.includedProjects,
		options.includedArchitectures,
		options.excludedConfigs,
		options.excludedArchitectures,
		options.excludedProjects,
		options.pcLint)

def flush( options, args ):
	build( options.visual_studio,
		options.devEnvCom,
		options.build_type,
		(CLEAN_CMAKE_CACHE | EXIT_AFTER_CLEAN),
		options.generate_only,
		options.incredibuild,
		options.includedProjects,
		options.includedArchitectures,
		options.excludedConfigs,
		options.excludedArchitectures,
		options.excludedProjects,
		options.pcLint)

def clean( options, args ):
	build( options.visual_studio,
		options.devEnvCom,
		options.build_type,
		(CLEAN_ALL | EXIT_AFTER_CLEAN),
		options.generate_only,
		options.incredibuild,
		options.includedProjects,
		options.includedArchitectures,
		options.excludedConfigs,
		options.excludedArchitectures,
		options.excludedProjects,
		options.pcLint)
		
def pcLint( options, args ):
	build( options.visual_studio,
		options.devEnvCom,
		options.build_type,
		CLEAN_NONE,
		options.generate_only,
		options.incredibuild,
		options.includedProjects,
		options.includedArchitectures,
		options.excludedConfigs,
		options.excludedArchitectures,
		options.excludedProjects,
		options.pcLint)
		
if __name__ == "__main__":
	sys.exit( main() )

# winCMakeClient.py
