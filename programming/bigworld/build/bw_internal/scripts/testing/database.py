import os
import sys
import csv
import socket
from constants import *

from datetime import datetime

import sqlalchemy
from sqlalchemy.engine.url import URL
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Column, Integer, Float, String, Enum, DateTime

now = datetime.now

SQLAlchemyBase = declarative_base()

class SmokeTestResult( SQLAlchemyBase ):
	__tablename__ = 'client_smoketest_results'
	
	id 				= Column( Integer, primary_key = True )
	hostName		= Column( String(256) )
	branchName		= Column( String(256) )
	dateTime		= Column( DateTime, default=now )
	testName		= Column( String(256) )
	configuration	= Column( Enum( BUILD_CONFIGS ) )
	timeToRun		= Column( Float )
	totalMemoryAllocations		= Column( Integer )
	peakAllocatedBytes	= Column( Integer )
	memLeakedCount		= Column( Integer )
	successState	= Column( Enum( SUCCESS_STATE ) )
	changelist		= Column( Integer )

	def __init__( self, testName, branchName, changelist , configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memLeakedCount):
		self.hostName = socket.gethostname()
		self.branchName = branchName
		self.testName = testName
		self.configuration = configuration
		self.successState = successState
		self.timeToRun = timeToRun
		self.totalMemoryAllocations = totalMemoryAllocations
		self.peakAllocatedBytes = peakAllocatedBytes
		self.memLeakedCount = memLeakedCount
		self.changelist = changelist
		
	def __repr__( self ):
		return "<SmokeTestResult('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s') : hostname=%s" % \
			(self.testName, self.branchName, self,changelist, self.configuration, self.timeToRun, self.totalMemoryAllocations, 
			self.peakAllocatedBytes, self.memLeakedCount, self.successState, self.hostname)
	
	@staticmethod
	def addResult( session, testName, branchName, changelist, configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memLeakedCount ):
		v = SmokeTestResult( testName, branchName, changelist, configuration, successState, timeToRun, totalMemoryAllocations, peakAllocatedBytes, memLeakedCount )
		session.add( v )
		session.commit()
		return v
		
class ProjectBuildTime( SQLAlchemyBase ):
	__tablename__ = 'project_build_time'
	
	id 				= Column( Integer, primary_key = True )
	dateTime		= Column( DateTime, default=now )
	hostName		= Column( String(256) )
	changelist		= Column( Integer )
	timeToBuild		= Column( String(256) )
	projectName		= Column( String(256) )
	executable		= Column( Enum( EXE_TYPES ) )

	def __init__( self, changelist, timeToBuild, projectName, executable ):
		self.hostName = socket.gethostname()
		self.changelist = changelist
		self.timeToBuild = timeToBuild
		self.projectName = projectName
		self.executable = executable		
		
	def __repr__( self ):
		return "<ProjectBuildTime('%s', '%s', '%s', '%s') : hostname=%s" % \
			(self,changelist, self.timeToBuild, self.projectName, self.executable,
			self.hostname)
	
	@staticmethod
	def addResult( session, changelist, timeToBuild, projectName, executable ):
		v = ProjectBuildTime( changelist, timeToBuild, projectName, executable )
		session.add( v )
		session.commit()
		return v
		
def createSession( dbType ):		
	url = URL( "mysql", 
		username = MYSQL_USER,
		password = MYSQL_PWD,
		host = MYSQL_HOST,
		database = MYSQL_DB )
		
	dbEngine = sqlalchemy.create_engine( url, echo=False )
	SQLAlchemyBase.metadata.create_all( dbEngine )
	
	Session = sqlalchemy.orm.sessionmaker( bind=dbEngine )
	return Session()
