import os
import re
import xml.etree.ElementTree as ET
import constants
import stat

class BWTestingError( Exception ):
	pass
	
PACKAGE_ROOT = os.getcwd()

#
# Resolves the root of the "package" (historically known as "mf root").
def packageRoot():
	return PACKAGE_ROOT

#
# Determines what version of BigWorld is being used. 
# This is parsed from bwversion.hpp
def bigworldVersion():
	path = os.path.join( packageRoot(), "programming/bigworld/lib/cstdmf/bwversion.hpp" )
	
	if not os.path.exists( path ):
		raise BWTestingError( "Could not load BW version information from '%s'" % path )

	major = None
	minor = None
	patch = None
	
	fp = open( path )
	for line in fp:
		match = re.match( "#define *BW_VERSION_(\w+) *(\d+)", line )
		if match:
			if match.group( 1 ) == "MAJOR":
				major = int( match.group( 2 ) )

			elif match.group( 1 ) == "MINOR":
				minor = int( match.group( 2 ) )

			elif match.group( 1 ) == "PATCH":
				patch = int( match.group( 2 ) )
	
	return (major, minor, patch)


def resolveClientExecutable( buildConfig, exeType ):
	dir = os.path.join( packageRoot(), constants.EXE_DIRS[buildConfig] )
	fullPath = os.path.normpath( os.path.join( dir, constants.EXE_FILES[exeType] ) )
	if os.path.exists( fullPath ):
		return fullPath
	raise BWTestingError( "Client executable is not present." )
	
#
# 
def resolveDataFilePath( filename ):
	dataPath = os.path.join( os.path.dirname( __file__ ), "data" )
	return os.path.join( dataPath, filename )


def parseXMLValue( xmlFilePath, nodeName, defaultVal ):
	try:
		doc = ET.parse( xmlFilePath )
	except IOError:
		print "ERROR: error reading XML", xmlFilePath
		return defaultVal
		
	root = doc.getroot()
	node = root.find( nodeName )
	if node is None:
		return defaultVal
	
	return node.text


def replaceLineInFile( srcFile, destFile, origStr, replStr ):

	sourceFile = open(srcFile, "r")
	sourceLines = sourceFile.readlines()
	sourceFile.close()
	
	os.chmod( destFile, stat.S_IWRITE )
	os.unlink(destFile)
	destFile = open(destFile, "w")

	replace = False
	for line in sourceLines:
		# Check for any potential substitutions
		if re.search( origStr, line ):
			replace = True
			line = re.sub( origStr, replStr, line)
		destFile.write( line )

	destFile.close()
	return replace

#
# Reads all "benchmark:name:resultname:value" lines from the given file.
#
# Returns a dictionary of dictionaries. There can be any number of of values for
# each result name (e.g. the test ran multiple times).
# { "benchmarkName":
#		"resultName1": [valueX, valueY, valueZ, ...],
#		"resultName2": [valueX, valueY, valueZ, ...],
#		...
# }
def parseResultsFile( filename ):
	resultsDict = {}
	
	# Get result data generated by calcstats.
	resultFile = open( filename )
	for line in resultFile:
		if "benchmark:" not in line:
			continue
		
		data = line.split( ":" )
		benchmarkName = data[1]
		resultName = data[3]
		resultValue = data[5]
		
		if benchmarkName not in resultsDict:
			resultsDict[benchmarkName] = {}
		
		if resultName not in resultsDict[benchmarkName]:
			resultsDict[benchmarkName][resultName] = []
		
		resultsDict[benchmarkName][resultName].append( resultValue )
	
	return resultsDict

def engineConfigXML( resPath ):	
	return os.path.normpath( os.path.join( resPath, "engine_config.xml" ) )