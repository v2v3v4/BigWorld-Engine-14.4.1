import os
import shutil
import util
import reporter
import open_automate

from database import LoadTimeResult, LoadTimeReferenceValue
from constants import *
from util import BWTestingError

TEST_OUTPUT_FILE = "load_timer.txt"
OO_LOADTIME_TEST = "loadtimeTest"

REFERENCE_CALCULATION_RUN_COUNT = 2

spaceMapping = {'spaces/08_ruinberg' : 'RunProfilerLoadtimeRuinberg', 
		'spaces/highlands' : 'RunProfilerLoadtimeHighlands', 
		'spaces/minspec' : 'RunProfilerLoadtimeMinspec', 
		'spaces/arctic' : 'RunProfilerLoadtimeArctic'};

def _resetTestOutput( resPath ):
	try:
		fullName = os.path.join( resPath, TEST_OUTPUT_FILE )
		os.unlink( fullName )
	except WindowsError:
		pass

def _fpsResultInMS( resultValues ):
	fpsResults = [ float(x) for x in resultValues["elapsedTime"] ]
	avg = sum( fpsResults ) / len ( fpsResults )
	return avg
	
def _calculateReferenceValue( buildConfig, exeType, flags, branchName, exePath, resPath, 
					ooScript, scriptIndex, dbSession, benchmarkTestName ):
	# We don't want to clobber any test results that we have so far. Backup
	# the test output file, then reset for a clean reference test.
	fullLoadTimeOutputFile = os.path.join( resPath, TEST_OUTPUT_FILE )

	if os.path.exists( fullLoadTimeOutputFile+".bak" ):
		os.unlink( fullLoadTimeOutputFile+".bak" )
	if os.path.exists( fullLoadTimeOutputFile ):
		os.rename( fullLoadTimeOutputFile, fullLoadTimeOutputFile+".bak" )
			
	# Run the test (a few times to get an average)
	for x in range( REFERENCE_CALCULATION_RUN_COUNT ):
		open_automate.runScript( exePath, flags, resPath, ooScript, scriptIndex )
		
	# Parse results
	testResults = util.parseResultsFile( fullLoadTimeOutputFile )
	flag = False

	for benchmarkName, resultValues in testResults.iteritems():
		if spaceMapping[benchmarkName] == benchmarkTestName:
			flag = True	

			calculatedValue = _fpsResultInMS( resultValues )
			print "\tReference for %s:%s (%s %s) is now %.2fms" % \
				(branchName, spaceMapping[benchmarkName], buildConfig, exeType, calculatedValue)
			LoadTimeReferenceValue.set( dbSession, buildConfig, exeType, branchName, spaceMapping[benchmarkName], calculatedValue )
			
	if not flag:
		raise BWTestingError(
				"Failed to calculate reference value for %s:%s (%s %s).	reference value was nod found in %s" % \
				(benchmarkTestName, branchName, buildConfig, exeType, TEST_OUTPUT_FILE)
			)
	# Double check everything got set (sanity check)
	benchmarkNames = open_automate.collectBenchmarkNames( resPath, OO_LOADTIME_TEST, scriptIndex )
	for benchmarkName in benchmarkNames:
		if LoadTimeReferenceValue.get( dbSession, buildConfig, exeType, branchName, benchmarkName ) is None:
			raise BWTestingError(
					"Failed to calculate reference value for %s:%s (%s %s)." % \
					(branchName, benchmarkName, buildConfig, exeType)
				)
				
	if os.path.exists( fullLoadTimeOutputFile ):
		os.unlink( fullLoadTimeOutputFile )
	if os.path.exists( fullLoadTimeOutputFile+".bak" ):
		os.rename( fullLoadTimeOutputFile+".bak", fullLoadTimeOutputFile )

		
def _processResults( resPath, buildConfig, exeType, branchName, dbSession, changelist ):
	fullLoadTimeOutputFile = os.path.join( resPath, TEST_OUTPUT_FILE )
	testResults = util.parseResultsFile( fullLoadTimeOutputFile )
	
	testReport = ""
	
	# Build the rest of the report
	testReport += "Complete report:\n"
	testReport += "____ \n"

	for benchmarkName, resultValues in testResults.iteritems():
		testReport += "\n"
		if benchmarkName in testResults:			
			allValues = [ float(x) for x in resultValues["elapsedTime"] ]
			avg = sum( allValues ) / len ( allValues )
			
			LoadTimeResult.addResult( dbSession, buildConfig, exeType, branchName, benchmarkName, avg, changelist )
			
			testReport += "Branch: %s, Space: %s, Averge load time: %.4fsec (%d runs)\n" % \
							(branchName, benchmarkName, avg, len(allValues))
		else:
			testReport += "Data Missing\n"

	print testReport
	
	tag = "Load time test (%s %s)" % (buildConfig, exeType)
	
	return reporter.Report( True, tag, testReport, branchName )
	
	

def run( buildConfig, exeType, dbSession, reportHolder, branchTag, testName, compiled_space, changelist, primer_run, flags):
	global OO_LOADTIME_TEST
	if ( testName != "" ):
		OO_LOADTIME_TEST = testName
		
	exePath = util.resolveClientExecutable( buildConfig, exeType )
	resPath = os.path.normpath( os.path.join( util.packageRoot(), GAME_RESOURCE_PATH ) )

	print "test type: loadtime"
	print "build configuration:", buildConfig
	print "executable type:", exeType
	print "executable path:", exePath
	print "resource path:", resPath
	
	if branchTag:
		print "branch tag:", branchTag
	
	bwversion = util.bigworldVersion()
	
	engineXMLPath = util.engineConfigXML( resPath )
		
	branchName = "bw_%(versionMajor)d_%(versionMinor)d_full"
	if compiled_space :
		branchName = "bw_%(versionMajor)d_%(versionMinor)d_compiled_space"

	branchName = branchName % \
	{
		"versionMajor":bwversion[0],
		"versionMinor":bwversion[1],
		"versionPatch":bwversion[2]
	}
	
	if branchTag:
		branchName += "_" + branchTag
	
	print
	print "Testing branch: %s" % branchName
	
	# Be sure to flush out any existing test results
	_resetTestOutput( resPath )
	
	# Resolve paths for various configuration XML files
	referencePreferences = util.resolveDataFilePath( "preferences.xml" )
	preferences = os.path.normpath(
			os.path.join( os.path.dirname( exePath ), "preferences.xml" )
		)
		
	if not os.path.exists( referencePreferences ):
		raise BWTestingError( "Could not find reference preferences at '%s'." % referencePreferences )
		
	if util.replaceLineInFile( engineXMLPath, engineXMLPath, 
		"<spaceType> COMPILED_SPACE </spaceType>", 
		"<spaceType> CHUNK_SPACE </spaceType>" ):
		print "Replace <spaceType> COMPILED_SPACE </spaceType> with <spaceType> CHUNK_SPACE </spaceType>"
		
	scriptIndex = 0
	
	# Run a primer run once at the beginning and ignore the result 
	# This will 'prime' the cache with the data we're interested in
	if primer_run:
		print "--run primer run--"
		print "The results won't be saved"
		while open_automate.checkScriptExists( resPath, OO_LOADTIME_TEST, scriptIndex ):
			open_automate.runScript( exePath, flags, resPath, OO_LOADTIME_TEST, scriptIndex, primer_run )
			scriptIndex += 1
		# flush out test results
		_resetTestOutput( resPath )
		scriptIndex = 0	
		print "--End primer run--"
		
	while open_automate.checkScriptExists( resPath, OO_LOADTIME_TEST, scriptIndex ):
		# Check to see if a reference value has been calculated for this test combination.
		referenceValueSet = True
		benchmarkNames = open_automate.collectBenchmarkNames( resPath, OO_LOADTIME_TEST, scriptIndex )
		unsetReferences = []
		for benchmarkName in benchmarkNames:
			referenceValue = LoadTimeReferenceValue.get( dbSession, buildConfig, exeType, branchName, benchmarkName )
			if referenceValue is None:
				unsetReferences.append( benchmarkName )
		
		if len(unsetReferences) > 0:
			print
			print "-----"
			print "Reference values not set for:", ", ".join( unsetReferences )
			print "BEGIN reference calculation."
			
			_calculateReferenceValue( buildConfig, exeType, flags, branchName, exePath, resPath,
					OO_LOADTIME_TEST, scriptIndex, dbSession, benchmarkName )
					
			print "END reference calculation."
			print "-----"
			print
		
		open_automate.runScript( exePath, flags, resPath, OO_LOADTIME_TEST, scriptIndex )
		scriptIndex += 1
	
	# Process results
	reportHolder.addReport( _processResults( resPath, buildConfig, exeType, branchName, dbSession, changelist ) )
	
	