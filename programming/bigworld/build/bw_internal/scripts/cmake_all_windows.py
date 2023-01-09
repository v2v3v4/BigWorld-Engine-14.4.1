#!/usr/bin/env python

import os
import sys
import time
import datetime
import shutil
import optparse
import build_common
import re
from operator import add
from socket import error as socket_error

import winCMakeClient
import winCMakeUnitTests

def clean_metric_name(string):
	return re.sub( '\W', '_', string )

def addBuildTimeToMetricList( buildTime, metricPrefix, metricList ):
	duration, solution, config = buildTime
	metricName = ".".join( [metricPrefix,
		clean_metric_name (os.environ['JOB_NAME'] ),
		clean_metric_name( os.environ['NODE_NAME'] ),
		clean_metric_name( solution ), config] )
	metric = GraphiteMetric( metricName, duration.total_seconds() )
	metricList.add_metric( metric )

def submitMetrics( buildTimesMetricList, graphiteServer ):
	connection = GraphitePickleConnector( graphiteServer )
	try:
		connection.connect()
		connection.send_metrics( buildTimesMetricList )
		connection.close()
	except socket_error as serr:
		print "Warning: could not send stats to graphite - %s" % str( serr )

def main():
	# Setup the defaults, then tweak if it is a nightly (ie: full) build
	shouldCleanClient = False
	# Build time constants
	BT_TOTAL_PARENT = "build_total"
	BT_SOLUTIONS = "solutions"
	BT_UNITTESTS = "unit_tests"
	BT_ALL = "all"
	
	(options, args) = winCMakeClient.parseOptions()
	buildAndRunUnitTests = not options.disableUnitTest
		
	if options.mysql:
		import testing.database as database
		from testing.database import ProjectBuildTime

	# Get the build type, continuous, nightly etc
	try:
		buildType = build_common.stringToBuildType( options.build_type )
	except ValueError, ve:
		print "ERROR: %s" % str( ve )
		return False

	# Load the graphite client libraries
	if options.graphite:
		if 'JOB_NAME' in os.environ:
			sys.path.append( os.path.join( "..", "..", "..", "licensing", "tools" ) )
			global GraphiteMetricList, GraphiteMetric, GraphitePickleConnector
			from graphite_client.graphite_metric_list import GraphiteMetricList
			from graphite_client.graphite_metric import GraphiteMetric
			from graphite_client.graphite_connector import GraphitePickleConnector
			buildTimesMetricList = GraphiteMetricList()
			metricPrefix = 'stats.builds'
			graphiteServer = 'bwgraphite'
		else:
			raise Exception("Graphite stats can only be submitted from CI builds")
		
	# First we clean up any existing unit tests
	#winCMakeUnitTests.cleanOldTests(options.codeCoverag)
	
	# Only run a clean
	# If the --clean option is set, everything should be cleaned,
	# and no further building performed.
	if options.clean:
		winCMakeClient.clean( options, args )
		return True
		
	# Only run a flush
	# If the --flush option is set, remove all of the CMake generated files,
	# and no further building is performed
	elif options.flush:
		winCMakeClient.flush( options, args )
		return True

	# Only run PcLint
	# If the --pcLint option is set, create solutions 
	# Run Pc Lint on the source code,
	# and no further building performed.
	if options.pcLint:
		winCMakeClient.pcLint( options, args )
		return True
		
	# Full build - do extra cleaning
	if buildType == build_common.BUILD_TYPE_FULL:
		winCMakeClient.clean( options, args )

	# Now we can build the client, and all of it's related projects
	buildTimes = winCMakeClient.run( options, args )

	# Run unit tests
	if buildAndRunUnitTests:
		unitTestsStart = datetime.datetime.fromtimestamp( time.time() )
		winCMakeUnitTests.runUnitTests(options.codeCoverage)
		unitTestsEnd = datetime.datetime.fromtimestamp( time.time() )
		unitTestTime = ( unitTestsEnd-unitTestsStart, BT_TOTAL_PARENT, BT_UNITTESTS )

	print "\n\nAll build times:"
	for buildTime in buildTimes:
		if options.mysql:
			dbSession = database.createSession( database.DB_PRODUCTION )
			ProjectBuildTime.addResult( dbSession, options.changelist,
				*buildTime )
		if options.graphite:
			addBuildTimeToMetricList( buildTime, metricPrefix,
				buildTimesMetricList )
		print winCMakeClient.formatBuildTime( *buildTime )
	
	if buildAndRunUnitTests:
		if options.graphite:
			addBuildTimeToMetricList( unitTestTime, metricPrefix,
				buildTimesMetricList )
		print winCMakeClient.formatBuildTime( *unitTestTime )
	
	if options.graphite:
		totalSolutionTime = ( reduce( add, (bt[0] for bt in buildTimes) ),
			BT_TOTAL_PARENT, BT_SOLUTIONS )
		if buildAndRunUnitTests:
			totalBuildTime = ( totalSolutionTime[0] + unitTestTime[0],
				BT_TOTAL_PARENT, BT_ALL)
		else:
			totalBuildTime = ( totalSolutionTime[0], BT_TOTAL_PARENT, BT_ALL)
		
		addBuildTimeToMetricList( totalSolutionTime, metricPrefix,
			buildTimesMetricList )
		addBuildTimeToMetricList( totalBuildTime, metricPrefix,
			buildTimesMetricList )
		submitMetrics( buildTimesMetricList, graphiteServer )
	
	return True


if __name__ == "__main__":
	sys.exit( not main() )

# make_all_windows.py
