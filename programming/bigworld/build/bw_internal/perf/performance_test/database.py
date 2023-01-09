import os
import sys
import csv
import socket

from datetime import datetime, date

import sqlalchemy
from sqlalchemy.engine.url import URL
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.ext.hybrid import hybrid_property, hybrid_method
from sqlalchemy import Column, Integer, Float, String, Enum, Date, DateTime
from sqlalchemy.schema import UniqueConstraint

from constants import *

LOCAL_HOST_NAME = socket.gethostname()

OK_STATE		= "OK"
FAIL_STAT		= "FAIL"
SUCCESS_STATE = [ OK_STATE, FAIL_STAT ]

now = datetime.now
today = date.today

SQLAlchemyBase = declarative_base()
	
class ReferenceValue( SQLAlchemyBase ):
	__tablename__ = 'reference_values'
	
	id 				= Column( Integer, primary_key = True )
	hostname		= Column( String(256) )
	configuration	= Column( Enum( *BUILD_CONFIGS ) )
	executable 		= Column( Enum( *EXE_TYPES ) )
	branchName		= Column( String(256) )
	benchmarkName	= Column( String(256) )
	value			= Column( Float )
	
	__table_args__ = (
			UniqueConstraint( 'hostname', 'configuration', 'executable', 'branchName', 'benchmarkName' ),
		)

	def __init__( self, configuration, executable, branchName, benchmarkName, value ):
		self.hostname = LOCAL_HOST_NAME
		self.configuration = configuration
		self.executable = executable
		self.branchName = branchName
		self.benchmarkName = benchmarkName
		self.value = value
		
	def __repr__( self ):
		return "<ReferenceValue('%s', '%s', '%s', '%s', '%s') : hostname=%s>" % \
			(self.configuration, self.executable, self.branchName, \
			self.benchmarkName, self.value, self.hostname)
		
		
	@staticmethod
	def get( session, configuration, executable, branchName, benchmarkName, hostname=None ):
		if hostname is None:
			hostname = LOCAL_HOST_NAME
		
		return session.query( ReferenceValue ) \
			.filter_by( hostname=hostname, configuration=configuration, executable=executable, \
						branchName=branchName, benchmarkName=benchmarkName ) \
			.first()
			
	@staticmethod
	def set( session, configuration, executable, branchName, benchmarkName, value ):
		v = ReferenceValue( configuration, executable, branchName, benchmarkName, value )
		session.add( v )
		session.commit()
		return v
		
	@staticmethod
	def remove( session, configuration, executable, branchName, benchmarkName, hostname=None ):
		if hostname is None:
			hostname = LOCAL_HOST_NAME
		v = ReferenceValue.get( session, configuration, executable, branchName, benchmarkName, hostname )
		if v is not None:
			session.delete( v )
			session.commit()
			return True
		return False
		
class LoadTimeReferenceValue( SQLAlchemyBase ):
	__tablename__ = 'loadtime_reference_values'
	
	id 				= Column( Integer, primary_key = True )
	hostname		= Column( String(256) )
	configuration	= Column( Enum( *BUILD_CONFIGS ) )
	executable 		= Column( Enum( *EXE_TYPES ) )
	branchName		= Column( String(256) )
	benchmarkName	= Column( String(256) )
	value			= Column( Float )
	
	__table_args__ = (
			UniqueConstraint( 'hostname', 'configuration', 'executable', 'branchName', 'benchmarkName' ),
		)

	def __init__( self, configuration, executable, branchName, benchmarkName, value ):
		self.hostname = LOCAL_HOST_NAME
		self.configuration = configuration
		self.executable = executable
		self.branchName = branchName
		self.benchmarkName = benchmarkName
		self.value = value
		
	def __repr__( self ):
		return "<LoadTimeReferenceValue('%s', '%s', '%s', '%s', '%s') : hostname=%s>" % \
			(self.configuration, self.executable, self.branchName, \
			self.benchmarkName, self.value, self.hostname)
		
		
	@staticmethod
	def get( session, configuration, executable, branchName, benchmarkName, hostname=None ):
		if hostname is None:
			hostname = LOCAL_HOST_NAME
		
		return session.query( LoadTimeReferenceValue ) \
			.filter_by( hostname=hostname, configuration=configuration, executable=executable, \
						branchName=branchName, benchmarkName=benchmarkName ) \
			.first()
			
	@staticmethod
	def set( session, configuration, executable, branchName, benchmarkName, value ):
		v = LoadTimeReferenceValue( configuration, executable, branchName, benchmarkName, value )
		session.add( v )
		session.commit()
		return v
		
	@staticmethod
	def remove( session, configuration, executable, branchName, benchmarkName, hostname=None ):
		if hostname is None:
			hostname = LOCAL_HOST_NAME
		v = LoadTimeReferenceValue.get( session, configuration, executable, branchName, benchmarkName, hostname )
		if v is not None:
			session.delete( v )
			session.commit()
			return True
		return False
			
			
class FrameRateResult( SQLAlchemyBase ):
	__tablename__ = 'framerate_results'
	
	id 				= Column( Integer, primary_key = True )
	date			= Column( Date, default=today )
	hostname		= Column( String(256) )
	configuration	= Column( Enum( *BUILD_CONFIGS ) )
	executable 		= Column( Enum( *EXE_TYPES ) )
	branchName		= Column( String(256) )
	benchmarkName	= Column( String(256) )
	value			= Column( Float )
	changelist		= Column( Integer )
	dateTime		= Column( DateTime, default=now )

	def __init__( self, configuration, executable, branchName, benchmarkName, value, changelist ):
		self.hostname = LOCAL_HOST_NAME
		self.configuration = configuration
		self.executable = executable
		self.branchName = branchName
		self.benchmarkName = benchmarkName
		self.value = value
		self.changelist = changelist
		
	def __repr__( self ):
		return "<FrameRateResult('%s', '%s', '%s', '%s', '%s', '%s') : hostname=%s>" % \
			(self.configuration, self.executable, self.branchName, 
			self.benchmarkName, self.value, self.hostname, self.changelist)
		
	@staticmethod
	def addResult( session, configuration, executable, branchName, benchmarkName, value, changelist ):
		v = FrameRateResult( configuration, executable, branchName, benchmarkName, value, changelist )
		session.add( v )
		session.commit()
		return v
		
	@staticmethod
	def importCSV( session, csvFile ):
		numRows = 0
		f = open( csvFile, 'rb' )
		
		reader = csv.reader( f )
		headers = reader.next()
		
		# Map columns to indices
		c2i = {}
		for i in range(len(headers)):
			c2i[headers[i]] = i
			
		for row in reader:
			values = {
				"branchName": row[ c2i["branchName"] ],
				"configuration": row[ c2i["configuration"] ],
				"executable": row[ c2i["executable"] ],
				"benchmarkName": row[ c2i["benchmarkName"] ],
				"value": float( row[ c2i["value"] ] ),
				"changelist": row[ c2i["changelist"] ],
			}
			
			frr = FrameRateResult( **values )
			frr.hostname = row[ c2i["hostname"] ]
			frr.date = datetime.strptime( row[ c2i["timeStamp"] ], "%Y-%m-%d" ).date()
			
			session.add( frr )
			session.commit()
			
			numRows += 1
			
		print
			
		return numRows


class LoadTimeResult( SQLAlchemyBase ):
	__tablename__ = 'loadtime_results'
	
	id 				= Column( Integer, primary_key = True )
	date			= Column( Date, default=today )
	hostname		= Column( String(256) )
	configuration	= Column( Enum( *BUILD_CONFIGS ) )
	executable 		= Column( Enum( *EXE_TYPES ) )
	branchName		= Column( String(256) )
	benchmarkName	= Column( String(256) )
	changelist		= Column( Integer )
	value			= Column( Float )
	dateTime		= Column( DateTime, default=now )

	def __init__( self, configuration, executable, branchName, benchmarkName, value, changelist ):
		self.hostname = LOCAL_HOST_NAME
		self.configuration = configuration
		self.executable = executable
		self.branchName = branchName
		self.benchmarkName = benchmarkName
		self.changelist = changelist
		self.value = value
		
	def __repr__( self ):
		return "<LoadTimeResult('%s', '%s', '%s', '%s', '%s', '%s') : hostname=%s>" % \
			(self.configuration, self.executable, self.branchName, self.benchmarkName, self.value, self.changelist)
	
	@staticmethod
	def addResult( session, configuration, executable, branchName, benchmarkName, value, changelist ):
		v = LoadTimeResult( configuration, executable, branchName, benchmarkName, value, changelist )
		session.add( v )
		session.commit()
		return v
		
class SmokeTestReferenceValue( SQLAlchemyBase ):

	__tablename__ = 'smoketest_reference_values'
	
	id 						= Column( Integer, primary_key = True )
	hostname				= Column( "hostName", String(256) )
	branchName				= Column( String(256) )
	benchmarkName			= Column( "testName", String(256) )
	configuration 			= Column( Enum( *BUILD_CONFIGS ) )
	timeToRun				= Column( Float )
	totalMemoryAllocations	= Column( Integer )
	peakAllocatedBytes		= Column( Integer )
	memLeakedCount			= Column( Integer )
	
		
	__table_args__ = (
			UniqueConstraint( 'hostName', 'branchName', 'testName', 'configuration' ),
		)
	

	def __init__( self, branchName, benchmarkName, configuration,
		timeToRun, totalMemoryAllocation, peakAllocatedBytes,
		memLeakedCount ):
		self.hostname = LOCAL_HOST_NAME
		self.branchName = branchName
		self.benchmarkName = benchmarkName
		self.configuration = configuration
		self.timeToRun = timeToRun
		self.totalMemoryAllocation = totalMemoryAllocation
		self.peakAllocatedBytes = peakAllocatedBytes
		self.memLeakedCount = memLeakedCount
	

	@hybrid_property
	def executable(self):
		return ""
		
	def __repr__( self ):
		return "<SmokeTestReferenceValue('%s', '%s', '%s', '%s', '%s') : hostname=%s>" % \
			(self.configuration, self.executable, self.branchName, self.benchmarkName, self.totalMemoryAllocation)
	
	
	@staticmethod
	def get( session, configuration, executable, branchName, benchmarkName, hostname=None ):
		if hostname is None:
			hostname = LOCAL_HOST_NAME
		 
		return session.query( SmokeTestReferenceValue ) \
			.filter_by( hostname=hostname, configuration=configuration,  \
						branchName=branchName, benchmarkName=benchmarkName ) \
			.first()	
		
			
class SmokeTestResult( SQLAlchemyBase ):
	__tablename__ = 'client_smoketest_results'

	id 						= Column( Integer, primary_key = True )
	hostname				= Column( "hostName", String(256) )
	branchName				= Column( String(256) )
	date					= Column( "dateTime", Date, default=today )
	benchmarkName			= Column( "testName", String(256) )
	configuration 			= Column( Enum( *BUILD_CONFIGS ) )
	timeToRun				= Column( Float )
	totalMemoryAllocations	= Column( Integer )
	peakAllocatedBytes		= Column( Integer )
	successState			= Column( Enum( *SUCCESS_STATES ) )
	memLeakedCount			= Column( Integer )
	changelist				= Column( Integer )

	def __init__( self, branchName, date, benchmarkName, configuration,
		timeToRun, totalMemoryAllocation, peakAllocatedBytes, successState,
		memLeakedCount, changelist ):
		self.hostname = LOCAL_HOST_NAME
		self.branchName = branchName
		self.date = date
		self.benchmarkName = benchmarkName
		self.configuration = configuration
		self.timeToRun = timeToRun
		self.totalMemoryAllocation = totalMemoryAllocation
		self.peakAllocatedBytes = peakAllocatedBytes
		self.successState = successState
		self.memLeakedCount = memLeakedCount
		self.changelist = changelist


	def __repr__( self ):
		return "<SmokeTestResult('%s', '%s', '%s', '%s', '%s', '%s') : hostname=%s>" % \
			(self.configuration, self.executable, self.branchName, self.benchmarkName, self.totalMemoryAllocation, self.changelist)
	
	@hybrid_property
	def executable(self):
		return ""

	@staticmethod
	def addResult( session, branchName, dateTime, testName, configuration,
		timeToRun, totalMemoryAllocation, peakAllocatedBytes, successState,
		memLeakedCount, changelist ):
		v = LoadTimeResult( branchName, dateTime, testName, configuration,
			timeToRun, totalMemoryAllocation, peakAllocatedBytes, successState,
			memLeakedCount, changelist )
		session.add( v )
		session.commit()
		return v
		
def createSession( dbType ):		
	if dbType == DB_MEMORY:
		url = URL( "sqlite", database=":memory:" )
	elif dbType == DB_SQLITE:
		fullPath = os.path.join( os.path.dirname( __file__ ), "performance_test.db" )
		fullPath = os.path.normpath( fullPath )
		url = URL( "sqlite", database=fullPath )
	else:
		url = URL( "mysql", 
			username = MYSQL_USER,
			password = MYSQL_PWD,
			host = MYSQL_HOST,
			database = MYSQL_DB )
		
	dbEngine = sqlalchemy.create_engine( url, echo=False )
	SQLAlchemyBase.metadata.create_all( dbEngine )
	
	Session = sqlalchemy.orm.sessionmaker( bind=dbEngine )
	return Session()
