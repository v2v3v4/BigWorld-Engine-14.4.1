import os
import re
import stat
import xml.etree.ElementTree as ET

def bigworldVersion():
	path = os.path.join( os.getcwd(), "programming/bigworld/lib/cstdmf/bwversion.hpp" )
	
	if not os.path.exists( path ):
		raise Exception( "Could not load BW version information from '%s'" % path )

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

def replaceLineInFile( srcFile, destFile, origStr, replStr ):

	if os.access(srcFile, os.F_OK) and not os.access(srcFile, os.W_OK):
		os.chmod(srcFile, stat.S_IWUSR)
	sourceFile = open(srcFile, "r")
	sourceLines = sourceFile.readlines()
	sourceFile.close()

	os.unlink(destFile)
	destFile = open(destFile, "w")
	
	replace = False
	for line in sourceLines:
		# Check for any potential substitutions
		if re.search( origStr, line ):
			replace = True
			if replStr == None: 
				continue
			line = re.sub( origStr, replStr, line)
		destFile.write( line )

	destFile.close()
	return replace

def engineConfigXML( resPath ):	
	return os.path.normpath( os.path.join( resPath, "engine_config.xml" ) )