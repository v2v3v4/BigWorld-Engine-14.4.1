import sys
import copy
import unittest
import sqlalchemy

from datetime import date

import database

from database import ReferenceValue, FrameRateResult, LoadTimeResult
from constants import *

TESTHOST = "TESTHOST"

class TestReferenceValue( unittest.TestCase ):
		
	def setUp( self ):
		self.session = database.createSession( DB_MEMORY )
		self.testData = [
			ReferenceValue( EXE_CONSUMER, "bw_2_0_full", "highlands", 15 ),
			ReferenceValue( EXE_RELEASE, "bw_2_0_full", "highlands", 16 ),
			ReferenceValue( EXE_RELEASE, "bw_2_0_no_umbra", "highlands", 17 ),
		]
	
	def _populateTestData( self ):
		self.session.add_all( self.testData  )
		self.session.commit()
	
	def test_basicInsert( self ):
		self._populateTestData()
		all = self.session.query( ReferenceValue ).all()
		self.assertTrue( len(all) == len(self.testData) )
		
	def test_get( self ):
		self._populateTestData()
		
		# Test something that should exist
		self.assertTrue( ReferenceValue.get( self.session, EXE_CONSUMER, "bw_2_0_full", "highlands", TESTHOST ) is not None )
				
		# Test something that shouldn't exist
		self.assertTrue( ReferenceValue.get( self.session, EXE_CONSUMER, "bw_2_0_full", "urban", TESTHOST ) is None )		
		
	def test_set( self ):
		self._populateTestData()		
		ReferenceValue.set( self.session, EXE_CONSUMER, "bw_2_0_full", "urban", 14 )
		ReferenceValue.set( self.session, EXE_CONSUMER, "bw_2_0_no_umbra", "highlands", 16  )
		
	def test_remove( self ):
		self._populateTestData()		
		
		all = self.session.query( ReferenceValue ).all()
		startLen = len(all)
		
		# Try to remove something that doesn't exist
		self.assertFalse( ReferenceValue.remove( self.session, EXE_CONSUMER, "bw_2_0_full", "urban", TESTHOST ) )
		all = self.session.query( ReferenceValue ).all()
		self.assertTrue( len(all) == startLen )
		
		# Try to remove something that should exist
		self.assertTrue( ReferenceValue.remove( self.session, EXE_RELEASE, "bw_2_0_no_umbra", "highlands", TESTHOST ) )
		all = self.session.query( ReferenceValue ).all()
		self.assertTrue( len(all) == startLen-1 )
		
		# Test what we just removed no longer exists
		self.assertTrue( ReferenceValue.get( self.session, EXE_RELEASE, "bw_2_0_no_umbra", "highlands", TESTHOST ) is None )
	
	def test_executableNameConstraint( self ):
		self._populateTestData()
		
		# Exectuable and name should be unique as a pair.
		# We should be able to add one or the other fine.
		# These two add's should work fine.
		bmv = ReferenceValue( EXE_CONSUMER, "bw_2_0_no_umbra", "highlands", 10 )
		self.session.add( bmv )
		
		bmv = ReferenceValue( EXE_RELEASE, "bw_2_0_no_job_system", "highlands", 10 )
		self.session.add( bmv )
		
		# However this add should fail the integrity check
		bmv = ReferenceValue( EXE_RELEASE, "bw_2_0_no_umbra", "highlands", 10 )
		self.session.add( bmv )
		self.assertRaises( sqlalchemy.exc.IntegrityError, self.session.commit )
		self.session.rollback()

		
class TestFrameRateResult( unittest.TestCase ):
		
	def setUp( self ):
		self.session = database.createSession( DB_MEMORY )
		self.testData = [
			FrameRateResult( EXE_CONSUMER, "bw_2_0_full", "highlands", 15 ),
			FrameRateResult( EXE_RELEASE, "bw_2_0_full", "highlands", 16 ),
			FrameRateResult( EXE_RELEASE, "bw_2_0_no_umbra", "highlands", 17 ),
		]
	
	def _populateTestData( self ):
		self.session.add_all( self.testData )
		self.session.commit()
		
	def _all( self ):
		return self.session.query( FrameRateResult ). \
				order_by( FrameRateResult.date ).all()
	
	def test_basicInsert( self ):
		self._populateTestData()
		all = self.session.query( FrameRateResult ).all()
		self.assertTrue( len(all) == len(self.testData) )
		
	def test_addResult( self ):
		self._populateTestData()
		
		# Add a result
		FrameRateResult.addResult( self.session, EXE_RELEASE, "bw_2_0_full", "urban", 15 )
		
		all = self._all()
		self.assertTrue( len(all) == len(self.testData)+1 )
		
		# Add the same result again, should have overwritten the previous result because
		# it has the same date stamp.
		FrameRateResult.addResult( self.session, EXE_RELEASE, "bw_2_0_full", "highlands", 16 )
		
		all = self._all()
		self.assertTrue( len(all) == len(self.testData)+1 )
		
		# Make sure the last result added is the NEW version, not the old one (i.e. check overwrite)
		# Do this by checking the value changed.
		all = self._all()
		self.assertTrue( all[-1].value == 16 )
		
	def test_removeByDate( self ):
		self._populateTestData()
		
		all = self.session.query( FrameRateResult ).all()
		startLen = len(all)
		
		FrameRateResult.removeByDate( self.session, EXE_CONSUMER, "bw_2_0_full", "highlands", date.today(), TESTHOST )
		
		all = self.session.query( FrameRateResult ).all()
		self.assertTrue( len(all) == startLen-1 )
		
		
		
		
class LoadTimeResultResult( unittest.TestCase ):
	
		
	def setUp( self ):
		self.session = database.createSession( DB_MEMORY )
		self.testData = [
			LoadTimeResult( EXE_CONSUMER, "bw_2_0_full", "highlands", 15 ),
			LoadTimeResult( EXE_RELEASE, "bw_2_0_full", "highlands", 16 ),
			LoadTimeResult( EXE_RELEASE, "bw_2_0_no_umbra", "highlands", 17 ),
		]
	
	def _populateTestData( self ):
		self.session.add_all( self.testData )
		self.session.commit()
	
	def test_basicInsert( self ):
		self._populateTestData()
		all = self.session.query( LoadTimeResult ).all()
		self.assertTrue( len(all) == len(self.testData) )
		
	def test_addResult( self ):
		self._populateTestData()
		LoadTimeResult.addResult( self.session, EXE_CONSUMER, "bw_2_0_full", "highlands", 15 )
		LoadTimeResult.addResult( self.session, EXE_CONSUMER, "bw_2_0_full", "highlands", 16 )
		
		all = self.session.query( LoadTimeResult ).all()
		self.assertTrue( len(all) == len(self.testData)+2 )
		
	

def run():
	database.LOCAL_HOST_NAME = TESTHOST
	module = sys.modules[__name__]
	test = unittest.defaultTestLoader.loadTestsFromModule( module )
	testRunner = unittest.TextTestRunner()
	result = testRunner.run( test )
	
	return result.wasSuccessful()
