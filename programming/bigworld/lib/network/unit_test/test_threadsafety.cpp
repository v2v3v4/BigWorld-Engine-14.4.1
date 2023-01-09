#include "pch.hpp"

#include "cstdmf/concurrency.hpp"

#include "network/packet.hpp"
#include "network/udp_bundle.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

namespace // anonymous
{

typedef BW::vector<SimpleThread*> Threads;

/**
 *	This function creates the given number of threads that all run the same
 *	function.
 */
void runTest( int numThreads, SimpleThreadFunc func )
{
	Threads threads;

	INFO_MSG( "runTest: creating %d threads\n", numThreads );

	for (int i = 0; i < numThreads; ++i)
	{
		threads.push_back( new SimpleThread( func, NULL ) );
	}

	for (Threads::iterator i = threads.begin();
			i != threads.end();
			++i)
	{
		delete *i;
	}

	INFO_MSG( "runTest: all threads joined\n" );
}



// Blob and addPotentialNullBlobToStream are for testing add blobs to Bundle
// and forcing fragmentation

struct Blob
{
	const char*	pBlob;
	uint32		length;

	Blob() : pBlob( NULL ), length( 0 ) {}	// NULL blob
	Blob( const char * pData, uint32 len ) : pBlob( pData ), length( len )
	{}

	bool isNull() const 	{ return (pBlob == NULL); }
};

void addPotentialNullBlobToStream( BinaryOStream& stream, const Blob& blob )
{
	if (blob.pBlob && blob.length)
	{
		stream.appendString( blob.pBlob, blob.length );
	}
	else	// NULL value or just empty string
	{
		stream.appendString( "", 0 );
		stream << uint8((blob.pBlob) ? 1 : 0);
	}
}



/**
 *	Packet test main thread function.
 */
void packetTestMain( void * arg )
{
	INFO_MSG( "packetTestMain: thread %lx started\n", pthread_self() );

	const int NUM_PACKETS = 6000;

	// allocate and deallocate packets
	// in reality, this is exercising PoolAllocator via Packet::operator new

	BW::vector<Mercury::Packet *> packets;

	// allocate two-thirds
	for (int i = 0; i < NUM_PACKETS * 2 / 3; ++i)
	{
		packets.push_back( new Mercury::Packet() );
	}

	// deallocate a third of the packets
	for (int i = 0; i < NUM_PACKETS / 3; ++i)
	{
		delete packets.front();
		packets.erase( packets.begin() );
	}

	// re-allocate a two-thirds of the packets to get back up to NUM_PACKETS
	for (int i = 0; i < NUM_PACKETS * 2 / 3; ++i)
	{
		packets.push_back( new Mercury::Packet() );
	}

	// deallocate them all
	for (int i = 0; i < NUM_PACKETS; ++i)
	{
		delete packets.front();
		packets.erase( packets.begin() );
	}

	INFO_MSG( "packetTestMain: thread %lx done\n", pthread_self() );
}


void bundleTestMain( void * arg )
{
	INFO_MSG( "bundleTestMain: thread %lx\n", pthread_self() );

	Mercury::UDPBundle bundle;

	// this emulates what happens when MySQL returns data back from an
	// executeRawDatabaseCommand request

	bundle.startReply( 0xdeadbeef );

	const unsigned int lengths[] =
		{ 5, 6, 10, 6, 0, 1, 8, 8, 8, 1, 1, 7, 100 };

	bundle << BW::string();
	bundle << uint32( 13 );
	bundle << uint32( 1000 );

	for (int j = 0; j < 1000; ++j)
	{
		for (unsigned int i = 0; i < sizeof( lengths ) / sizeof( int ); ++i)
		{
			char * buf = NULL;
			if (lengths[i])
			{
				buf = new char[lengths[i]];
			}
			memset( buf, 0x0a, lengths[i] );

			addPotentialNullBlobToStream( bundle, Blob( buf, i ) );
			delete [] buf;

		}
	}

	INFO_MSG( "TestFragment::testBundle: done (%lx)\n", pthread_self() );

}

}; // anonymous namespace


/*
// These are not really valid tests, as there are no outputs compared 
// with some expected results. A "success" is when the unit test executable
// does not crash (which is entirely non-deterministic).
TEST( ThreadSafety_testPacket )
{
	runTest( 15, packetTestMain );
}

TEST( ThreadSafety_testBundle )
{
	runTest( 15, bundleTestMain );
}
*/

BW_END_NAMESPACE

// test_threadsafety.cpp
