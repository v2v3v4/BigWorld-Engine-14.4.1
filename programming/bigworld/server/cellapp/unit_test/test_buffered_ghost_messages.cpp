#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/shared_ptr.hpp"

#include "../buffered_ghost_messages.hpp"
#include "../buffered_ghost_message.hpp"

#include <limits.h>


BW_BEGIN_NAMESPACE

namespace
{

typedef int SeqNum;
class TestApp;

typedef BW::list< Mercury::Address > Addresses;


// -----------------------------------------------------------------------------
// Section: MockEntity
// -----------------------------------------------------------------------------

/**
 *	This class is a fake Entity class.
 */
class MockEntity
{
public:
	MockEntity( EntityID id,
			const Mercury::Address & realAddr,
			SeqNum seqNum ) :
		id_( id ),
		realAddr_( realAddr ),
		nextRealAddr_( 0, 0 ),
		seqNum_( seqNum )
	{
	}

	SeqNum seqNum() const		{ return seqNum_; }
	void seqNum( SeqNum num )	{ seqNum_ = num; }

	const Mercury::Address & realAddr() const	{ return realAddr_; }

	void ghostSetReal( const Mercury::Address & addr,
			SeqNum seqNum, TestApp & app );

	void ghostSetNextReal( const Mercury::Address & addr, TestApp & app );

	/**
	 *	Returns where to expect the next message from the real entity to come
	 *	from.
	 */
	const Mercury::Address & addrForMessagesFromReal() const
	{
		if (nextRealAddr_ != Mercury::Address::NONE)
		{
			return nextRealAddr_;
		}

		return realAddr_;
	}

private:
	EntityID id_;
	Mercury::Address realAddr_;
	Mercury::Address nextRealAddr_;
	SeqNum seqNum_;
};


// -----------------------------------------------------------------------------
// Section: MockEntities
// -----------------------------------------------------------------------------

/**
 *	This class maintains a collection of fake entities.
 */
class MockEntities
{
public:
	MockEntities() : numEntitiesEver_( 0 )
	{
	}

	void create( EntityID entityID,
			const Mercury::Address & realAddr,
			SeqNum seqNum )
	{
		MF_ASSERT( map_.find( entityID ) == map_.end() );
		map_[ entityID ] = new MockEntity( entityID, realAddr, seqNum );

		++numEntitiesEver_;
	}

	void destroy( EntityID entityID, BufferedGhostMessages & bufferedMessages )
	{
		Map::iterator iter = map_.find( entityID );

		MF_ASSERT( iter != map_.end() );

		delete iter->second;
		map_.erase( iter );

		bufferedMessages.playNewLifespanFor( entityID );
	}

	MockEntity * find( EntityID entityID ) const
	{
		Map::const_iterator iter = map_.find( entityID );

		return (iter != map_.end()) ? iter->second : NULL;
	}

	bool isEmpty() const
	{
		return map_.empty();
	}

	size_t numEntitiesEver() const	{ return numEntitiesEver_; }

private:
	typedef BW::map< EntityID, MockEntity * > Map;
	Map map_;

	size_t numEntitiesEver_;
};


// -----------------------------------------------------------------------------
// Section: TestApp
// -----------------------------------------------------------------------------

class TestApp
{
public:
	BufferedGhostMessages & bufferedMessages()
	{
		return bufferedMessages_;
	}

	MockEntities & entities()
	{
		return entities_;
	}

	/**
	 *	This method simulates a message being received from the network.
	 */
	void deliver( EntityID entityID,
			const Mercury::Address & srcAddr,
			BufferedGhostMessage * pMsg )
	{
		bool shouldPlay = false;

		if (pMsg->isCreateGhostMessage())
		{
			shouldPlay = !bufferedMessages_.hasMessagesFor( entityID, srcAddr );
		}
		else
		{
			MockEntity * pEntity = entities_.find( entityID );

			shouldPlay = pEntity &&
				// From the correct address
				(pEntity->addrForMessagesFromReal() == srcAddr) &&
				// and not delaying messages
				!bufferedMessages_.isDelayingMessagesFor( entityID, srcAddr );
		}

		if (shouldPlay)
		{
			// DEBUG_MSG( "Playing %p\n", pMsg );
			pMsg->play();
			delete pMsg;
		}
		else
		{
			// DEBUG_MSG( "Buffering %p\n", pMsg );
			bufferedMessages_.add( entityID, srcAddr, pMsg );
		}
	}

private:
	BufferedGhostMessages bufferedMessages_;
	MockEntities entities_;
};


// -----------------------------------------------------------------------------
// Section: Mock Messages
// -----------------------------------------------------------------------------

/**
 *	Common base class for the Mock messages.
 */
class GhostMessage : public BufferedGhostMessage
{
public:
	GhostMessage( TestApp & app,
			EntityID entityID,
			bool isSubsequenceStart, bool isSubsequenceEnd ) :
		BufferedGhostMessage( isSubsequenceStart, isSubsequenceEnd ),
		app_( app ),
		entityID_( entityID )
	{
	}

	EntityID entityID() const	{ return entityID_; }

protected:
	TestApp & app_;
	EntityID entityID_;
};


/**
 *	Simulates CellAppInterface::createGhost - the first message of a lifespan.
 */
class CreateGhostMessage : public GhostMessage
{
public:
	CreateGhostMessage( TestApp & app, EntityID id,
			const Mercury::Address & srcAddr, SeqNum num ) :
		GhostMessage( app, id, true, false ),
		srcAddr_( srcAddr ),
		seqNum_( num )
	{
		DEBUG_MSG( "CreateGhostMessage::CreateGhostMessage( %p ): "
					"id = %d. srcAddr = %s. seqNum = %d\n",
				this, entityID_, srcAddr_.c_str(), seqNum_ );
	}

	virtual void play()
	{
		DEBUG_MSG( "CreateGhostMessage::play( %p ): "
					"id = %d. srcAddr = %s. seqNum = %d\n",
				this, entityID_, srcAddr_.c_str(), seqNum_ );

		if (app_.entities().find( entityID_ ))
		{
			app_.bufferedMessages().delaySubsequence( entityID_, srcAddr_,
				new CreateGhostMessage( app_, entityID_, srcAddr_, seqNum_ ) );
		}
		else
		{
			app_.entities().create( entityID_, srcAddr_, seqNum_ );
		}
	}

	virtual bool isCreateGhostMessage() const { return true; }

private:
	const Mercury::Address srcAddr_;
	SeqNum seqNum_;
};


/**
 *	Simulates CellAppInterface::delGhost - the last message of a lifespan.
 */
class DelGhostMessage : public GhostMessage
{
public:
	DelGhostMessage( TestApp & app, EntityID id ) :
		GhostMessage( app, id, false, true )
	{
		DEBUG_MSG( "DelGhostMessage::DelGhostMessage( %p ): id = %d\n",
				this, entityID_ );
	}

	virtual void play()
	{
		DEBUG_MSG( "DelGhostMessage::play( %p ): id = %d\n",
				this, entityID_ );
		app_.entities().destroy( entityID_, app_.bufferedMessages() );
	}
};


/**
 *	Simulates CellAppInterface::ghostSetReal - the first message of a
 *	subsequence.
 */
class GhostSetRealMessage : public GhostMessage
{
public:
	GhostSetRealMessage( TestApp & app, EntityID id,
			const Mercury::Address & srcAddr, SeqNum seqNum ) :
		GhostMessage( app, id, true, false ),
		srcAddr_( srcAddr ),
		seqNum_( seqNum )
	{
		DEBUG_MSG( "GhostSetRealMessage::GhostSetRealMessage( %p ): "
					"id = %u. srcAddr = %s. seqNum = %d\n",
				this, entityID_, srcAddr_.c_str(), seqNum_ );
	}

	virtual void play()
	{
		DEBUG_MSG( "GhostSetRealMessage::play( %p ): "
					"id = %u. srcAddr = %s. seqNum = %d\n",
				this, entityID_, srcAddr_.c_str(), seqNum_ );
		app_.entities().find( entityID_ )->ghostSetReal(
				srcAddr_, seqNum_, app_ );
	}

private:
	Mercury::Address srcAddr_;
	SeqNum seqNum_;
};


/**
 *	Simulates CellAppInterface::ghostSetNextReal - the last message of a
 *	subsequence.
 */
class GhostSetNextRealMessage : public GhostMessage
{
public:
	GhostSetNextRealMessage( TestApp & app, EntityID id,
			const Mercury::Address & nextAddr ) :
		GhostMessage( app, id, false, true ),
		nextAddr_( nextAddr )
	{
		DEBUG_MSG( "GhostSetNextRealMessage::GhostSetNextRealMessage( %p ): "
					"id = %u. nextAddr = %s\n",
				this, entityID_, nextAddr_.c_str() );
	}

	virtual void play()
	{
		DEBUG_MSG( "GhostSetNextRealMessage::play( %p ): "
					"id = %u. nextAddr = %s\n",
				this, entityID_, nextAddr_.c_str() );
		app_.entities().find( entityID_ )->ghostSetNextReal( nextAddr_, app_ );
	}

private:
	Mercury::Address nextAddr_;
};


// -----------------------------------------------------------------------------
// Section: Messages
// -----------------------------------------------------------------------------

/**
 *	This class contains the messages that are to be delivered to an app.
 */
class Messages
{
public:
	/**
	 *	This method creates a set of messages that simulate what a ghost would
	 *	receive if the real entity travelled to the CellApps given.
	 */
	void addLifespan( TestApp & app,
			EntityID id, const Addresses & reals, SeqNum & seqNum )
	{
		Addresses::const_iterator iter = reals.begin();

		while (iter != reals.end())
		{
			Mercury::Address addr = *iter;
			List & currList = map_[ *iter ];
			const bool isLifespanStart = (iter == reals.begin());
			++iter;
			const bool isLifespanEnd = (iter == reals.end());

			if (isLifespanStart)
			{
				currList.push_back(
					new CreateGhostMessage( app, id, addr, seqNum ) );
			}
			else
			{
				currList.push_back(
					new GhostSetRealMessage( app, id, addr, seqNum ) );
			}

			if (isLifespanEnd)
			{
				currList.push_back( new DelGhostMessage( app, id ) );
			}
			else
			{
				currList.push_back(
					new GhostSetNextRealMessage( app, id, *iter ) );
			}

			++seqNum;
		}
	}


	/**
	 *	This method delivers messages from srcAddr to the given app.
	 */
	void deliver( const Mercury::Address & srcAddr,
			TestApp & app,
			int count = INT_MAX )
	{
		Map::iterator mapIter = map_.find( srcAddr );

		if (mapIter != map_.end())
		{
			List & messages = mapIter->second;

			List::iterator iter = messages.begin();

			while (iter != messages.end() && count > 0)
			{
				GhostMessage * pMsg = *iter;
				app.deliver( pMsg->entityID(), srcAddr, pMsg );

				++iter;
				--count;
			}

			messages.erase( messages.begin(), iter );

			if (messages.empty())
			{
				map_.erase( mapIter );
			}
		}
	}

private:
	typedef BW::list< GhostMessage * > List;
	typedef BW::map< Mercury::Address, List > Map;
	Map map_;
};


/**
 *	This class is responsible for running tests. It is set up with the ghost
 *	lifespans that want to be tested and then all permutations of message
 *	delivery are tested.
 */
class TestRunner
{
public:
	TestRunner() : numFailures_( 0 )
	{
	}

	void startLifespan( const Mercury::Address & addr )
	{
		lifespans_.push_back( Lifespan() );
		this->addToLifespan( addr );
	}

	void addToLifespan( const Mercury::Address & addr )
	{
		lifespans_.back().push_back( addr );
	}

	void testAll() const
	{
		Counts counts;
		this->calcCounts( counts );

		Addresses receiveOrder;

		this->visitPermutations( receiveOrder, counts );
	}

	int numFailures() const { return numFailures_; }

private:
	typedef BW::map< Mercury::Address, int > Counts;

	void initMessages( Messages & messages, TestApp & app, EntityID id ) const
	{
		SeqNum seqNum = 0;
		Lifespans::const_iterator iter = lifespans_.begin();

		while (iter != lifespans_.end())
		{
			messages.addLifespan( app, id, *iter, seqNum );

			++iter;
		}
	}

	/**
	 *	This method populates a map with the counts for the number of messages
	 *	expected from each address. This map is used to generate the
	 *	permutations.
	 */
	void calcCounts( Counts & counts ) const
	{
		Lifespans::const_iterator iter1 = lifespans_.begin();

		while (iter1 != lifespans_.end())
		{
			Lifespan::const_iterator iter2 = iter1->begin();

			while (iter2 != iter1->end())
			{
				// There's a subsequence start and end message for each address.
				const int numMsgs = 2;

				counts[ *iter2 ] += numMsgs;

				++iter2;
			}

			++iter1;
		}
	}


	/**
	 *	This method is used to test all permutations.
	 *
	 *	@param receiveOrder A list of address in the order that the messages
	 *		will be received.
	 *	@param counts A map containing how more messages there are from each
	 *		address.
	 */
	void visitPermutations( Addresses & receiveOrder, Counts & counts ) const
	{
		bool shouldRunTest = true;

		int i = 0;
		Counts::iterator iter = counts.begin();

		while (iter != counts.end())
		{
			if (iter->second > 0)
			{
				shouldRunTest = false;
				--(iter->second);
				receiveOrder.push_back( iter->first );

				this->visitPermutations( receiveOrder, counts );

				receiveOrder.pop_back();
				++(iter->second);
			}

			++i;
			++iter;
		}

		if (shouldRunTest)
		{
			this->test( receiveOrder );
		}
	}


	static void printAddresses( const Addresses & addrs )
	{
		Addresses::const_iterator iter = addrs.begin();

		while (iter != addrs.end())
		{
			BWUnitTest::unitTestInfo( "%s\n", iter->c_str() );

			++iter;
		}
	}

	void printTest( const Addresses & receiveOrder ) const
	{
		BWUnitTest::unitTestInfo( "Running Test:\n" );

		Lifespans::const_iterator iter = lifespans_.begin();

		while (iter != lifespans_.end())
		{
			BWUnitTest::unitTestInfo( "Lifespan:\n" );
			printAddresses( *iter );
			++iter;
		}

		BWUnitTest::unitTestInfo( "Receive order:\n" );
		printAddresses( receiveOrder );
	}


	/**
	 *	This method tests the senario where the messages are received in the
	 *	order given.
	 */
	void test( const Addresses & receiveOrder ) const
	{
		// this->printTest( receiveOrder );

		EntityID entityID = 1;
		TestApp app;
		Messages messages;
		this->initMessages( messages, app, entityID );

		Addresses::const_iterator iter = receiveOrder.begin();

		while (iter != receiveOrder.end())
		{
			messages.deliver( *iter, app, 1 );

			++iter;
		}

		int numFailures = 0;

		if (!app.entities().isEmpty())
		{
			ERROR_MSG( "TestRunner::test: There are still entities\n" );
			++numFailures;
		}

		if (app.entities().numEntitiesEver() != lifespans_.size())
		{
			ERROR_MSG( "TestRunner::test: Wrong number of entities. "
					"Expected %" PRIzd ", got %" PRIzd ".\n",
				lifespans_.size(), app.entities().numEntitiesEver() );
			++numFailures;
		}

		if (!app.bufferedMessages().isEmpty())
		{
			ERROR_MSG( "TestRunner::test: Still have buffered messages\n" );
			++numFailures;
		}

		if (numFailures)
		{
			numFailures_ += numFailures;
			this->printTest( receiveOrder );
		}
	}

private:
	typedef Addresses Lifespan;
	typedef BW::list< Lifespan > Lifespans;

	Lifespans lifespans_;
	mutable int numFailures_;
};


// -----------------------------------------------------------------------------
// Section: MockEntity implementation
// -----------------------------------------------------------------------------

/**
 *	This method hanndles ghostSetReal messages.
 */
void MockEntity::ghostSetReal( const Mercury::Address & addr,
		SeqNum seqNum, TestApp & app )
{
	if (seqNum != seqNum_ + 1 )
	{
		app.bufferedMessages().delaySubsequence( id_, addr,
				new GhostSetRealMessage( app, id_, addr, seqNum ) );
		return;
	}

	MF_ASSERT( nextRealAddr_ == addr );
	nextRealAddr_ = Mercury::Address::NONE;

	realAddr_ = addr;
	seqNum_ = seqNum;
}


/**
 *	This method hanndles ghostSetNextReal messages.
 */
void MockEntity::ghostSetNextReal( const Mercury::Address & addr,
		TestApp & app )
{
	nextRealAddr_ = addr;
	app.bufferedMessages().playSubsequenceFor( id_, nextRealAddr_ );
}


// -----------------------------------------------------------------------------
// Section: Tests
// -----------------------------------------------------------------------------

TEST( BufferedGhostMessages )
{
	TestApp app;
	SeqNum seqNum = 0;
	EntityID entityID = 1;

	Messages messages;

	Mercury::Address A( 0x01010101, 0x100 );
	Mercury::Address B( 0x02020202, 0x200 );
	Mercury::Address C( 0x03030303, 0x300 );

	Addresses first;
	first.push_back( A );
	first.push_back( B );
	messages.addLifespan( app, entityID, first, seqNum );

	Addresses second;
	second.push_back( C );
	second.push_back( B );
	messages.addLifespan( app, entityID, second, seqNum );

	messages.deliver( B, app, 1 );
	messages.deliver( C, app );
	messages.deliver( B, app );
	messages.deliver( A, app );

	CHECK( app.entities().isEmpty() );
	CHECK( app.entities().numEntitiesEver() == 2 );
}


TEST( BufferedGhostMessages2 )
{
	TestApp app;
	SeqNum seqNum = 0;
	EntityID entityID = 1;

	Messages messages;

	// Lifespans:
	// (A)
	// (BA)

	Mercury::Address A( 0x01010101, 0x100 );
	Mercury::Address B( 0x02020202, 0x200 );

	Addresses first;
	first.push_back( A );
	messages.addLifespan( app, entityID, first, seqNum );

	Addresses second;
	second.push_back( B );
	second.push_back( A );
	messages.addLifespan( app, entityID, second, seqNum );

	messages.deliver( B, app );
	messages.deliver( A, app );

	CHECK( app.entities().isEmpty() );
	CHECK( app.entities().numEntitiesEver() == 2 );
}

TEST( AllPerms )
{
	TestRunner testRunner;

	Mercury::Address A( 0x01010101, 0x100 );
	Mercury::Address B( 0x02020202, 0x200 );
	Mercury::Address C( 0x03030303, 0x300 );

	// Lifespans:
	// (AB)
	// (CB)

	testRunner.startLifespan( A );
	testRunner.addToLifespan( B );

	testRunner.startLifespan( C );
	testRunner.addToLifespan( B );

	testRunner.testAll();

	CHECK_EQUAL( 0, testRunner.numFailures() );
}

TEST( AllPerms2 )
{
	TestRunner testRunner;

	Mercury::Address A( 0x01010101, 0x100 );
	Mercury::Address B( 0x02020202, 0x200 );
	Mercury::Address C( 0x03030303, 0x300 );

	// Lifespans:
	// (C)
	// (ABA)

	testRunner.startLifespan( C );

	testRunner.startLifespan( A );
	testRunner.addToLifespan( B );
	testRunner.addToLifespan( A );

	testRunner.testAll();

	CHECK_EQUAL( 0, testRunner.numFailures() );
}

TEST( AllPerms3 )
{
	TestRunner testRunner;

	Mercury::Address A( 0x01010101, 0x100 );
	Mercury::Address B( 0x02020202, 0x200 );

	// Lifespans:
	// (A)
	// (A)
	// (BA)

	testRunner.startLifespan( A );
	testRunner.startLifespan( A );
	testRunner.startLifespan( B );
	testRunner.addToLifespan( A );

	testRunner.testAll();

	CHECK_EQUAL( 0, testRunner.numFailures() );
}


TEST( AllPerms4 )
{
	TestRunner testRunner;

	Mercury::Address A( 0x01010101, 0x100 );
	Mercury::Address B( 0x02020202, 0x200 );

	// Lifespans:
	// (BA)
	// (A)
	// (A)

	testRunner.startLifespan( B );
	testRunner.addToLifespan( A );
	testRunner.startLifespan( A );
	testRunner.startLifespan( A );

	testRunner.testAll();

	CHECK_EQUAL( 0, testRunner.numFailures() );
}

} // anonymous namespace

BW_END_NAMESPACE

// test_updatables.cpp
