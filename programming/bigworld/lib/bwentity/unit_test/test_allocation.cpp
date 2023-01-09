#include "pch.hpp"

#include "connection/condemned_interfaces.hpp"
#include "connection/entity_def_constants.hpp"
#include "connection/filter_environment.hpp"
#include "connection/avatar_filter.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection_model/bw_entity.hpp"
#include "connection_model/bw_entity_factory.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "connection_model/simple_space_data_storage.hpp"
#include "cstdmf/memory_stream.hpp"
#include "network/space_data_mappings.hpp"

namespace
{
static bool s_newCalled = false;
static bool s_freeCalled = false;

class FailingEntity : public BW::BWEntity
{
public:
	FailingEntity( BW::BWConnection * pConnection ) :
		BW::BWEntity( pConnection ) {}

private:
	void onMethod( int messageID, BW::BinaryIStream & data ) { return; }
	void onProperty( int messageID, BW::BinaryIStream & data,
		bool isInitialising ) { return; }
	void onNestedProperty( BW::BinaryIStream & data, bool isSlice,
		bool isInitialising ) { return; }
	int getMethodStreamSize( int methodID ) const { return -1; }
	int getPropertyStreamSize( int propertyID ) const { return -1; }
	const BW::string entityTypeName() const { return BW::string("Failing"); }
	bool initBasePlayerFromStream( BW::BinaryIStream & data ) { return false; }
	bool initCellPlayerFromStream( BW::BinaryIStream & data ) { return false; }
	bool restorePlayerFromStream( BW::BinaryIStream & data ) { return false; }
};


class TestEntityFactory : public BW::BWEntityFactory
{
	BW::BWEntity * doCreate( BW::EntityTypeID entityTypeID,
		BW::BWConnection * pConnection )
	{
		return new FailingEntity(pConnection);
	}
};


} // anonymous namespace


class MovementFilterEnvironmentStub : public BW::FilterEnvironment
{
public:

	virtual ~MovementFilterEnvironmentStub() {}

	virtual bool filterDropPoint( BW::SpaceID spaceID,
			const BW::Position3D & fall, BW::Position3D & land,
			BW::Vector3 * pGroundNormal = NULL )
	{
		return true;
	}

	virtual bool resolveOnGroundPosition( BW::Position3D & position, 
		bool & isOnGround )
	{
		return true;
	}

	virtual void transformIntoCommon( BW::SpaceID spaceID,
		BW::EntityID vehicleID, BW::Position3D & position,
		BW::Direction3D & direction )
	{
	}

	virtual void transformFromCommon( BW::SpaceID spaceID,
		BW::EntityID vehicleID, BW::Position3D & position,
		BW::Direction3D & direction )
	{
	}

	static MovementFilterEnvironmentStub & instance()
	{
		static MovementFilterEnvironmentStub tmp;
		return tmp;
	}
};

// Override operator new and operator delete
inline void * operator new( size_t size )
{
	s_newCalled = true;
	return malloc( size );
}


inline void * operator new[]( size_t size )
{
	s_newCalled = true;
	return malloc( size );
}


inline void operator delete( void* ptr )
{
	s_freeCalled = true;
	free( ptr );
}


inline void operator delete[]( void* ptr )
{
	s_freeCalled = true;
	free( ptr );
}


// MemoryOStream overrides operator new and operator delete so it should never
// call the above global operators
TEST( allocation_MemoryOStream_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::MemoryOStream * pStream = new BW::MemoryOStream();
	delete pStream;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_MemoryOStream_safe_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::MemoryOStream * pStream = new BW::MemoryOStream();
	bw_safe_delete( pStream );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_MemoryOStream_autoptr )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::MemoryOStream> pStream( new BW::MemoryOStream() );
	pStream.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_MemoryOStream_delete_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::BinaryOStream * pStream = new BW::MemoryOStream();
	delete pStream;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_MemoryOStream_safe_delete_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::BinaryOStream * pStream = new BW::MemoryOStream();
	bw_safe_delete( pStream );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_MemoryOStream_autoptr_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::BinaryOStream> pStream( new BW::MemoryOStream() );
	pStream.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


// CondemnedInterfaces overrides operator new and operator delete so it should never
// call the above global operators
TEST( allocation_CondemnedInterfaces_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::CondemnedInterfaces * pCondemnedInterfaces = new BW::CondemnedInterfaces();
	delete pCondemnedInterfaces;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_CondemnedInterfaces_safe_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::CondemnedInterfaces * pCondemnedInterfaces = new BW::CondemnedInterfaces();
	bw_safe_delete( pCondemnedInterfaces );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_CondemnedInterfaces_autoptr )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::CondemnedInterfaces> pCondemnedInterfaces(
		new BW::CondemnedInterfaces() );
	pCondemnedInterfaces.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


// LoginChallengeFactories overrides operator new and operator delete so it should never
// call the above global operators
TEST( allocation_LoginChallengeFactories_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::LoginChallengeFactories * pLoginChallengeFactories = new BW::LoginChallengeFactories();
	delete pLoginChallengeFactories;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_LoginChallengeFactories_safe_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::LoginChallengeFactories * pLoginChallengeFactories = new BW::LoginChallengeFactories();
	bw_safe_delete( pLoginChallengeFactories );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_LoginChallengeFactories_autoptr )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::LoginChallengeFactories> pLoginChallengeFactories(
		new BW::LoginChallengeFactories() );
	pLoginChallengeFactories.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


// SpaceDataMappings overrides operator new and operator delete so it should never
// call the above global operators
TEST( allocation_SpaceDataMappings_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::SpaceDataMappings * pSpaceDataMappings = new BW::SpaceDataMappings();
	delete pSpaceDataMappings;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_SpaceDataMappings_safe_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::SpaceDataMappings * pSpaceDataMappings = new BW::SpaceDataMappings();
	bw_safe_delete( pSpaceDataMappings );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_SpaceDataMappings_autoptr )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::SpaceDataMappings> pSpaceDataMappings(
		new BW::SpaceDataMappings() );
	pSpaceDataMappings.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_SpaceDataMappings_smartpointer )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::SpaceDataMappingsPtr pSpaceDataMappings( new BW::SpaceDataMappings() );
	pSpaceDataMappings = NULL;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


// AvatarFilter overrides operator new and operator delete so it should
// never call the above global operators
TEST( allocation_AvatarFilter_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::AvatarFilter * pAvatarFilter =
		new BW::AvatarFilter( MovementFilterEnvironmentStub::instance() );
	delete pAvatarFilter;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_AvatarFilter_safe_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::AvatarFilter * pAvatarFilter =
		new BW::AvatarFilter( MovementFilterEnvironmentStub::instance() );
	bw_safe_delete( pAvatarFilter );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_AvatarFilter_autoptr )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr< BW::AvatarFilter > pAvatarFilter(
		new BW::AvatarFilter( MovementFilterEnvironmentStub::instance() ) );
	pAvatarFilter.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_AvatarFilter_delete_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::MovementFilter * pAvatarFilter =
		new BW::AvatarFilter( MovementFilterEnvironmentStub::instance() );
	delete pAvatarFilter;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_AvatarFilter_safe_delete_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::MovementFilter * pAvatarFilter =
		new BW::AvatarFilter( MovementFilterEnvironmentStub::instance() );
	bw_safe_delete( pAvatarFilter );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST( allocation_AvatarFilter_autoptr_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr< BW::MovementFilter > pAvatarFilter(
		new BW::AvatarFilter( MovementFilterEnvironmentStub::instance() ) );
	pAvatarFilter.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}
// BWServerConnection overrides operator new and operator delete so it should
// never call the above global operators
struct BWServerConectionTestFixture
{
	BWServerConectionTestFixture() :
		entityFactory_(),
		spaceDataStorage_( new BW::SpaceDataMappings() ),
		loginChallengeFactories_(),
		condemnedInterfaces_(),
		entityDefConstants_()
	{
	}

	BW::BWServerConnection * createConnection()
	{
		return new BW::BWServerConnection( entityFactory_, spaceDataStorage_,
			loginChallengeFactories_, condemnedInterfaces_, entityDefConstants_,
			0.0 );
	}

	TestEntityFactory entityFactory_;
	BW::SimpleSpaceDataStorage spaceDataStorage_;
	BW::LoginChallengeFactories loginChallengeFactories_;
	BW::CondemnedInterfaces condemnedInterfaces_;
	BW::EntityDefConstants entityDefConstants_;
};


TEST_F( BWServerConectionTestFixture, allocation_BWServerConnection_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::BWServerConnection * pConnection = this->createConnection();
	delete pConnection;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST_F( BWServerConectionTestFixture, allocation_BWServerConnection_safe_delete )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::BWServerConnection * pConnection = this->createConnection();
	bw_safe_delete( pConnection );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST_F( BWServerConectionTestFixture, allocation_BWServerConnection_autoptr )
{
	BW::SimpleSpaceDataStorage spaceDataStorage( new BW::SpaceDataMappings() );
	BW::CondemnedInterfaces condemnedInterfaces;
	TestEntityFactory entityFactory;
	BW::EntityDefConstants entityDefConstants;

	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::BWServerConnection> pConnection(this->createConnection());
	pConnection.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST_F( BWServerConectionTestFixture, allocation_BWServerConnection_delete_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::BWConnection * pConnection = this->createConnection();
	delete pConnection;
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST_F( BWServerConectionTestFixture, allocation_BWServerConnection_safe_delete_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	BW::BWConnection * pConnection = this->createConnection();
	bw_safe_delete( pConnection );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


TEST_F( BWServerConectionTestFixture, allocation_BWServerConnection_autoptr_virtual )
{
	s_newCalled = false;
	s_freeCalled = false;
	std::auto_ptr<BW::BWConnection> pConnection(this->createConnection());
	pConnection.reset( NULL );
	CHECK( !s_newCalled );
	CHECK( !s_freeCalled );
}


// test_allocation.cpp
