#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "dbapp.hpp"
#include "dbapp_config.hpp"
#include "entity_auto_loader.hpp"
#include "get_entity_handler.hpp"

#include "network/udp_bundle.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

namespace // anonymous
{


// -----------------------------------------------------------------------------
// Section: AutoLoadingEntityHandler
// -----------------------------------------------------------------------------
/**
 *	This class handles auto loading an entity from the database.
 */
class AutoLoadingEntityHandler :
							public Mercury::ShutdownSafeReplyMessageHandler,
							public GetEntityHandler,
							public IDatabase::IPutEntityHandler
{
public:
	AutoLoadingEntityHandler( EntityTypeID entityTypeID, DatabaseID dbID,
		EntityAutoLoader& mgr );
	virtual ~AutoLoadingEntityHandler()
	{
		mgr_.onAutoLoadEntityComplete( isOK_ );
	}

	void autoLoad();

	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * );
	virtual void handleException( const Mercury::NubException & ne, void * );

	// IDatabase::IGetEntityHandler/GetEntityHandler overrides
	virtual void onGetEntityCompleted( bool isOK,
		const EntityDBKey & entityKey,
		const EntityMailBoxRef * pBaseEntityLocation );

	// IDatabase::IPutEntityHandler overrides
	virtual void onPutEntityComplete( bool isOK, DatabaseID dbID );

private:
	enum State
	{
		StateInit,
		StateWaitingForSetBaseToLoggingOn,
		StateWaitingForCreateBase,
		StateWaitingForSetBaseToFinal
	};

	State				state_;
	EntityDBKey			ekey_;
	Mercury::UDPBundle	createBaseBundle_;
	EntityAutoLoader &	mgr_;
	bool				isOK_;
};


/**
 *	Constructor.
 */
AutoLoadingEntityHandler::AutoLoadingEntityHandler( EntityTypeID typeID,
		DatabaseID dbID, EntityAutoLoader & mgr ) :
	state_(StateInit),
	ekey_( typeID, dbID ),
	createBaseBundle_(),
	mgr_( mgr ),
	isOK_( true )
{}


/**
 *	Start auto-loading the entity.
 */
void AutoLoadingEntityHandler::autoLoad()
{
	// Start create new base message even though we're not sure entity exists.
	// This is to take advantage of getEntity() streaming properties into the
	// bundle directly.
	DBApp::prepareCreateEntityBundle( ekey_.typeID, ekey_.dbID,
		Mercury::Address( 0, 0 ), this, createBaseBundle_ );

	// Get entity data into bundle
	DBApp::instance().getEntity( ekey_, &createBaseBundle_, false, *this );
	// When getEntity() completes onGetEntityCompleted() is called.
}


/**
 *	Handles response from BaseAppMgr that base was created successfully.
 */
void AutoLoadingEntityHandler::handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * )
{
	Mercury::Address proxyAddr;
	data >> proxyAddr;
	EntityMailBoxRef baseRef;
	data >> baseRef;
	// Still may contain a sessionKey if it is a proxy and contains
	// latestVersion and impendingVersion from the BaseAppMgr.
	data.finish();

	state_ = StateWaitingForSetBaseToFinal;

	DBApp & dbApp = DBApp::instance();

	dbApp.setBaseEntityLocation( ekey_, baseRef, *this );
	// When completes, onPutEntityComplete() is called.

	if (dbApp.shouldCacheLogOnRecords())
	{
		dbApp.logOnRecordsCache().insert( ekey_, baseRef );
	}
}


/**
 *	Handles response from BaseAppMgr that base creation has failed.
 */
void AutoLoadingEntityHandler::handleException(
	const Mercury::NubException & ne, void * )
{
	isOK_ = false;
	delete this;
}


void AutoLoadingEntityHandler::onGetEntityCompleted( bool isOK,
		const EntityDBKey & entityKey,
		const EntityMailBoxRef * pBaseEntityLocation )
{
	ekey_ = entityKey;

	if (isOK)
	{
		state_ = StateWaitingForSetBaseToLoggingOn;
		EntityMailBoxRef	baseRef;
		DBApp::setBaseRefToLoggingOn( baseRef, ekey_.typeID );

		DBApp::instance().setBaseEntityLocation( ekey_,
				baseRef, *this );
		// When completes, onPutEntityComplete() is called.
	}
	else
	{
		ERROR_MSG( "AutoLoadingEntityHandler::onGetEntityCompleted: "
				"Failed to load entity %" FMT_DBID " of type %d\n",
				entityKey.dbID, entityKey.typeID );
		isOK_ = false;
		delete this;
	}
}


void AutoLoadingEntityHandler::onPutEntityComplete( bool isOK, DatabaseID dbID )
{
	MF_ASSERT( isOK );

	if (state_ == StateWaitingForSetBaseToLoggingOn)
	{
		state_ = StateWaitingForCreateBase;
		DBApp::instance().baseAppMgr().channel().send( &createBaseBundle_ );
	}
	else
	{
		MF_ASSERT(state_ == StateWaitingForSetBaseToFinal);
		delete this;
	}
}

} // end namespace


// -----------------------------------------------------------------------------
// Section: EntityAutoLoader
// -----------------------------------------------------------------------------
/**
 *	Constructor.
 */
EntityAutoLoader::EntityAutoLoader() :
	numOutstanding_( 0 ),
	maxOutstanding_( DBApp::Config::maxConcurrentEntityLoaders() ),
	numSent_( 0 ),
	hasErrors_( false )
{}


/**
 *	Optimisation. Reserves the correct number of entities to be auto-loaded.
 */
void EntityAutoLoader::reserve( int numEntities )
{
	entities_.reserve( numEntities );
}


/**
 *	This method starts loading the entities into the system.
 */
void EntityAutoLoader::start()
{
	if (entities_.empty())
	{
		// If no entities need to be loaded, just start the database server.
		DBApp::instance().onEntitiesAutoLoadCompleted(
			/*didAutoLoad*/ false );
		return;
	}

	for (int i = 0; i < maxOutstanding_; i++)
	{
		this->sendNext();
	}
}


/**
 *	This method is used instead of start() to indicate that there was an
 * 	error.
 */
void EntityAutoLoader::abort()
{
	entities_.clear();
	DBApp::instance().onEntitiesAutoLoadError();
	delete this;
}


/**
 *	This method adds a database entry that will later be loaded.
 */
void EntityAutoLoader::addEntity( EntityTypeID entityTypeID, DatabaseID dbID )
{
	entities_.push_back( std::make_pair( entityTypeID, dbID ) );
}


/**
 *	This method loads the next pending entity.
 */
void EntityAutoLoader::sendNext()
{
	if (!this->allSent() && numOutstanding_ < maxOutstanding_)
	{
		AutoLoadingEntityHandler* pEntityAutoLoader =
			new AutoLoadingEntityHandler( entities_[numSent_].first,
				entities_[numSent_].second, *this );
		pEntityAutoLoader->autoLoad();

		++numSent_;

		// TRACE_MSG( "EntityAutoLoader::sendNext: numSent = %d\n", numSent_ );

		++numOutstanding_;
	}
}


/**
 *	AutoLoadingEntityHandler calls this method when the process of auto-loading
 *	the entity has completed - regardless of success or failure.
 */
void EntityAutoLoader::onAutoLoadEntityComplete( bool isOK )
{
	--numOutstanding_;
	hasErrors_ = hasErrors_ || !isOK;

	if (!hasErrors_)
	{
		this->sendNext();
	}

	this->checkFinished();
}


/**
 *	This method checks whether or not this object has finished its job. If it
 *	has, this object deletes itself.
 */
void EntityAutoLoader::checkFinished()
{
	MF_ASSERT( numOutstanding_ >= 0 );

	if (numOutstanding_ == 0)
	{
		if (hasErrors_)
		{
			DBApp::instance().onEntitiesAutoLoadError();
		}
		else if (this->allSent())
		{
			bool didAutoLoadEntitiesFromDB = (numSent_ > 0);
			DBApp::instance().onEntitiesAutoLoadCompleted(
				didAutoLoadEntitiesFromDB );
		}
		delete this;
	}
}

BW_END_NAMESPACE

// entity_auto_loader.cpp
