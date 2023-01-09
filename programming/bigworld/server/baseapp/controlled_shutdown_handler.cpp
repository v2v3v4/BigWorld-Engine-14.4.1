#include "script/first_include.hpp"

#include "controlled_shutdown_handler.hpp"

#include "base.hpp"
#include "baseappmgr_gateway.hpp"
#include "bases.hpp"
#include "baseapp.hpp"
#include "replay_data_file_writer.hpp"
#include "sqlite_database.hpp"

#include "server/writedb.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

namespace
{

// -----------------------------------------------------------------------------
// Section: ControlledShutDownHandler
// -----------------------------------------------------------------------------

/**
 *	This class is used to handle shutting down in a controlled way. It writes
 *	the entities that have a database entry (i.e. non-zero databaseID) to the
 *	database. It only writes a few entities to the database at a time so that
 *	the database does not get flooded. It also finalises any active replay file
 *	data writers.
 */
class ControlledShutDownHandler : public WriteToDBReplyHandler
{
public:
	ControlledShutDownHandler();
	void init( const Bases & bases,
			Mercury::ReplyID replyID,
			const Mercury::Address & srcAddr );

	virtual ~ControlledShutDownHandler() {};

	void checkFinished();

protected:
	virtual void onInit( const Bases & bases ) = 0;
	int numOutstanding_;

private:
	Mercury::ReplyID replyID_;
	Mercury::Address srcAddr_;
};


/**
 *	Constructor.
 */
ControlledShutDownHandler::ControlledShutDownHandler() :
	numOutstanding_( 0 ),
	replyID_( 0 ),
	srcAddr_()
{
}


/**
 *	This method initialises this handler. The real work is done in the derived
 *	classes.
 */
void ControlledShutDownHandler::init( const Bases & bases,
		Mercury::ReplyID replyID,
		const Mercury::Address & srcAddr )
{
	replyID_ = replyID;
	srcAddr_ = srcAddr;

	this->onInit( bases );

	ReplayDataFileWriter::closeAll( NULL, /* shouldFinalise */ true );

	this->checkFinished();
}


/**
 *	This method checks whether the shutdown has finished. If it has, this object
 *	is deleted.
 */
void ControlledShutDownHandler::checkFinished()
{
	if (numOutstanding_ != 0)
	{
		return;
	}

	BaseApp * pApp = BaseApp::pInstance();

	if (pApp == NULL)
	{
		ERROR_MSG( "ControlledShutDownHandler::checkFinished: pApp is NULL\n" );
		return;
	}

	BaseAppMgrGateway & baseAppMgr = pApp->baseAppMgr();

	IF_NOT_MF_ASSERT_DEV( srcAddr_ == baseAppMgr.addr() )
	{
		return;
	}

	baseAppMgr.bundle().startReply( replyID_ );
	baseAppMgr.send();

	pApp->callShutDownCallback( 2 );

	delete this;
	pApp->shutDown();
}


// -----------------------------------------------------------------------------
// Section: With secondary db
// -----------------------------------------------------------------------------

/**
 *	This handler is used when there is a secondary database.
 */
class ShutDownHandlerWithSecondaryDB : public ControlledShutDownHandler
{
public:
	ShutDownHandlerWithSecondaryDB( SqliteDatabase & secondaryDB ) :
		secondaryDB_( secondaryDB )
	{
	}

protected:
	virtual void onWriteToDBComplete( bool /*succeeded*/ );
	virtual void onInit( const Bases & bases );

private:
	SqliteDatabase & secondaryDB_;
};


/**
 *	This method writes all of the entities to the secondary database.
 */
void ShutDownHandlerWithSecondaryDB::onInit( const Bases & bases )
{
	Bases::const_iterator iter = bases.begin();

	while (iter != bases.end())
	{
		Base & currentBase = *iter->second;

		// Increment now as during retirement writeToDB may erase the base
		++iter;

		if (currentBase.hasWrittenToDB())
		{
			currentBase.writeToDB( WRITE_BASE_CELL_DATA, this );
			++numOutstanding_;
		}
	}

	secondaryDB_.commit();
}


/**
 *	This method is called when an entity has finished writing itself.
 */
void ShutDownHandlerWithSecondaryDB::onWriteToDBComplete( bool /*succeeded*/ )
{
	--numOutstanding_;
	this->checkFinished();
}


// -----------------------------------------------------------------------------
// Section: Without secondary db
// -----------------------------------------------------------------------------

/**
 *	This handler is used when there is not a secondary database.
 */
class ShutDownHandlerWithoutSecondaryDB : public ControlledShutDownHandler
{
protected:
	virtual void onWriteToDBComplete( bool succeeded );
	virtual void onInit( const Bases & bases );

private:
	BW::vector< BasePtr > bases_;
};


/**
 *	This method writes all of the entities to the database. It does a few at a
 *	time.
 */
void ShutDownHandlerWithoutSecondaryDB::onInit( const Bases & bases )
{
	// TODO: This could be made configurable
	const int MAX_OUTSTANDING = 5;
	Bases::const_iterator iter = bases.begin();
	int leftSize = bases.size();

	while ((numOutstanding_ < MAX_OUTSTANDING) && (iter != bases.end()))
	{
		Base & currentBase = *iter->second;

		// Increment now as during retirement writeToDB may erase the base
		++iter;

		if (currentBase.hasWrittenToDB())
		{
			if (currentBase.writeToDB( WRITE_BASE_CELL_DATA, this ))
			{
				++numOutstanding_;
			}
		}

		--leftSize;
	}

	if (iter != bases.end())
	{
		MF_ASSERT( MAX_OUTSTANDING == numOutstanding_ );
		bases_.resize( leftSize );

		for (int i = 0; i < leftSize; ++i)
		{
			bases_[i] = iter->second;
			++iter;
		}

		MF_ASSERT( iter == bases.end() );
	}
}


/**
 *	This method is called when an entity has finished writing itself.
 */
void ShutDownHandlerWithoutSecondaryDB::onWriteToDBComplete( bool succeeded )
{
	if (!succeeded)
	{
		--numOutstanding_;
		this->checkFinished();
		return;
	}
	
	// Once an entity has been written to the database, we try to write
	// another one to take its place.

	bool isOkay = true;

	do
	{
		isOkay = true;

		if (!bases_.empty())
		{
			if (bases_.back()->isDestroyed() ||
				!bases_.back()->hasWrittenToDB() ||
				!bases_.back()->writeToDB( WRITE_BASE_CELL_DATA, this ))
			{
				isOkay = false;
			}

			bases_.pop_back();
		}
		else
		{
			--numOutstanding_;
			this->checkFinished();
		}
	}
	while (!isOkay);
}

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: ShutDownHandlerWithoutSecondaryDB
// -----------------------------------------------------------------------------

namespace ControlledShutdown
{

void start( SqliteDatabase * pSecondaryDB,
			const Bases & bases,
			Bases & localServiceFragments,
			Mercury::ReplyID replyID,
			const Mercury::Address & srcAddr )
{
	localServiceFragments.discardAll();

	// This object deletes itself.
	ControlledShutDownHandler * pHandler = NULL;

	if (pSecondaryDB)
	{
		pHandler = new ShutDownHandlerWithSecondaryDB( *pSecondaryDB );
	}
	else
	{
		pHandler = new ShutDownHandlerWithoutSecondaryDB();
	}
	pHandler->init( bases, replyID, srcAddr );
}

} // namespace ControlledShutdown

BW_END_NAMESPACE

// controlled_shutdown_handler.cpp
