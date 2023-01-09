import os
import sys
import optparse

import performance_test
import performance_test.database as database
from performance_test.constants import *

def main( testType ):
	usage = "usage: %prog [options] build_config client_type"
	
	parser = optparse.OptionParser( usage )
	
	parser.add_option( "-u", "--unit_test",
					dest = "unit_test", default=False,
					action = "store_true",
					help = "Run unit tests and exit" )

	parser.add_option( "-d", "--database",
					dest = "database_type", default=DB_MEMORY,
					help = "Database engine: " + "|".join( DB_TYPES ) )
					
	parser.add_option( "-c", "--compiled_space",
					dest = "compiled_space", default=False,
					action = "store_true",
					help = "Change space type from chunk space to compiled space" )				

	parser.add_option( "-b", "--branch_tag",
					dest = "branch_tag", default="",
					help = "Additional tag to append to the base branch name." +
						"This is useful	for doing tests on feature branches." )
						
	parser.add_option( "-f", "--flags",
					dest = "flags", default="",
					help = "Add a list of flags to the client command line, " \
							+ "separated by semicolon -f -flag1;value;--flag2;flag3;value" )
						
	parser.add_option( "--changelist",
					dest = "changelist", default=0,
					help = "Specify p4 changelist number" )
					
	parser.add_option( "--primer_run",
				dest = "primer_run", default=False,
				action = "store_true",
				help = "Run the test once at the beginning and ignore the result." )
	
	(options, args) = parser.parse_args()
		
	if len(args) < 2:
		printUsage( parser )
		return FAILURE
	
	if options.unit_test:
		sys.exit( not performance_test.unit_tests.run() )
		
	dbType = options.database_type	
	if options.database_type not in DB_TYPES:
		printUsage( parser )
		return FAILURE
	
	flags = " ".join( options.flags.split(";") )	
	if options.compiled_space:
		flags += " -spaceType COMPILED_SPACE"
	dbSession = database.createSession( dbType )

	try:
		buildConfig = args[0].upper()
	except IndexError:
		printUsage( parser )
		return FAILURE

	if buildConfig not in BUILD_CONFIGS:
		print "%s is not in %s" % (buildConfig,BUILD_CONFIGS)
		printUsage( parser )
		return FAILURE
	
	try:
		exeType = args[1].upper()
	except IndexError:
		printUsage( parser )
		return FAILURE
	
	if exeType not in EXE_TYPES:
		print "%s is not in %s" % (exeType,EXE_TYPES)
		printUsage( parser )
		return FAILURE	
		
	testName = ""
	try:
		testName = args[2]
	except IndexError:
		#not a specific test
		pass
		
	try:
		bwversion = performance_test.util.bigworldVersion()
	except performance_test.util.BWTestingError as e:
		print "Could not determine BigWorld version from cwd: '%s'" % os.getcwd()
		return FAILURE
	
	print "Performance test suite (BigWorld version %d.%d.%d)" % bwversion
	print "Database session created (%s)" % dbType.lower()
	print
	
	if testType == TEST_FRAMERATE:
		reportHolder = performance_test.reporter.ReportHolder(
			"Framerate test on bw_%d_%d_%s changelist %s" % \
				(bwversion[0], bwversion[1], exeType.lower(), options.changelist) )
		testModule = performance_test.fps_test
	elif testType == TEST_LOADTIME:
		reportHolder = performance_test.reporter.ReportHolder(
			"Load Time test on bw_%d_%d_%s changelist %s" % \
				(bwversion[0], bwversion[1], exeType.lower(), options.changelist) )
		testModule = performance_test.loadtime_test
	else:
		assert( False ) # Unknown test type.
	
	# Main test
	try:
		testModule.run( buildConfig, exeType, dbSession, reportHolder, options.branch_tag, testName, options.compiled_space, options.changelist, options.primer_run, flags )
		reportHolder.sendMail( REPORT_EMAIL_ADDRESSES )
		sys.stdout.write( "\nOK\n" )
		return SUCCESS
	except performance_test.util.BWTestingError as e:
		sys.stderr.write( "\n------\n" )
		sys.stderr.write( "Runtime error:\n\n" )
		sys.stderr.write( str(e) + "\n" )
		sys.stderr.write( "\nFAIL\n" )
		return FAILURE
		
	

def printUsage( parser ):
	parser.print_help()
	print
	print "Valid build_configs:", ", ".join( BUILD_CONFIGS )
	print "Valid client_types:", ", ".join( EXE_TYPES )

