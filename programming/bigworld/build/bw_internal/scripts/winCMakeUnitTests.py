#!/usr/bin/env python

import os
import shutil
import sys
import build_common
import xml.etree.ElementTree as ET
import fnmatch

'''
*** Change these to disable a specific unit test ***
Add the name of the executables you want to not run.
eg. To disable the network unit test, add "network_unit_test.exe" to the tuple
'''
EXCLUDED_TESTS = ()
INCLUDED_TESTS = ( "cstdmf_unit_test", "math_unit_test", "network_unit_test", "particle_unit_test", "physics2_unit_test", "resmgr_unit_test", "terrain_unit_test",  "offline_processor_unit_test" )

COVERAGE_VALIDATOR_EXE = "c:\\Program Files (x86)\\Software Verification\\C++ Coverage Validator\\coverageValidator.exe"
COVERAGE_VALIDATOR_CMD = "\"" + COVERAGE_VALIDATOR_EXE + "\""
EXCLUDED_COVERAGE_TESTS = ( "network", "duplo" )
EXCLUDED_COVERAGE_DIR = ( "third_party" )
COVERAGE_VALIDATOR_DIRECTORY = os.path.normpath("v:\\" )

UNIT_TEST_DIR32 = os.path.normpath(
os.path.join( build_common.getRootPath(), "../game/bin/unit_tests/win32" )
)

UNIT_TEST_DIR64 = os.path.normpath(
os.path.join( build_common.getRootPath(), "../game/bin/unit_tests/win64" )
)

UNIT_TEST_DIR_HYBRID32 = os.path.normpath(
os.path.join( UNIT_TEST_DIR32, "Hybrid" )
)

UNIT_TEST_DIR_HYBRID64 = os.path.normpath(
os.path.join( UNIT_TEST_DIR64, "Hybrid" )
)

'''
Directories of all unit tests.
'''
ALL_UNIT_TEST_DIRS = ( UNIT_TEST_DIR32, UNIT_TEST_DIR64 )

'''
*** Change these to enable/disable unit test types ***
Tuple of directories containing unit tests to be run.
Edit this to run debug, release, etc.
'''
ENABLED_UNIT_TEST_DIRS = (
	UNIT_TEST_DIR32,
	UNIT_TEST_DIR64 )

def cleanOldTests(codeCoverage):
	# Clean all unit tests
	for testDir in ALL_UNIT_TEST_DIRS:
		print
		print "* winMakeUnitTests.cleanOldTests() from", testDir
		
		filelist = [ f for f in os.listdir( testDir ) \
			if (f.rfind( "unit_test" ) != -1) ]
		for f in filelist:
			fileToRemove = os.path.normpath(
				os.path.join( testDir, f ) )
			print "Removing " + fileToRemove
			os.remove( fileToRemove )

def _runExe( fileName, codeCoverage ):

	# Get the name of the test from the filename
	( path, testName ) = os.path.split( fileName )

	# Check if it is excluded
	if testName in EXCLUDED_TESTS:
		print
		print "* Disabled unit test '%s'" % fileName

	# Attempt to run the test
	elif fileName.find( ".exe" ) != -1:
		if fileName.find( ".manifest" ) == -1:
			if codeCoverage and fileName.find( "win64" ) == -1:
				#run code coverage on 32bit debug file only
				_runCodeCoverage( fileName.replace("_h.exe", "_d.exe") )
			else:
				print
				print "* Running unit test '%s'" % fileName
				build_common.runCmd( fileName + " -v" )

def _runCodeCoverage( debugFileName ):
	#run code coverage on a spesific test
	testName = os.path.basename( debugFileName ).split("_unit_test")[0]
	
	if testName in EXCLUDED_COVERAGE_TESTS:
		print
		print "* Disabled unit test coverage '%s'" % debugFileName
	else:
		outputFolder = os.path.join( COVERAGE_VALIDATOR_DIRECTORY, testName )
		if not os.path.exists(outputFolder):
			os.makedirs(outputFolder)
		xmlOutputFile = os.path.join(outputFolder, testName+".xml")
		htmlOutputFile = os.path.join(outputFolder, testName+".html")
		
		#create a source hook list
		srcFile = os.path.join(outputFolder, "libtime.srchook")
		tmpfile = open( srcFile , 'w' )
		tmpfile.write( "Rule:DoHook\n" )
		srcFolder = ""
		for f in os.listdir(os.path.join(debugFileName.split("game")[0], "programming", "bigworld", "lib" )):
			if f in EXCLUDED_COVERAGE_DIR:
				continue
			srcFolder = os.path.join(debugFileName.split("game")[0], "programming", "bigworld", "lib", f ) #, testName
			tmpfile.write( srcFolder + "\\\n" )
		tmpfile.close()
				
		session = os.path.join(outputFolder, testName + ".cvm")
		sourceFileFilter = " -sourceFileFilterHookFile " + srcFile + " -exportUnhooked:Off "
		
		exportVariables = " -exportAsHTML " + htmlOutputFile + " -exportAsXML " \
						+ xmlOutputFile +  " -exportType SummaryAndCoverage "\
						+ "-exportDetailedReport:On "
						
		#run code covreage tool-
		cmd = COVERAGE_VALIDATOR_CMD + " -createProcessStartupThread -hideUI " \
		+ "-program " + debugFileName + sourceFileFilter + " -saveSession " \
		+ session + exportVariables

		print "\n* Running unit test '%s' with code coverage" % debugFileName
		build_common.runCmd( cmd )
		
		
		
		summaryNode = ET.parse(xmlOutputFile).getroot().find('SUMMARY')
		print "CODE COVERAGE: PERCENTAGE OF VISITED LINES IN LIB= " \
				+ summaryNode.find('NUMBER_OF_VISITED_LINES_PC').text

				
def _MergeCCResult( ):
	#create an html index file
	indexFile = open(os.path.join( COVERAGE_VALIDATOR_DIRECTORY, "index.html" ),'w')
	indexFile.write("<html><body>\n") 
	indexFile.write("<a href=\"" + os.path.join( "summary", "report.html" ) + "\">Summary Report</a><br><br>\n")
	# merge all Code Cover result into one report
	if not os.path.exists( os.path.join( COVERAGE_VALIDATOR_DIRECTORY, "summary" )):
		#create a summary folder
		os.makedirs(os.path.join( COVERAGE_VALIDATOR_DIRECTORY, "summary" ))
		
	mainSessionPath = os.path.join( COVERAGE_VALIDATOR_DIRECTORY, "summary", "report" )
	mainSession = mainSessionPath + ".cvm"
	#delete old report
	if os.path.exists( mainSession ):
		os.remove( mainSession )
	
	#list of all the tests directory - 
	folderList = [ f for f in os.listdir( COVERAGE_VALIDATOR_DIRECTORY ) \
			if os.path.isdir(os.path.join( COVERAGE_VALIDATOR_DIRECTORY, f )) ]
			
	for folder in folderList:
		if folder == "summary":
			continue
		#for each test directory search for the saved session file
		sessionlist = [ s for s in os.listdir(									\
						os.path.join( COVERAGE_VALIDATOR_DIRECTORY, folder ) )	\
						if s.endswith( ".cvm" ) ]
		for s in sessionlist:
			#add html report to the index html
			indexFile.write("<a href=\"" +os.path.join( folder, s.replace(".cvm", ".html") ) + "\">" + s.replace(".cvm", "") + "</a><br>\n") 
			
			#for each session, merge result to the Main session
			sessionToMerge = os.path.join( COVERAGE_VALIDATOR_DIRECTORY, folder, s ) 
			print "Merged " + s + " into the report"
			
			if not os.path.exists( mainSession ):
				#we build the merge report on the first result
				shutil.copyfile(sessionToMerge, mainSession)
				continue
			cmd = COVERAGE_VALIDATOR_CMD + " -hideUI -mergeClearNone"		\
					+ " -loadSession " + mainSession						\
					+ " -loadSession2 " + sessionToMerge + " -mergeSessions"\
					+ " -saveMergeResult " + mainSession
			os.system( cmd )			
			
	print "EXPORT " + mainSession
	cmd = COVERAGE_VALIDATOR_CMD + " -hideUI -refreshCoverage -refreshFunctions"\
			+ " -refreshFilesAndLines -loadSession " + mainSession 				\
			+ " -exportAsHTML " + mainSessionPath + ".html" + " -exportAsXML "	\
			+ mainSessionPath + ".xml" +  " -exportType SummaryAndCoverage"		\
			+ " -exportDetailedReport:On"
	os.system( cmd )
	
	indexFile.write("<br><hr><b>This is only a summery of the full report.<br>\n")
	indexFile.write("To view the full report:<br>\n")
	indexFile.write("* Connect to bbwindows05 using VNC<br>\n")
	indexFile.write("* open \"C++ Coverage Validator\"<br>\n")
	indexFile.write("* choose the test results you want from drive V:<br></b>\n")

	indexFile.write("</html></body>\n")
	indexFile.close()
		
def runUnitTests(codeCoverage = False):

	if codeCoverage and not os.path.exists( COVERAGE_VALIDATOR_EXE ):
		print "Code Coverage application is not installed."
		print COVERAGE_VALIDATOR_CMD
		codeCoverage = False
		
	for testDir in ENABLED_UNIT_TEST_DIRS:
		print "* running unit tests in directory " + testDir

		filelist = fnmatch.filter(os.listdir(testDir), '*unit_test*.exe')
		for f in filelist:
			fileToRun = os.path.normpath(
				os.path.join( testDir, f ) )
			print "* test found. Name: " + fileToRun
			currDir = os.getcwd()
			os.chdir( testDir )
			_runExe( fileToRun, codeCoverage )
			os.chdir( currDir )
	if codeCoverage:
		_MergeCCResult()
	return True

def main():
	runUnitTests()

if __name__ == "__main__":
	sys.exit( not main() )

# winMakeUnitTests.py
