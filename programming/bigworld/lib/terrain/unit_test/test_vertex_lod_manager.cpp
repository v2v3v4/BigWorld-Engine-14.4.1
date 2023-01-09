#include "pch.hpp"

#include "terrain/terrain2/vertex_lod_manager.hpp"
#include "terrain/terrain2/terrain_height_map2.hpp"
#include "terrain/terrain_settings.hpp"


BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace 
{
	const int		NUM_LODS = 8;

	const uint32	TOP_LOD = 0;

	class DummyTerrainBlock2 : public Terrain::TerrainBlock2
	{
	public:
		DummyTerrainBlock2() : Terrain::TerrainBlock2( NULL )
		{
			heightMap2( new TerrainHeightMap2( blockSize(), 129 ) );
		}
	};

	// This is a vertex lod manager with a deliberate pause in the load, to make
	// asynchronous testing a bit more interesting.
	class SlowVertexLodManager : public VertexLodManager
	{
	public:
		SlowVertexLodManager( 
			TerrainBlock2&	owner,
			uint32			numLods ) : 
		VertexLodManager( owner, numLods )
		{
		}

	protected:

		// Running in tests, we can't create DX resources, but we can create an
		// empty lod entry, which is sufficient for the tests.
		virtual bool generate( uint32 lod )
		{
			// don't load if already there
			if ( (*object_)[ lod ].exists() )
			{
				return true;
			}

			// create a dummy vertex lod entry
			mySleep(100);
			(*object_)[ lod ] = new VertexLodEntry();

			return true;
		}
	};

	struct Fixture
	{
		Fixture()
		{
			BgTaskManager::instance().startThreads(1);
			FileIOTaskManager::instance().startThreads(1);
			dummyTerrainBlock_ = new DummyTerrainBlock2();		
			fixture_ = new SlowVertexLodManager( *dummyTerrainBlock_, NUM_LODS );
		}

		~Fixture()
		{
			fixture_ = NULL;
			dummyTerrainBlock_ = NULL;
			BgTaskManager::instance().stopAll( false, false );
			FileIOTaskManager::instance().stopAll( false, false );
		}

		SmartPointer<SlowVertexLodManager>	fixture_;
		SmartPointer<DummyTerrainBlock2>	dummyTerrainBlock_;
	};
}

// Tests disabled temporarily, they don't work with CppUnitLite2. This should
// be re-enabled once tests go back into main branch.

TEST_F( Fixture, VertexLodManager_testConstruction )
{	
 	// main exists object and is loaded after construction
 	CHECK( NULL != fixture_->getObject() );
 	CHECK( RS_Loaded == fixture_->getState() );
}

TEST_F( Fixture, VertexLodManager_tearDown )
{
	fixture_ = NULL;
	BgTaskManager::instance().stopAll( false, false );
	FileIOTaskManager::instance().stopAll( false, false );
}

TEST_F( Fixture, VertexLodManager_testWorkingSet )
{
	VertexLodManager::WorkingSet set1, set2;

	CHECK( set1.start_ == 0 );
	CHECK( set1.end_	== 0 );

	CHECK( set1.IsWithin(set2) == true);

	set2.start_ = 1;
	set2.end_	= 2;

	CHECK( set1.IsWithin(set2) == false );

	set1.start_	= 2;
	set1.end_	= 3;

	CHECK( set1.IsWithin(set2) == false );

	set1.start_ = 1;
	set1.end_	= 2;

	CHECK( set1.IsWithin(set2) == true );
}

TEST_F( Fixture, VertexLodManager_testEvaluate )
{	
	// border cases - should only need two lods
	VertexLodManager::WorkingSet targetSet;

	// upper border
	fixture_->evaluate( 0, TOP_LOD );
	fixture_->getRequestedWorkingSet( targetSet );
	CHECK_EQUAL( uint32(0), targetSet.start_ );
	CHECK_EQUAL( uint32(1), targetSet.end_ );

	// lower border
	const uint32 LAST_LOD = NUM_LODS - 1;
	fixture_->evaluate( LAST_LOD, TOP_LOD );
	fixture_->getRequestedWorkingSet( targetSet );
	CHECK_EQUAL( LAST_LOD - 1, targetSet.start_ );
	CHECK_EQUAL( LAST_LOD, targetSet.end_ );

	// middle, should have a range of 3
	fixture_->evaluate( 1, TOP_LOD );
	fixture_->getRequestedWorkingSet( targetSet );
	CHECK_EQUAL( uint32(0), targetSet.start_ );
	CHECK_EQUAL( uint32(2), targetSet.end_ );
}

TEST_F( Fixture, VertexLodManager_testStreamSync )
{
	// Stream first two lod levels
	VertexLodManager::WorkingSet targetSet;

	fixture_->evaluate( 0, TOP_LOD );
	fixture_->getRequestedWorkingSet( targetSet );
	fixture_->stream( RST_Syncronous );

	CHECK( fixture_->getLod( targetSet.start_ ) != NULL );
	CHECK( fixture_->getLod( targetSet.end_ ) != NULL );

	// now load the next three - because we keep a lod if there is no overlap,
	// lod 0 should exist
	fixture_->evaluate( 3, TOP_LOD );
	fixture_->stream( RST_Syncronous );

	// loaded
	CHECK( fixture_->getLod(2) != NULL );
	CHECK( fixture_->getLod(3) != NULL );
	CHECK( fixture_->getLod(4) != NULL );
	CHECK( fixture_->getLod(0) != NULL );

	// unloaded
	CHECK( fixture_->getLod(1) == NULL );

	// overlap load based on lod 4.
	fixture_->evaluate( 4, TOP_LOD );
	fixture_->stream( RST_Syncronous );

	// loaded
	CHECK( fixture_->getLod(3) != NULL );
	CHECK( fixture_->getLod(4) != NULL );
	CHECK( fixture_->getLod(5) != NULL );

	// unloaded
	CHECK( fixture_->getLod(2) == NULL );
	CHECK( fixture_->getLod(6) == NULL );
}

TEST_F( Fixture, VertexLodManager_testStreamAsync )
{
	// Stream first two lod levels
	fixture_->evaluate( 0, TOP_LOD );
	fixture_->stream();

	CHECK( RS_Loading == fixture_->getState() );
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();	
	mySleep( 500 ); // sleep long enough so it should definitely be done.
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();	
	CHECK( RS_Loading != fixture_->getState() );

	CHECK( fixture_->getLod( 0 ) != NULL );
	CHECK( fixture_->getLod( 1 ) != NULL );
}

TEST_F( Fixture, VertexLodManager_testStreamMultiAsync )
{
	// Test multiple requests interleaved with stream calls
	CHECK( RS_Loading != fixture_->getState() );
	fixture_->evaluate( 0, TOP_LOD );
	fixture_->stream();
	CHECK( RS_Loading == fixture_->getState() );
	fixture_->evaluate( 3, TOP_LOD );
	fixture_->stream();
	CHECK( RS_Loading == fixture_->getState() );
	fixture_->evaluate( NUM_LODS - 1, TOP_LOD );
	fixture_->stream();
	CHECK( RS_Loading == fixture_->getState() );

	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();	
	mySleep( 500 ); // sleep long enough so it should definitely be done.
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();	
	CHECK( RS_Loading != fixture_->getState() );

	// Only first two should be loaded, others will be ignored because the
	// load was already issued.
	CHECK( fixture_->getLod( 0 ) != NULL );
	CHECK( fixture_->getLod( 1 ) != NULL );
	CHECK( fixture_->getLod( 2 ) == NULL );
	CHECK( fixture_->getLod( 3 ) == NULL );
	CHECK( fixture_->getLod( 4 ) == NULL );
	CHECK( fixture_->getLod( 5 ) == NULL );
	CHECK( fixture_->getLod( 6 ) == NULL );
	CHECK( fixture_->getLod( 7 ) == NULL );
}

// Test what happens when a resource is deleted while loading
TEST_F( Fixture, VertexLodManager_testStreamAsyncDelete )
{
	// Stream first two lod levels
	fixture_->evaluate( 0, TOP_LOD );
	fixture_->stream();

	CHECK( RS_Loading == fixture_->getState() );
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();	

	fixture_ = NULL;

	mySleep( 500 ); // sleep long enough so it should definitely be done.
	BgTaskManager::instance().tick();	
	FileIOTaskManager::instance().tick();	
}

BW_END_NAMESPACE

// test_vertex_lod_manager.cpp
