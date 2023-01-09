import glob
import optparse
import os
from os.path import basename
from os.path import splitext
import shutil
import subprocess
import sys
import time
import threading
import database
import reporter
import util
import stat

from database import SmokeTestResult

WORLDEDITOR		= "worldeditor"
MODELEDITOR		= "modeleditor"
PARTICLEEDITOR	= "particleeditor"
NAVGEN			= "navgen"
TOOLS			= [ WORLDEDITOR, MODELEDITOR, PARTICLEEDITOR, NAVGEN ]
CLIENT			= "bwclient"
EXECUTABLES		= [ WORLDEDITOR, MODELEDITOR, PARTICLEEDITOR, NAVGEN, CLIENT ]
TIMEOUT			= 600

PACKAGE_ROOT	= os.getcwd()
DATA_DIR		= os.path.join( os.path.dirname( __file__ ), "data/" )
GAME_RESOURCE_PATH= os.path.join( PACKAGE_ROOT, "game/res/fantasydemo" )
TOOLS_SCRIPT_DIR= os.path.join( PACKAGE_ROOT, "game/res/fantasydemo/scripts/editor" )
CLIENT_SCRIPT_DIR= os.path.join( PACKAGE_ROOT, "game/res/fantasydemo/scripts/client" )
SPACE_DIR= os.path.join( PACKAGE_ROOT, "game/res/fantasydemo/spaces" )
BUILD_WIN32_DIR	= os.path.join( PACKAGE_ROOT, "game/bin/client/win32/" )
BUILD_WIN64_DIR	= os.path.join( PACKAGE_ROOT, "game/bin/client/win64/" )
BUILD_DIRS		= [ BUILD_WIN32_DIR, BUILD_WIN64_DIR ]

EXE_FILE		= "%s.exe"
OPTIONS_FILE	= "%s.options"
SETTINGS_FILE	= "%s_settings.xml"
BAK_FILE		= "%s.bak"
MEM_STATS_FILE	= "memStats.csv"
SPACE_SETTING_FILE = "space.settings"
PREFERENCES_FILE = "preferences.xml"

def forceDelete( path ):
	if os.path.exists(path):
		os.chmod(path, stat.S_IWUSR)
		os.remove(path)

def processMemStats( executable ):
	colLeakedBytes = None;
	colLeakedCount = None;
	colPeakAllocation = None;
	colTotalAllocationCount = None;
	memLeakedBytes = 0;
	memLeakedCount = 0;
	peakAllocation = 0;
	totalAllocationCount = 0;

	file = open( MEM_STATS_FILE )
	for line in file:
		columns = line.split( "," )
		if colLeakedBytes is None:
			col = 0;
			for column in columns:
				if column.strip() == "Memory leaked (bytes)":
					colLeakedBytes = col
				elif column.strip()  == "Peak allocation (bytes)":
					colPeakAllocation = col
				elif column.strip()  == "Total allocation count":
					colTotalAllocationCount = col
				elif column.strip()  == "Memory leak count":
					colLeakedCount = col
				col += 1
		else: 
			memLeakedBytes += int( columns[colLeakedBytes] )
			memLeakedCount += int( columns[colLeakedCount] )
			peakAllocation += int( columns[colPeakAllocation] )
			totalAllocationCount += int( columns[colTotalAllocationCount] )
			break
	
	return [ memLeakedBytes, memLeakedCount, peakAllocation, totalAllocationCount ]

#run program with timeout
class Command(object):
	def __init__(self, cmd):
		self.cmd = cmd
		self.process = ""#None

	def run(self, timeout, program):
		def target():
			self.returncode = 0
			try:
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
			self.subprocess_output =  "TIMEOUT ERROR. Total Time: " + str(timeToRun)
			self.returncode = 1
			
		return self.subprocess_output
	
def runToolsTest( build_dir, executable, test, reportHolder, branchName, changelist, dbType, configuration):
	exe_file = EXE_FILE % ( os.path.join( "..", "..", "tools", executable, "win64", executable ))
	exe_path = os.path.join( build_dir, exe_file )

	default_options_file = OPTIONS_FILE % ( executable )
	default_options_path = os.path.join( DATA_DIR, default_options_file )
	if not os.path.exists( default_options_path ):
		default_options_path = ""

	default_settings_file = SETTINGS_FILE % ( executable )
	default_settings_path = os.path.join( DATA_DIR, default_settings_file )
	if not os.path.exists( default_settings_path ):
		default_settings_path = ""

	test_paths = glob.glob( "%s%s*.py" % ( DATA_DIR, test ) ) + glob.glob( "%s%s%s*.py" % ( DATA_DIR, executable, test ) )
	for test_path in test_paths:
		test_file = basename( test_path )
		test_name = splitext( test_file )[0]

		options_file = OPTIONS_FILE % ( test_name )
		options_path = os.path.join( DATA_DIR, options_file )
		if not os.path.exists( options_path ):
			options_path = default_options_path

		settings_file = SETTINGS_FILE % ( test_name )
		settings_path = os.path.join( DATA_DIR, settings_file )
		if not os.path.exists( settings_path ):
			settings_path = default_settings_path

		if options_path != "":
			options_path_bak = BAK_FILE % ( options_path )
			forceDelete( options_path_bak )
			shutil.copy( options_path, options_path_bak )

		if settings_path != "":
			settings_path_bak = BAK_FILE % ( settings_path )
			forceDelete( settings_path_bak )
			shutil.copy( settings_path, settings_path_bak )

		# copy the test script to the resource directory
		temp_test_path = os.path.join( TOOLS_SCRIPT_DIR, test_file )
		forceDelete(temp_test_path)
		shutil.copy( test_path, temp_test_path )

		# run executable
		print "%s running %s..." % ( executable, test_name )
		cmd = "%s -noConversion -memdumpstats -unattended --script %s" % ( exe_path, test_name )
		if options_path != "":
			cmd += " --options %s" % ( options_path )
		if settings_path != "":
			cmd += " --settings %s" % ( settings_path )
		print cmd
		
		start_time = time.time()
		command = Command(cmd)
		subprocess_output = command.run(timeout=TIMEOUT, program=exe_file )
		res = command.returncode
		
		end_time = time.time()
		timeToRun = end_time - start_time
		totalMemoryAllocations = 0
		peakAllocatedBytes = 0
		memoryLeaks = 0
		
		# delete the test script from the resource directory
		forceDelete( temp_test_path )

		if options_path != "":
			forceDelete( options_path )
			os.rename( options_path_bak, options_path )

		if settings_path != "":
			forceDelete( settings_path )
			os.rename( settings_path_bak, settings_path )

		# check if the executable exited cleanly
		test_result = "\nComplete report: %s\n" % ( test_name )
		if res != 0:
			test_result += "FAILURE: %s did not cleanly shut down.\n" % ( executable )
			successState = database.FAIL_STAT
		else:
			test_result += "%s cleanly shut down.\n" % ( executable )
			successState = database.OK_STATE

			# parse the output csv file for memory leaks
			mem_stats = processMemStats( executable )
			if mem_stats[0] == 0:
				test_result += "no leaks detected.\n"
			else:
				test_result += "%s bytes leaked with a total of %s leaks.\n" % ( mem_stats[0], mem_stats[1] )
			totalMemoryAllocations = mem_stats[3]
			peakAllocatedBytes = mem_stats[2]
			memoryLeaks = mem_stats[1]
			
		#add subprocess.CalledProcessError to the result
		test_result += subprocess_output.replace( "\r","" )
		
		print test_result
		
		# Add to DB
		addToDB( dbType, test_name, branchName, changelist, configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memoryLeaks)

		reportHolder.addReport( reporter.Report( res == 0, test_name, test_result, branchName ) )

def runClientTest( build_dir, executable, test, reportHolder, branchName, changelist, dbType , configuration, flags):
	exe_file = EXE_FILE % ( executable + "_h" )
	exe_path = os.path.join( build_dir, exe_file )

	test_paths = glob.glob( "%s%s*.py" % ( DATA_DIR, test ) ) + glob.glob( "%s%s%s*.py" % ( DATA_DIR, executable, test ) )

	# copy the bwclient_preferences.xml to the bin folder
	preferences_file_path = os.path.join(build_dir, PREFERENCES_FILE)
	print "copy preferences.xml to " + build_dir
	# create a backup file for preferences.xml
	if os.path.exists( preferences_file_path ):
		forceDelete(BAK_FILE % ( preferences_file_path ))
		shutil.move( preferences_file_path, BAK_FILE % ( preferences_file_path ) )
	shutil.copy( os.path.join( DATA_DIR, "bwclient_" +PREFERENCES_FILE ), preferences_file_path )
				
	for test_path in test_paths:
		test_file = basename( test_path )
		test_name = splitext( test_file )[0]
			
		#search for the space name
		test_name_array = test_name.split( '_' )
		space_name = test_name_array[len( test_name_array )-1]
		
		#search for a space setting file
		temp_path = os.path.join( SPACE_DIR, space_name )
		org_space_setting_file = os.path.join( temp_path, SPACE_SETTING_FILE )	
		space_setting_path_bak = BAK_FILE % ( org_space_setting_file )
		
		#search for the new space setting file
		tmp_space_setting_file = "%s.%s"  % ( test_name, SPACE_SETTING_FILE )
		new_space_setting_file = os.path.join( DATA_DIR, tmp_space_setting_file )
			
		if os.path.exists( new_space_setting_file ):
			# create a backup file for the space setting
			if os.path.exists( org_space_setting_file ):
				shutil.copy( org_space_setting_file, space_setting_path_bak )
			# copy the space setting script to the space directory
			shutil.copy( new_space_setting_file, org_space_setting_file )
			print "Space settings has been changed\n"

		# copy the test script to the resource directory
		temp_test_path = os.path.join( CLIENT_SCRIPT_DIR, test_file )
		if os.path.exists( temp_test_path ):
			os.chmod( temp_test_path, stat.S_IWRITE )
		shutil.copy( test_path, temp_test_path )

		# run executable
		print "%s running %s..." % ( executable, test_name )
		cmd = "%s -noConversion %s -memdumpstats -unattended --script %s" % ( exe_path, flags, test_name )		
		print cmd
		
		start_time = time.time()
		
		command = Command(cmd)
		subprocess_output = command.run(timeout=TIMEOUT, program=exe_file)	
		res = command.returncode
		
		end_time = time.time()
		timeToRun = end_time - start_time
		totalMemoryAllocations = 0
		peakAllocatedBytes = 0
		memoryLeaks = 0
		
		# return the space setting to the space directory
		if os.path.exists( new_space_setting_file ) :
			forceDelete( org_space_setting_file )
			if	os.path.exists( space_setting_path_bak ):
				shutil.copy( space_setting_path_bak, org_space_setting_file)
				forceDelete( space_setting_path_bak )
		
		# delete the test script from the resource directory
		forceDelete( temp_test_path )

		# check if the executable exited cleanly
		test_result = "\nComplete report: %s\n" % ( test_name )
		if res != 0:
			test_result += "FAILURE: %s did not cleanly shut down.\n" % ( executable )
			successState = database.FAIL_STAT
		else:
			test_result += "%s cleanly shut down.\n" % ( executable )
			successState = database.OK_STATE

			# parse the output csv file for memory leaks
			mem_stats = processMemStats( executable )
			if mem_stats[0] == 0:
				test_result += "no leaks detected.\n"
			else:
				test_result += "FAILURE: %s bytes leaked with a total of %s leaks.\n" % ( mem_stats[0], mem_stats[1] )
				res = 1 
			totalMemoryAllocations = mem_stats[3]
			peakAllocatedBytes = mem_stats[2]
			memoryLeaks = mem_stats[1]
		#add subprocess.CalledProcessError to the result
		test_result += subprocess_output.replace( "\r","" )
		print test_result

		addToDB( dbType, test_name, branchName, changelist, configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memoryLeaks )
		
		reportHolder.addReport( reporter.Report( res == 0, test_name, test_result, branchName ) )
		
	#return the preferences.xml to the bin folder
	forceDelete( preferences_file_path )
	if os.path.exists(BAK_FILE % ( preferences_file_path )):
		shutil.move( BAK_FILE % ( preferences_file_path ), preferences_file_path )

def runTest( executable, test, reportHolder, branchName, changelist, dbType, flags ):
	if executable in TOOLS:
		runToolsTest( BUILD_WIN64_DIR, executable, test, reportHolder, branchName, changelist, dbType, database.BUILD_WIN64 )
	else:
		runClientTest( BUILD_WIN32_DIR, executable, test, reportHolder, "%s (WIN32)" % branchName, changelist, dbType, database.BUILD_WIN32, flags )
		
def addToDB( dbType, test_name, branchName, changelist, configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memoryLeaks ):
	if dbType == database.DB_PRODUCTION:
		dbSession = database.createSession( dbType )
		# Add to MYSQL server
		SmokeTestResult.addResult( dbSession, test_name, branchName, changelist, configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memoryLeaks )

def runTests():
	bwversion = util.bigworldVersion()
	usage = "usage: %prog [options] test"
	
	parser = optparse.OptionParser( usage )
	
	parser.add_option( "-e", "--executable",
					   dest = "executable", default=None,
					   help = "Specific executable to test\n%s" % ( EXECUTABLES ) )

	parser.add_option( "-t", "--tools",
					   dest = "tools", default=True,
					   help = "Should the tools be tested?" )

	parser.add_option( "-c", "--client",
					   dest = "client", default=True,
					   help = "Should the client be tested?" )
	
	parser.add_option( "--compiled_space",
						action = "store_true",
						dest = "compiled_space", default=False,
						help = "Change space type from chunk space to compiled space" )
					   
	parser.add_option( "-u", "--url ",
					   dest = "url", default=None,
					   help = "The URL for the results in jenkins" )
					   
	parser.add_option( "-d", "--database",
					dest = "database_type", default=database.DB_MEMORY,
					help = "Database engine: " + "|".join( database.DB_TYPES ) \
							+ " (default="+database.DB_MEMORY+")" )
					
	parser.add_option( "-b", "--branchName",
					dest = "branchName", default="",
					help = "Specify branch name, default=bw_%d_%d" % ( bwversion[0], bwversion[1] ) )
					
	parser.add_option( "--changelist",
					dest = "changelist", default=0,
					help = "Specify p4 changelist number" )
					
	(options, args) = parser.parse_args()
	
	branchName = "bw_%d_%d" % ( bwversion[0], bwversion[1] )

	if not options.branchName == "":
		branchName += "_" + options.branchName
	
	test = "smoketest"
	try:
		test = args[0].lower()
	except:
		pass

	if ( options.executable != None and options.executable not in EXECUTABLES ) or options.database_type not in database.DB_TYPES :
	   parser.print_help()
	   return 1

	dbType = options.database_type	
	
	reportHolder = reporter.ReportHolder( "Automated Testing",
			"%s on %s" % ( test, branchName ), options.url, options.changelist )

	engineXMLPath = util.engineConfigXML( GAME_RESOURCE_PATH )
		

	if util.replaceLineInFile( engineXMLPath, engineXMLPath, 
		"<spaceType> COMPILED_SPACE </spaceType>", 
		"<spaceType> CHUNK_SPACE </spaceType>" ):
		print "Replace <spaceType> COMPILED_SPACE </spaceType> with <spaceType> CHUNK_SPACE </spaceType>"
	
	flags = ""
	if options.compiled_space:
		flags += "-spaceType COMPILED_SPACE"
		
	if options.executable != None:
		runTest( options.executable, test, reportHolder, branchName, options.changelist, dbType, flags)
	else:

		for executable in EXECUTABLES:
			if executable in TOOLS and options.tools == True:
				runTest( executable, test, reportHolder, branchName, options.changelist, dbType, flags)
			if executable == CLIENT and options.client == True:
				runTest( executable, test, reportHolder, branchName, options.changelist, dbType, flags)
	
				
	reportHolder.sendMail()

	for report in reportHolder.reports:
		if not report.success:
			return 1

	return 0

if __name__ == "__main__":
	sys.exit( runTests() )