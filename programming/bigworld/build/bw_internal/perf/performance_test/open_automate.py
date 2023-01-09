import os
import sys
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import time
import threading
import util

XML_LOCATION = "scripts/testing/openautomate"
TIMEOUT = 900

FLUSH_EXT_TO_INCLUDE = [
	"spt","mfm","jpg","raw","tga",
	"animationsettings","txt","xml","texformat",
	"dds","py","swf","fev","fxo","animation",
	"fxh","template","chunk","primitives",
	"visualsettings","vlo","fx","odata",
	"font","bmp","cdata","bsp2","pyc","png",
	"localsettings","prefab","ctree","fsb",
	"anca","gui","settings","stc","visual",
	"def","model","texanim","bin","processed",
	"bat","uwv","gif","deps","opt","options"
]


def _flushResCache( path ):
	def touch( path, sTypeList ):
		f = tempfile.TemporaryFile( mode='wb', bufsize=2000 )
		cmd = "for /R "+ path + " %I in ("+sTypeList+") do fsutil file setvaliddata \"%I\" %~zI"
		subprocess.call( cmd, shell=True, stdout=f, stderr=None )
		f.close()

	def touchAll( path ):
		typeDict = {}
		for parent, dirnames, filenames in os.walk( path ):
			if '.svn' in dirnames:
				dirnames.remove( '.svn' )
			for filename in filenames:
				fileExt = filename.split('.')[-1]
				if fileExt in FLUSH_EXT_TO_INCLUDE:
					typeDict[ "*." + filename.split('.')[-1] ] = 1

		if len( typeDict ):
			sList = ",".join( typeDict.keys() )
			touch( path, sList )

	touchAll( path )
	
#run program with timeout
class Command(object):
	def __init__(self, cmd):
		self.cmd = cmd
		self.process = ""#None
		self.subprocess_output = ""

	def run(self, timeout, program):
		def target():
			self.returncode = 0
			try:
				print self.cmd
				self.subprocess_output = subprocess.check_output( self.cmd, 
										stderr=subprocess.STDOUT, shell=True )
			except subprocess.CalledProcessError, e:
				self.subprocess_output = e.output
				self.returncode = e.returncode

		start_time = time.time()
		thread = threading.Thread(target=target)
		thread.start()

		thread.join(timeout)
		if thread.is_alive():
			subprocess.check_output("taskkill /im " + program + " /f")
			thread.join()
			end_time = time.time()
			timeToRun = end_time - start_time
			self.subprocess_output +=  "\nTIMEOUT ERROR. Total Time: " + str(timeToRun)
			self.returncode = 1
			
		return self.subprocess_output
		
		
def _exec( exePath, xmlPath, flags ):
	cmd = "%s -noConversion %s -openautomate INTERNAL_TEST %s" % (exePath, flags, xmlPath) 
	( path, exe_file ) = os.path.split( exePath )
	command = Command(cmd)
	subprocess_output = command.run(timeout=TIMEOUT, program=exe_file )
	res = command.returncode
	
	if res != 0:
		sys.stderr.write( "\ERROR EXECUTING:\n> " + cmd + "\n\n" )
		print subprocess_output
		raise util.BWTestingError( "The client did not cleanly shut down.\n" +
			subprocess_output )


def resolveXMLPath( scriptName, scriptIndex=None ):
	if scriptIndex is None:
		scriptIndex = ""
	return "%s/%s%s.xml" % (XML_LOCATION, scriptName, scriptIndex)


def resolveAbsoluteXMLPath( resPath, scriptName, scriptIndex=None ):
	relativePath = resolveXMLPath( scriptName, scriptIndex )
	return os.path.normpath( os.path.join( resPath, relativePath ) )

	
def checkScriptExists( resPath, scriptName, scriptIndex=None ):
	absXMLPath = resolveAbsoluteXMLPath( resPath, scriptName, scriptIndex )
	return os.path.exists( absXMLPath )
		
	
# Executes the given open automate script with the given scriptIndex.
# The scriptIndex is the number suffix on the XML file.
# Returns False if the command+scriptIndex wasn't found.

# This will run the script X times based on the runCount inside the
# the script's XML definition.
def runScript( exePath, flags, resPath, scriptName, scriptIndex=None, primer_run=False ):
	absXMLPath = resolveAbsoluteXMLPath( resPath, scriptName, scriptIndex )
	if not os.path.exists( absXMLPath ):
		return False
	
	if primer_run:
		totalRunCount = 1
	else:
		totalRunCount = int( util.parseXMLValue( absXMLPath, 'runCount', 1 ) )
	
	needFlushResCache = int( util.parseXMLValue( absXMLPath, 'flushResCache', 0 ) )
	
	for runCount in range(totalRunCount):
		xmlPath = resolveXMLPath( scriptName, scriptIndex )
		
		print "\t%s%s" % (scriptName, scriptIndex if scriptIndex is not None else ""),
		print ", ".join( collectBenchmarkNames( resPath, scriptName, scriptIndex ) ),
		if totalRunCount > 1:
			print "run %d/%d" % (runCount+1, totalRunCount)
		else:
			print
		
		if needFlushResCache:
			print "\t\tFS cache flush...",
			_flushResCache( resPath )
			print "done."
		
		_exec( exePath, xmlPath, flags )		
	
	return True

	
# This finds all the CMD_RUN_BENCHMARK commands inside the given 
# open automate script, and returns a list of their benchmark names.
# e.g. RunProfilerMinspec, RunProfilerHighlands
def collectBenchmarkNames( resPath, scriptName, scriptIndex ):
	xmlPath = resolveAbsoluteXMLPath( resPath, scriptName, scriptIndex )
	try:
		doc = ET.parse( xmlPath )
	except IOError:
		raise BWTestingError( "collectBenchmarkNames: Failed to parse XML: %s" % xmlPath )
	
	names = []	
	root = doc.getroot()
	commandNodes = root.findall( "command" )
	for commandNode in commandNodes:
		if commandNode.text.strip() == "CMD_RUN_BENCHMARK":
			benchmarkNode = commandNode.find( "benchmark" )
			names.append( benchmarkNode.text.strip() )
	
	return names

