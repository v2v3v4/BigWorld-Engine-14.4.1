#include "CppUnitLite2/src/CppUnitLite2.h"

#include "cstdmf/memory_stream.hpp"

#include "db/db_hash_schemes.hpp"


BW_BEGIN_NAMESPACE


namespace // (anonymous)
{
	// Some test addresses.

	Mercury::Address ADDR_1( 0x01010101, 0x100 );
	Mercury::Address ADDR_3( 0x03030303, 0x300 );
	Mercury::Address ADDR_4( 0x04040404, 0x400 );
	Mercury::Address ADDR_7( 0x07070707, 0x700 );
} // end namespace (anonymous)


/**
 *	This struct holds static methods for populating test data suitable for the
 *	hash type in the template argument.
 */
template< typename BUCKET_TYPE, typename HASH >
struct DataPopulator;


/**
 *	This struct template specialisation provides DBAppID -> Mercury::Address
 *	test data.
 */
template< typename HASH >
struct DataPopulator< DBAppID, HASH >
{
	static void populateData( HASH & hash )
	{
		hash.insert( std::make_pair( 1, ADDR_1 ) );
		hash.insert( std::make_pair( 3, ADDR_3 ) );
		hash.insert( std::make_pair( 4, ADDR_4 ) );
		hash.insert( std::make_pair( 7, ADDR_7 ) );
	}
};


/**
 *	This struct template specialisation provides BW::string -> Mercury::Address
 *	test data.
 */
template< typename HASH >
struct DataPopulator< BW::string, HASH >
{
	static void populateData( HASH & hash )
	{
		hash.insert( std::make_pair( "alpha", ADDR_1 ) );
		hash.insert( std::make_pair( "beta", ADDR_3 ) );
		hash.insert( std::make_pair( "gamma", ADDR_4 ) );
		hash.insert( std::make_pair( "delta", ADDR_7 ) );
	}
};



/**
 *	This is a test function template to be instantiated for a given hash type.
 *
 *	@param result_ 	This is used by the CppUnitLite2 test macros.
 *	@param m_name 	This is used by the CppUnitLite2 test macros.
 */
template< typename HASH >
void testHashScheme( TestResult & result_, const char * m_name )
{
	HASH scheme;
	CHECK( scheme.empty() );

	typedef DataPopulator< typename HASH::Bucket, HASH> MyDataPopulator;
	MyDataPopulator::populateData( scheme );
	CHECK( !scheme.empty() );
	CHECK( scheme.size() > 0U );

	MemoryOStream stream;
	stream << scheme;

	HASH streamScheme;
	stream >> streamScheme;

	CHECK_EQUAL( scheme.size(), streamScheme.size() );

	typename HASH::const_iterator iter1 = scheme.begin();
	typename HASH::const_iterator iter2 = streamScheme.begin();

	while ((iter1 != scheme.end()) && (iter2 != streamScheme.end()))
	{
		CHECK( iter1->first == iter2->first );
		CHECK( iter1->second == iter2->second );
		++iter1;
		++iter2;
	}
}


/**
 *	This tests tests the integerToBigEndian function used for hashing numeric
 *	types.
 */
TEST( IntegerToBigEndian )
{
	unsigned char databaseIDBuffer[ sizeof(DatabaseID) ];
	BW_STATIC_ASSERT( sizeof(DatabaseID) == 8U, DatabaseID_size_mismatch );
	static const DatabaseID TEST_DBID = 0x0001020304050607;

	DBHashSchemes::integerToBigEndian( databaseIDBuffer, TEST_DBID );

	for (unsigned char i = 0; i < sizeof(DatabaseID); ++i)
	{
		CHECK_EQUAL( uint( i ), uint( databaseIDBuffer[i] ) );
	}

	unsigned char dbAppIDBuffer[ sizeof(DBAppID) ];
	BW_STATIC_ASSERT( sizeof(DBAppID) == 4U, DBAppID_size_mismatch );
	static const DBAppID TEST_DBAPPID = 0x00010203;

	DBHashSchemes::integerToBigEndian( dbAppIDBuffer, TEST_DBAPPID );

	for (unsigned char i = 0; i < sizeof(DBAppID); ++i)
	{
		CHECK_EQUAL( uint( i ), uint( dbAppIDBuffer[i] ) );
	}
}


/**
 *	This tests the hash schemes exposed in the DBHashSchemes namespace.
 */
TEST( Schemes )
{
	typedef DBHashSchemes::DBAppIDBuckets< Mercury::Address >::HashScheme
		DBAppIDScheme;
	typedef DBHashSchemes::StringBuckets< Mercury::Address >::HashScheme
		StringScheme;

	testHashScheme< DBAppIDScheme >( result_, m_name );
	testHashScheme< StringScheme >( result_, m_name );
}


BW_END_NAMESPACE 


// test_db_hash_schemes.cpp
