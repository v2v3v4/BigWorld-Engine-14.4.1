import os
import sys
import optparse
import simplejson as json
import pprint
import datetime
from math import sqrt

from performance_test import database
from performance_test.database import ReferenceValue, FrameRateResult, LoadTimeResult, SmokeTestResult, LoadTimeReferenceValue, SmokeTestReferenceValue
from performance_test.constants import *

from simplejson import encoder
encoder.FLOAT_REPR = lambda o: "%.2f" % o # Avoid stupid amount of precision

loadTimeSpaceMapping = {'spaces/08_ruinberg' : 'RunProfilerLoadtimeRuinberg', 
		'spaces/highlands' : 'RunProfilerLoadtimeHighlands', 
		'spaces/minspec' : 'RunProfilerLoadtimeMinspec', 
		'spaces/arctic' : 'RunProfilerLoadtimeArctic'};

EXPORT_TYPE = None # THIS MUST BE SET TO EITHER FrameRateResult or LoadTimeTest

def _jsonHandler(obj):
	if isinstance( obj, datetime.date ):
		return (obj.day, obj.month, obj.year)
	else:
		raise TypeError( 'Object of type %s with value of %s is not JSON serializable' % (type(obj), repr(obj)) )


COLUMN_FORMAT_USAGE = 'column format: "hostname,configuration,executableType,branchName,benchmarkName,columnName"'

DATE_RANGE_BEGIN = datetime.datetime.now() - datetime.timedelta( 90 ) # ~3 months
DATE_RANGE_END = datetime.datetime.now()


# http://www.physics.rutgers.edu/~masud/computing/WPark_recipes_in_python.html
"""
Calculate mean and standard deviation of data x[]:
    mean = {\sum_i x_i \over n}
    std = sqrt(\sum_i (x_i - mean)^2 \over n-1)
"""
def meanstdv(x):
	if len( x ) == 1:
		return x[0], 0
	n, mean, std = len(x), 0, 0
	for a in x:
		mean = mean + a
	mean = mean / float(n)
	for a in x:
		std = std + (a - mean)**2
	std = sqrt(std / float(n-1))
	return mean, std

class OutputColumn:
	def __init__( self, dbSession, hostname, configuration, executable, branchName, benchmarkName, name, value ):
		self.hostname = hostname
		self.configuration = configuration
		self.executable = executable
		self.branchName = branchName
		self.benchmarkName = benchmarkName
		self.name = name		
		
		if EXPORT_TYPE is FrameRateResult:
			v = ReferenceValue.get( dbSession, self.configuration, self.executable,
							self.branchName, self.benchmarkName, self.hostname )
			self.referenceValue = v.value if v is not None else 0
		elif EXPORT_TYPE is LoadTimeResult:
			v = LoadTimeReferenceValue.get( dbSession, self.configuration, 
									self.executable,self.branchName, 
									loadTimeSpaceMapping[self.benchmarkName], 
									self.hostname)
			self.referenceValue = v.value if v is not None else 0	
		elif EXPORT_TYPE is SmokeTestResult:
			v = SmokeTestReferenceValue.get( dbSession, self.configuration, 
									self.executable,self.branchName, 
									self.benchmarkName, 
									self.hostname)
			self.referenceValue = getattr(v, value) if v is not None else 0						
				
			
		queryFilter = 	{
							'hostname': self.hostname,
							'branchName': self.branchName,
						}
		if EXPORT_TYPE is not SmokeTestResult:
			queryFilter['executable'] = self.executable
			
		queryFilter['configuration'] = self.configuration
		queryFilter['benchmarkName'] = self.benchmarkName
			
		self.rows = dbSession.query( EXPORT_TYPE ).filter_by(
						**queryFilter ). \
						order_by( EXPORT_TYPE.date ). \
						filter( EXPORT_TYPE.date >= DATE_RANGE_BEGIN ). \
						all()
						
		#self.outputData = [ (x.date, x.value) for x in self.rows ]
		self.outputData = [ (x.date, getattr(x, value)) for x in self.rows ]
		
		values = [ getattr(x, value) for x in self.rows ]
		#values = [ x.value for x in self.rows ]
		self.minValue = min( values ) if values else 0
		self.maxValue = max( values ) if values else 0
		self.mean, self.stdv = meanstdv( values ) if values else (0,0)
		
		
		


def main():
	usage = 'usage: %prog [options] column1 column2 ...'
	
	parser = optparse.OptionParser( usage )
	
	parser.add_option( "-d", "--database",
					dest = "database_type", default=DB_PRODUCTION,
					help = "database engine: " + "|".join( DB_TYPES ) )
	parser.add_option( "-c", "--column-value",
					dest = "column_value", default=COLUMN_VALUE,
					help = "Column value: " + "|".join( COLUMN_VALUES ) )
					
	
	(options, args) = parser.parse_args()
	
	dbType = options.database_type	
	if options.database_type not in DB_TYPES:
		printUsage( parser )
		return FAILURE
	
	if len(args) < 1:
		printUsage( parser )
		return FAILURE

	columns = []
	
	dbSession = database.createSession( dbType )
		
	for arg in args:
		columnFilters = arg.split( "," )
		try:
			hostname = columnFilters[0]
			configuration = columnFilters[1]
			executable = columnFilters[2]
			branchName = columnFilters[3]
			benchmarkName = columnFilters[4]
			columnName = columnFilters[5]
		except IndexError:
			print "ERROR: Not enough filter arguments provided to column:", arg
			print COLUMN_FORMAT_USAGE
			return FAILURE
			
		columns.append( OutputColumn( dbSession, hostname, configuration, executable, branchName, benchmarkName, columnName, options.column_value ) )
			
	minDate = DATE_RANGE_BEGIN
	maxDate = DATE_RANGE_END
	
	outputColumns = []
	for column in columns:
		outputColumn = {
			"name": column.name,
			"hostname": column.hostname,
			"configuration": column.configuration,
			"executable": column.executable,
			"branch": column.branchName,
			"benchmark": column.benchmarkName,
			"min": column.minValue,
			"max": column.maxValue,
			"mean": column.mean,
			"stdv": column.stdv,
			"data": column.outputData 
		}
		
		if EXPORT_TYPE is FrameRateResult or EXPORT_TYPE is LoadTimeResult or EXPORT_TYPE is SmokeTestResult:
			outputColumn["referenceValue"] = column.referenceValue
		outputColumns.append( outputColumn )
	
	
	output = {
		"dateMin": minDate,
		"dateMax": maxDate,
		"columns": outputColumns
	}
	
	json.dump( output, sys.stdout, default=_jsonHandler )
	
	
		
	return SUCCESS

def printUsage( parser ):
	parser.print_help()
	print
	print COLUMN_FORMAT_USAGE
	print '\tvalid executableTypes:', ', '.join( EXE_TYPES )
	
		
