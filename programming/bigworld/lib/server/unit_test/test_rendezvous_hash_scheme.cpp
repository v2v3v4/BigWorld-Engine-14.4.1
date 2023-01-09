#include "CppUnitLite2/src/CppUnitLite2.h"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/timestamp.hpp"

#include "network/basictypes.hpp"

#include "server/murmur_hash.hpp"
#include "server/rendezvous_hash_scheme.hpp"

#include <cmath>


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

// Some test data.


const Mercury::Address ADDR_1( 0x01010101, 0x100 );
const Mercury::Address ADDR_3( 0x03030303, 0x300 );
const Mercury::Address ADDR_4( 0x04040404, 0x400 );
const Mercury::Address ADDR_6( 0x06060606, 0x600 );
const Mercury::Address ADDR_7( 0x07070707, 0x700 );

const Mercury::Address & ADDR_BLUE 		= ADDR_1;
const Mercury::Address & ADDR_RED 		= ADDR_3;
const Mercury::Address & ADDR_GREEN 	= ADDR_4;
const Mercury::Address & ADDR_YELLOW 	= ADDR_6;
const Mercury::Address & ADDR_PURPLE 	= ADDR_7;


} // end namespace (anonymous)

/**
 *	This class is used for RendezvousHashSchemes involving AppID buckets.
 */
template< typename MAPPED_TYPE, 
		typename BUCKET_MAP = BW::map< ServerAppInstanceID, MAPPED_TYPE > >
class AppIDDatabaseIDHashAlgorithm
{
public:
	// The hashing scheme that uses this algorithm.
	typedef RendezvousHashSchemeT<
			DatabaseID,
			ServerAppInstanceID,
			MAPPED_TYPE,
			AppIDDatabaseIDHashAlgorithm< MAPPED_TYPE, BUCKET_MAP >,
			BUCKET_MAP >
		HashScheme;

	// Test data to be used with this algorithm.
	typedef BW::map< ServerAppInstanceID, MAPPED_TYPE > TestData;

	// The output type of the hash. Required for use with
	// RendezvousHashSchemeT.
	typedef uint64 Value;

	/**
	 *	This method implements the hashing for a bucket name and the key.
	 */
	Value hash( ServerAppInstanceID appID, DatabaseID dbID ) const
	{
		unsigned char buf[sizeof( appID ) + sizeof( dbID )];

		buf[0]  = (appID >> 24) & 0xFF;
		buf[1]  = (appID >> 16) & 0xFF;
		buf[2]  = (appID >> 8) & 0xFF;
		buf[3]  = appID & 0xFF;

		buf[4]  = (dbID >> 56) & 0xFF;
		buf[5]  = (dbID >> 48) & 0xFF;
		buf[6]  = (dbID >> 40) & 0xFF;
		buf[7]  = (dbID >> 32) & 0xFF;
		buf[8]  = (dbID >> 24) & 0xFF;
		buf[9]  = (dbID >> 16) & 0xFF;
		buf[10] = (dbID >> 8) & 0xFF;
		buf[11] = dbID & 0xFF;

		return MurmurHash::hash( buf, sizeof( buf ), 0xdeadbeef );
	}


	/**
	 *	This method populates test data map suitable for schemes using this
	 *	algorithm.
	 */
	static void populateTestData( TestData & map )
	{
		map.clear();

		map.insert( std::make_pair( 1, ADDR_1 ) );
		map.insert( std::make_pair( 3, ADDR_3 ) );
		map.insert( std::make_pair( 4, ADDR_4 ) );
		map.insert( std::make_pair( 6, ADDR_6 ) );
		map.insert( std::make_pair( 7, ADDR_7 ) );
	}

};


/**
 *	This class is used for RendezvousHashSchemes involving string name buckets.
 */
template< typename MAPPED_TYPE, 
		typename BUCKET_MAP = BW::map< BW::string, MAPPED_TYPE > >
class NameDatabaseIDHashAlgorithm
{
public:
	// The hashing scheme that uses this algorithm.
	typedef RendezvousHashSchemeT< 
			DatabaseID, 
			BW::string,
			MAPPED_TYPE,
			NameDatabaseIDHashAlgorithm< MAPPED_TYPE, BUCKET_MAP >,
			BUCKET_MAP >
		HashScheme;

	// Test data to be used with this algorithm.
	typedef BW::map< BW::string, MAPPED_TYPE > TestData;

	// The output type of the hash. Required for use with
	// RendezvousHashSchemeT.
	typedef uint64 Value;


	/**
	 *	This method implements the hashing for a bucket name and the key.
	 */
	Value hash( const BW::string & name, DatabaseID dbID ) const
	{
		unsigned char buf[name.size() + sizeof( dbID )];

		buf[0] = (dbID >> 56) & 0xFF;
		buf[1] = (dbID >> 48) & 0xFF;
		buf[2] = (dbID >> 40) & 0xFF;
		buf[3] = (dbID >> 32) & 0xFF;
		buf[4] = (dbID >> 24) & 0xFF;
		buf[5] = (dbID >> 16) & 0xFF;
		buf[6] = (dbID >> 8) & 0xFF;
		buf[7] = dbID & 0xFF;

		memcpy( buf + sizeof( dbID ), name.data(), name.size() );

		return MurmurHash::hash( buf, sizeof( buf ), 0xdeadbeef );
	}


	/**
	 *	This method populates test data map suitable for schemes using this
	 *	algorithm.
	 */
	static void populateTestData( TestData & map )
	{
		map.clear();

		map.insert( std::make_pair( "blue", ADDR_BLUE) );
		map.insert( std::make_pair( "red", ADDR_RED ) );
		map.insert( std::make_pair( "green", ADDR_GREEN ) );
		map.insert( std::make_pair( "yellow", ADDR_YELLOW ) );
		map.insert( std::make_pair( "purple", ADDR_PURPLE ) );
	}
};


/**
 *	This class is a template class for test fixtures to be used for unit tests.
 */
template< typename ALGORITHM >
class RendezvousHashSchemeTestFixture
{
public:
	typedef typename ALGORITHM::HashScheme HashScheme;

	static const size_t NUM_ITERATIONS = 10000;

	RendezvousHashSchemeTestFixture();

	void runAllTests( TestResult & result, const char * name );

	void basicTest( TestResult & result, const char * name );
	void removalTest( TestResult & result, const char * name );
	void streamingTest( TestResult & result, const char * name );

	double calculateMeanAbsoluteDeviationRatio( const char * name,
		size_t numToTest = NUM_ITERATIONS,
		typename HashScheme::Key start = typename HashScheme::Key() ) const;


private:
	HashScheme hashScheme_;

	typename ALGORITHM::TestData testData_;

	static const double MAD_MAX_RATIO;
};

// Arbitrary choice of acceptable mean absolute deviation to average ratio.
// (Similar to the ratio of good to bad Mel Gibson movies)
template< typename ALGORITHM >
const double RendezvousHashSchemeTestFixture< ALGORITHM >::MAD_MAX_RATIO = 0.05;


/**
 *	Constructor.
 */
template< typename ALGORITHM >
RendezvousHashSchemeTestFixture< ALGORITHM >::
			RendezvousHashSchemeTestFixture() :
		hashScheme_(),
		testData_()
{
	ALGORITHM::populateTestData( testData_ );
}


/**
 *	This method runs all the tests.
 *
 *	@param result 		Required parameter for the CppUnitLite2 macros.
 *	@param name 		Required parameter for the CppUnitLite2 macros.
 */
template <typename ALGORITHM >
void RendezvousHashSchemeTestFixture< ALGORITHM >::runAllTests( 
		TestResult & result, const char * name )
{
	this->basicTest( result, name );
	this->removalTest( result, name );
	this->streamingTest( result, name );
}


/**
 *	This method performs some basic tests of the functionality of the
 *	rendezvous hash scheme.
 *
 *	@param result_ 		Required parameter for the CppUnitLite2 macros.
 *	@param name_ 		Required parameter for the CppUnitLite2 macros.
 */
template< typename ALGORITHM >
void RendezvousHashSchemeTestFixture< ALGORITHM >::basicTest( 
		TestResult & result_, const char * m_name )
{
	typename ALGORITHM::TestData::const_iterator begin = testData_.begin();
	typename ALGORITHM::TestData::const_iterator end = testData_.end();
	typename ALGORITHM::TestData::const_iterator iter = begin;

	MF_ASSERT( std::distance( begin, end ) >= 1 );

	CHECK_EQUAL( 0U, hashScheme_.size() );
	CHECK( hashScheme_.empty() );

	typename HashScheme::ValueType firstPair = *iter++;
	std::pair< typename HashScheme::iterator, bool > insertResult =
		hashScheme_.insert( firstPair );

	// Check insertion the first time.
	CHECK_EQUAL( 1U, hashScheme_.size() );
	CHECK( !hashScheme_.empty() );
	CHECK( insertResult.second );
	CHECK( insertResult.first->first == firstPair.first );
	CHECK( hashScheme_.find( firstPair.first ) != hashScheme_.end() );
	CHECK( *hashScheme_.find( firstPair.first ) == firstPair );

	// Check insertion a second time for the same bucket.
	CHECK( !hashScheme_.insert( firstPair ).second );
	CHECK_EQUAL( 1U, hashScheme_.size() );
	CHECK( !hashScheme_.empty() );

	// Erase a bucket that's in the collection.
	CHECK( hashScheme_.erase( firstPair.first ) );
	CHECK( hashScheme_.empty() );
	CHECK_EQUAL( 0U, hashScheme_.size() );
	CHECK( hashScheme_.find( firstPair.first ) == hashScheme_.end() );

	// Erase a bucket that's now not in the collection.
	CHECK( !hashScheme_.erase( firstPair.first ) );

	// Add it back in for the rest of the tests.
	hashScheme_.insert( firstPair );

	// Insert the rest of the input data - check the size is now the entire
	// input.
	hashScheme_.insert( iter, end );
	CHECK_EQUAL( size_t( std::distance( begin, end ) ), hashScheme_.size() );

	// Insert all the input data again - should be no increase in size.
	hashScheme_.insert( begin, end );
	CHECK_EQUAL( size_t( std::distance( begin, end ) ), hashScheme_.size() );

	// Check balance.
	CHECK( this->calculateMeanAbsoluteDeviationRatio( m_name ) < 
		MAD_MAX_RATIO );

	for (typename HashScheme::const_iterator checkIter = hashScheme_.begin();
			checkIter != hashScheme_.end();
			++checkIter)
	{
		typename ALGORITHM::TestData::const_iterator findIter =
			testData_.find( checkIter->first );

		CHECK( findIter != testData_.end() );
		CHECK( findIter->first == checkIter->first );
		CHECK( findIter->second == checkIter->second );
	}

	for (iter = begin; iter != end; ++iter)
	{
		typename HashScheme::const_iterator findIter = 
			hashScheme_.find( iter->first );

		CHECK( findIter != hashScheme_.end() );
		CHECK( findIter->first == iter->first );
		CHECK( findIter->second == iter->second );
		CHECK( hashScheme_.mappedValueForBucket( iter->first ) == 
			iter->second );
		CHECK_EQUAL( 1U, hashScheme_.count( iter->first ) );
	}

	// Check clearing.
	hashScheme_.clear();
	CHECK_EQUAL( 0U, hashScheme_.size() );
	CHECK( hashScheme_.empty() );
}


/**
 *	This method tests removal of buckets from the hash.
 *
 *	@param result_ 		Required parameter for the CppUnitLite2 macros.
 *	@param name_ 		Required parameter for the CppUnitLite2 macros.
 */
template< typename ALGORITHM>
void RendezvousHashSchemeTestFixture< ALGORITHM >::removalTest(
		TestResult & result_, const char * m_name )
{
	hashScheme_.insert( testData_.begin(), testData_.end() );
	CHECK_EQUAL( testData_.size(), hashScheme_.size() );

	HashScheme oldHash = hashScheme_;

	// Choose the second in the list to remove.
	MF_ASSERT( testData_.size() >= 2 );
	typename ALGORITHM::TestData::const_iterator removeIter = 
		++testData_.begin();
	typename HashScheme::Bucket removeBucket = removeIter->first;

	CHECK( hashScheme_.erase( removeBucket ) );
	CHECK_EQUAL( testData_.size() - 1, hashScheme_.size() );
	CHECK_EQUAL( 0U, hashScheme_.count( removeBucket ) );
	CHECK( hashScheme_.find( removeBucket ) == hashScheme_.end() );
	CHECK_EQUAL( testData_.size(), oldHash.size() );

	// Check only the IDs for the removed bucket have moved.
	for (DatabaseID key = 0; static_cast< size_t >( key ) < NUM_ITERATIONS; ++key)
	{
		if (oldHash.bucketForKey( key ) != removeBucket)
		{
			CHECK( oldHash.bucketForKey( key ) ==
				hashScheme_.bucketForKey( key ) );
		}
		else
		{
			CHECK( hashScheme_.bucketForKey( key ) != removeBucket );
		}
	}
}


/**
 *	This method tests streaming of hash objects between BinaryIStreams and
 *	BinaryOStreams.
 *
 *	@param result_ 		Required parameter for the CppUnitLite2 macros.
 *	@param name_ 		Required parameter for the CppUnitLite2 macros.
 */
template< typename ALGORITHM>
void RendezvousHashSchemeTestFixture< ALGORITHM >::streamingTest(
		TestResult & result_, const char * m_name )
{
	MemoryOStream stream;

	stream << hashScheme_;

	HashScheme outHash;

	stream >> outHash;

	CHECK( !stream.error() );
	if (stream.error())
	{
		return;
	}

	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( hashScheme_ == outHash );
	CHECK_EQUAL( hashScheme_.size(), outHash.size() );

	for (typename HashScheme::const_iterator iter = outHash.begin();
			iter != outHash.end();
			++iter)
	{
		CHECK_EQUAL( 1U, hashScheme_.count( iter->first ) );
		typename HashScheme::const_iterator findIter =
			hashScheme_.find( iter->first );

		CHECK( findIter != hashScheme_.end() );
		CHECK( findIter->first == iter->first );
		CHECK( findIter->second == iter->second );
	}

	for (typename HashScheme::const_iterator iter = hashScheme_.begin();
			iter != hashScheme_.end();
			++iter)
	{
		CHECK_EQUAL( 1U, outHash.count( iter->first ) );
		typename HashScheme::const_iterator findIter =
			outHash.find( iter->first );

		CHECK( findIter != outHash.end() );
		CHECK( findIter->first == iter->first );
		CHECK( findIter->second == iter->second );
	}

	// Check some keys.
	for (DatabaseID key = 0; 
			static_cast< size_t >( key ) < NUM_ITERATIONS;
			++key)
	{
		CHECK( hashScheme_.bucketForKey( key ) == outHash.bucketForKey( key ) );
		CHECK( hashScheme_.valueForKey( key ) == 
			outHash.valueForKey( key ) );
		CHECK( hashScheme_[ key ] == outHash[ key ] );
		CHECK( hashScheme_[ key ] == outHash.mappedValueForKey( key ) );
	}
}


/**
 *	This function analyses the key counts for the hash's buckets for a given
 *	number of test keys, and calculates the ratio of the mean absolute
 *	deviation of counts to the average number per bucket, which is a measure of
 *	how balanced the buckets are.
 *
 *	The lower this is, the better. 
 *
 *	@param testName 	The name of the test, usually taken from the m_name
 *						member of the fixture.
 *	@param hash 		The hash scheme to evaluate against.
 *	@param numToTest 	The number of keys to check.
 *	@param start 		The initial key to check.
 */
template< typename ALGORITHM >
double RendezvousHashSchemeTestFixture< ALGORITHM >::
		calculateMeanAbsoluteDeviationRatio( const char * testName,
			size_t numToTest /* = NUM_ITERATIONS */,
			typename RendezvousHashSchemeTestFixture::HashScheme::Key start 
				/* = Key() */ ) const
{
	typedef BW::map< typename HashScheme::Bucket, uint > Counts;

	Counts counts;
	typename HashScheme::Key key = start;

	TimeStamp startTime( timestamp() );
	for (size_t i = 0; i < numToTest; ++i)
	{
		++counts[ hashScheme_.bucketForKey( key ) ];
		++key;
	}
	HACK_MSG( "%s: Tested %" PRIzu " keys in %.03fms\n", 
		testName, numToTest, startTime.ageInSeconds() * 1000.0 );

	double sumAbsoluteDeviation = 0;

	const double average = double( numToTest ) / hashScheme_.size();

	for (typename Counts::const_iterator iter = counts.begin();
			iter != counts.end();
			++iter)
	{
		BW::ostringstream out;
		out << iter->first;
		const double deviation = double( iter->second ) - average;

		HACK_MSG( "%s: Bucket: %s: %u (%.03lf)\n", 
			testName, out.str().c_str(), 
			iter->second, deviation );

		sumAbsoluteDeviation += std::fabs( deviation );
	}

	const double meanAbsoluteDeviation = sumAbsoluteDeviation /
		hashScheme_.size();

	HACK_MSG( "%s: meanAbsoluteDeviation = %.03lf\n",
		testName, meanAbsoluteDeviation );
	HACK_MSG( "%s: meanAbsoluteDeviation ratio = %.03lf\n",
		testName, meanAbsoluteDeviation / average );

	return meanAbsoluteDeviation / average;
}


// Hash algorithm for AppIDAndDefaultMap test.
typedef RendezvousHashSchemeTestFixture< 
		AppIDDatabaseIDHashAlgorithm< Mercury::Address > > 
	AppIDAndDefaultMapTestFixture;
/**
 *	This test runs through tests defined on the template fixture for hash
 *	schemes with AppIDs as the bucket type, and using BW::map.
 */
TEST_F( AppIDAndDefaultMapTestFixture, AppIDAndDefaultMap )
{
	this->runAllTests( result_, m_name );
}


// Hash algorithm for NameStringsAndDefaultMap test.
typedef RendezvousHashSchemeTestFixture< 
		NameDatabaseIDHashAlgorithm< Mercury::Address > > 
	NameAndDefaultMapTestFixture;
/**
 *	This test runs through tests defined on the template fixture for hash
 *	schemes with BW::string as the bucket type, and using BW::map.
 */
TEST_F( NameAndDefaultMapTestFixture, NameStringsAndDefaultMap )
{
	this->runAllTests( result_, m_name );
}


// Hash algorithm for NameStringsAndStringMap test.
typedef RendezvousHashSchemeTestFixture< 
		NameDatabaseIDHashAlgorithm< Mercury::Address > > 
	NameAndStringMapTestFixture;
/**
 *	This test runs through tests defined on the template fixture for hash
 *	schemes with BW::string as the bucket type, and using StringMap.
 */
TEST_F( NameAndStringMapTestFixture, NameStringsAndStringMap )
{
	this->runAllTests( result_, m_name );
}


BW_END_NAMESPACE

// test_rendezvous_hash_scheme.cpp
