#include "CppUnitLite2/src/CppUnitLite2.h"

#include "cstdmf/debug.hpp"
#include "cstdmf/shared_ptr.hpp"

#include "../updatables.hpp"
#include "../updatable.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

namespace
{

class MyUpdatable;

class Tester
{
public:
	virtual void visit( MyUpdatable * pObj ) = 0;
	virtual bool isOkay() const	= 0;
};

Tester * g_pCurrentTester = NULL;


class MyUpdatable : public Updatable
{
public:
	MyUpdatable( int index ) :
		index_( index )
	{
		s_selfDelete.insert( shared_ptr< MyUpdatable >( this ) );
	}

	int index() const	{ return index_; }

private:
	virtual void update()
	{
		g_pCurrentTester->visit( this );
	}

	int index_;

	static BW::set< shared_ptr< MyUpdatable > > s_selfDelete;
};

BW::set< shared_ptr< MyUpdatable > > MyUpdatable::s_selfDelete;


// -----------------------------------------------------------------------------
// Section: Testers
// -----------------------------------------------------------------------------

class SimpleTester : public Tester
{
public:
	SimpleTester() :
		isOkay_( true ),
		lastVisit_( -1 ),
		numCalls_( 0 )
	{
	}

	virtual bool isOkay() const
	{
		return isOkay_;
	}

	virtual void visit( MyUpdatable * pObj )
	{
		isOkay_ &= (lastVisit_ <= pObj->index());
		lastVisit_ = pObj->index();
		++numCalls_;
	}

	int numCalls() const	{ return numCalls_; }

private:
	bool isOkay_;
	int lastVisit_;
	int numCalls_;
};


class AddTester : public SimpleTester
{
public:
	AddTester( Updatables & updatables ) :
		updatables_( updatables ),
		shouldAdd_( false )
	{
	}

private:
	virtual void visit( MyUpdatable * pObj )
	{
		if (shouldAdd_)
		{
			updatables_.add( new MyUpdatable( pObj->index() ), pObj->index() );
		}

		shouldAdd_ = !shouldAdd_;

		SimpleTester::visit( pObj );
	}

	Updatables & updatables_;
	bool shouldAdd_;
};

class DelTester : public SimpleTester
{
public:
	DelTester( Updatables & updatables ) :
		updatables_( updatables ),
		shouldRemove_( true )
	{
	}

private:
	virtual void visit( MyUpdatable * pObj )
	{
		if (shouldRemove_)
		{
			updatables_.remove( pObj );
		}

		shouldRemove_ = !shouldRemove_;

		SimpleTester::visit( pObj );
	}

	Updatables & updatables_;
	bool shouldRemove_;
};

TEST( Updatables )
{
	const int NUM_TO_TEST = 21;
	const int NUM_LEVELS = 3;

	Updatables updatables( NUM_LEVELS );

	for (int i = 0; i < NUM_TO_TEST; ++i)
	{
		int level = i % NUM_LEVELS;
		updatables.add( new MyUpdatable( level ), level );
	}

	SimpleTester simpleTester;
	g_pCurrentTester = &simpleTester;

	updatables.call();
	CHECK( simpleTester.isOkay() );
	CHECK_EQUAL( size_t( simpleTester.numCalls() ), updatables.size() );

	AddTester addTester( updatables );
	g_pCurrentTester = &addTester;

	int origSize = updatables.size();
	updatables.call();
	CHECK( addTester.isOkay() );
	CHECK_EQUAL( addTester.numCalls(), origSize );
	CHECK_EQUAL( 3 * origSize / 2, (int) updatables.size() );

	DelTester delTester( updatables );
	g_pCurrentTester = &delTester;
	origSize = updatables.size();
	updatables.call();

	CHECK( delTester.isOkay() );
	CHECK_EQUAL( delTester.numCalls(), origSize );
	CHECK_EQUAL( origSize / 2, (int) updatables.size() );
}

} // unnamed namespace

BW_END_NAMESPACE

// test_updatables.cpp
