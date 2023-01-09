#include "pch.hpp"

#include "terrain/terrain2/resource.hpp"


BW_BEGIN_NAMESPACE

using namespace Terrain;

// A dummy object that pretends to text from a file on the file system.
class DummyTextObject : public SafeReferenceCount
{
public:
	static const uint32 THRESHOLD = 10;

	bool doLoad( const BW::string & path )
	{
	//	BWUnitTest::unitTestInfo("Loading DummyTextObject @ %s ...\n", path.c_str() );
		mySleep( 100 );
	//	BWUnitTest::unitTestInfo("Done.\n" );
		return true;
	}
};

// A resource which wraps the dummy object, the resource is loaded if the 
// threshold parameter is above required value.
class DummyTextResource : public Resource< DummyTextObject >
{
public:
 	DummyTextResource( BW::string path ) : 
		path_( path )
 	{
 	}

	~DummyTextResource()
	{
	}

	ResourceRequired evaluate( uint32 threshold )
	{
		if ( threshold > DummyTextObject::THRESHOLD )
		{
			required_ = RR_Yes;
		}
		else
		{
			required_ = RR_No;
		}

		return required_;
	}

 	bool load()
 	{
		// assign and create in a temporary
		ObjectTypePtr tempObj = new ObjectType();
 		bool status = tempObj->doLoad( path_ );

		// Then assign whole object so we never have a partially initialised
		// object.
		if ( status )
		{
			object_ = tempObj;
		}

		return status;
 	}

private:
	BW::string path_;
};

struct Fixture
{
	Fixture()
	{
		BgTaskManager::instance().startThreads(1);
		FileIOTaskManager::instance().startThreads(1);
		fixture_ = new DummyTextResource( "c:/temp/dummy.txt" );
	}
	~Fixture()
	{
		fixture_ = NULL;
		BgTaskManager::instance().stopAll( false, false );
		FileIOTaskManager::instance().stopAll( false, false );
	}

	SmartPointer<DummyTextResource> fixture_;
};


TEST_F( Fixture, Resource_testConstruction )
{	
	CHECK( NULL			== fixture_->getObject() );
	CHECK( RS_Unloaded	== fixture_->getState() );
	CHECK( false		== fixture_->isRequired() );
}

TEST_F( Fixture, Resource_testEvaluate )
{
	// Not required
	CHECK( RR_No == fixture_->evaluate( 0 ) );
	CHECK( RR_No == fixture_->evaluate( 10 ) );
	CHECK( false == fixture_->isRequired() );

	// Now test alternating between the two 

	// Required
	CHECK( RR_Yes == fixture_->evaluate( 11 ) );

	// Not required again
	CHECK( RR_No == fixture_->evaluate( 10 ) );

	// Required again
	CHECK( RR_Yes == fixture_->evaluate( 11 ) );
}

TEST_F( Fixture, Resource_testStreamSync )
{
	// Stream when not required, shouldn't do anything
	CHECK( RR_No == fixture_->evaluate( 0 ) );
	fixture_->stream( RST_Syncronous );
	// nothing loaded still
	CHECK( RS_Unloaded == fixture_->getState() );
	CHECK( NULL == fixture_->getObject() );

	// Stream when required - no "loading" state, as it will happen immediately
	CHECK( RR_Yes == fixture_->evaluate( 11 ) );

	// here goes...
	fixture_->stream( RST_Syncronous );

	CHECK( RS_Loaded == fixture_->getState() );
	CHECK( NULL != fixture_->getObject() );
}

TEST_F( Fixture, Resource_testStreamAsync )
{
	// Stream when not required, shouldn't do anything
	CHECK( RR_No == fixture_->evaluate( 0 ) );
	fixture_->stream();
	// nothing loaded still
	CHECK( RS_Unloaded == fixture_->getState() );
	CHECK( NULL == fixture_->getObject() );

	// Stream when required, should go to "loading" state then
	// loaded (dummy object will take 0.5 second to load).
	CHECK( RR_Yes == fixture_->evaluate( 11 ) );

	// here goes...
	fixture_->stream();

	// should be loading, but object shouldn't be available for at least 0.1 
	// seconds.
 	CHECK( RS_Loading == fixture_->getState() );
	CHECK( NULL == fixture_->getObject() );
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();
	mySleep( 500 ); // sleep long enough so it should definitely be done.
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();
	CHECK( RS_Loaded == fixture_->getState() );
	CHECK( NULL != fixture_->getObject() );
}

// Test what happens when a resource is deleted while loading
TEST_F( Fixture, testStreamAsyncDelete )
{
	// Stream when required, should go to "loading" state then
	// loaded (dummy object will take 0.5 second to load).
	CHECK( RR_Yes == fixture_->evaluate( 11 ) );

	// here goes...
	fixture_->stream();

	// should be loading, but object shouldn't be available for at least 0.1 
	// seconds.
	CHECK( RS_Loading == fixture_->getState() );
	CHECK( NULL == fixture_->getObject() );
	BgTaskManager::instance().tick();
	FileIOTaskManager::instance().tick();

	fixture_ = NULL;

	mySleep( 500 ); // sleep long enough so it should definitely be done.
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();
}

BW_END_NAMESPACE

// test_resource.cpp
