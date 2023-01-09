#include "script/first_include.hpp"

#include "load_entity_handler.hpp"

#include "base.hpp"
#include "baseapp.hpp"

#include "db/dbapp_interface.hpp"

#include "connection/log_on_status.hpp"

#include "network/bundle.hpp"

#include "server/writedb.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: LoadEntityHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
LoadEntityHandler::LoadEntityHandler( EntityTypeID entityTypeID,
		EntityID entityID ) :
	entityTypeID_( entityTypeID ),
	entityID_( entityID )
{
}

void LoadEntityHandler::handleMessage(const Mercury::Address& srcAddr,
	Mercury::UnpackedMessageHeader& /*header*/,
	BinaryIStream& data, void * /*arg*/)
{
	uint8 result;
	data >> result;

	if (result == LogOnStatus::LOGGED_ON)
	{
		BasePtr pBase = NULL;
		EntityID id = entityID_;
		DatabaseID databaseID;
		data >> databaseID;

		EntityTypePtr pType = EntityType::getType( entityTypeID_,
				/*shouldExcludeServices*/ true );

		BaseApp & baseApp = BaseApp::instance();
		MF_ASSERT( pType );
		MF_ASSERT( pType->canBeOnBase() );

		if (id == 0)
		{
			id = baseApp.getID();
		}

		if (id == 0)
		{
			ERROR_MSG( "LoadEntityHandler::handleMessage: "
					"Unable to allocate id.\n" );
			this->unCheckoutEntity( databaseID );
			databaseID = 0;
			data.finish();
		}
		else
		{
			pBase = pType->create( id, databaseID, data, true );

			if (!pBase)
			{
				this->unCheckoutEntity( databaseID );
			}
		}
		this->onLoadedFromDB( pBase.get(), NULL, databaseID );
	}
	else if (result == LogOnStatus::LOGIN_REJECTED_ALREADY_LOGGED_IN)
	{
		DatabaseID databaseID;
		data >> databaseID;
		EntityMailBoxRef baseMB;
		data >> baseMB;

		if (entityID_)
		{
			BaseApp::instance().putUsedID( entityID_ );
		}

		this->onLoadedFromDB( NULL, &baseMB, databaseID );
	}
	else
	{
		BW::string msg;
		if (data.remainingLength())
		{
			data >> msg;
		}

		NOTICE_MSG( "LoadEntityHandler::handleMessage: "
			"Failed with reason %d %s\n", int( result ),
			msg.c_str() );
		if (entityID_)
		{
			BaseApp::instance().putUsedID( entityID_ );
		}
		this->onLoadedFromDB( NULL, NULL, 0 );
	}

	delete this;
}

void LoadEntityHandler::handleException( const Mercury::NubException& /*ne*/,
		void* /*arg*/)
{
	ERROR_MSG( "LoadEntityHandler::handleException: "
		"Failed to create base\n" );
	this->onLoadedFromDB( NULL, NULL, 0 );
	BaseApp::instance().putUsedID( entityID_ );

	delete this;
}


void LoadEntityHandler::handleShuttingDown( const Mercury::NubException & ne,
		void * )
{
	INFO_MSG( "LoadEntityHandler::handleShuttingDown: Ignoring\n" );
	delete this;
}


void LoadEntityHandler::unCheckoutEntity( DatabaseID databaseID )
{
	BaseApp& baseApp = BaseApp::instance();
	Mercury::Bundle & bundle = baseApp.dbApp().bundle();

	bundle.startMessage( DBAppInterface::writeEntity );
	bundle << WriteDBFlags( WRITE_LOG_OFF ) << entityTypeID_ << databaseID << entityID_;
	baseApp.dbApp().send();
}


// -----------------------------------------------------------------------------
// Section: LoadEntityHandlerWithCallback
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
LoadEntityHandlerWithCallback::LoadEntityHandlerWithCallback(
		PyObjectPtr pResultHandler,
		EntityTypeID entityTypeID, EntityID entityID ) :
	LoadEntityHandler( entityTypeID, entityID ),
	pResultHandler_( pResultHandler )
{
}

void LoadEntityHandlerWithCallback::onLoadedFromDB( Base * pBase,
		EntityMailBoxRef * pMailbox, DatabaseID dbID )
{
	if (pResultHandler_)
	{
		PyObject* pArg = NULL;
		PyObject* pOwnedArg = NULL;
		if (pMailbox)
		{
			pArg = Script::getData( *pMailbox );
			pOwnedArg = pArg;
		}
		else if (pBase)
		{
			pArg = pBase;
		}
		else
		{
			pArg = Py_None;
		}

		Py_INCREF( pResultHandler_.get() );
		PyObject* pDbID = Script::getData( dbID );
		PyObject* pWasActive = Script::getData( pMailbox != NULL );
		Script::call( pResultHandler_.get(),
				PyTuple_Pack( 3, pArg, pDbID, pWasActive ),
				"LoadEntityHandlerWithCallback::onLoadedFromDB ",
				/*okIfFnNull:*/false );

		Py_DECREF( pDbID );
		Py_DECREF( pWasActive );
		Py_XDECREF( pOwnedArg );
	}
}


// -----------------------------------------------------------------------------
// Section: LoadEntityHandlerWithReply
// -----------------------------------------------------------------------------

BW_END_NAMESPACE

#include "network/channel_sender.hpp"

BW_BEGIN_NAMESPACE

LoadEntityHandlerWithReply::LoadEntityHandlerWithReply(
		Mercury::ReplyID replyID,
		const Mercury::Address& srcAddr,
		EntityTypeID entityTypeID, EntityID entityID ) :
	LoadEntityHandler( entityTypeID, entityID ),
	replyID_( replyID ), srcAddr_( srcAddr )
{
}

// LoadEntityHandler overrides
void LoadEntityHandlerWithReply::onLoadedFromDB( Base * pBase,
	EntityMailBoxRef * pMailbox, DatabaseID dbID )
{
	Mercury::ChannelSender sender( BaseApp::getChannel( srcAddr_ ) );
	Mercury::Bundle & reply = sender.bundle();

	reply.startReply( replyID_ );

	if (pBase)
	{
		reply << LOAD_FROM_DB_SUCCEEDED;
		reply << dbID;
		reply << pBase->baseEntityMailBoxRef();
	}
	else if (pMailbox)
	{
		reply << LOAD_FROM_DB_FOUND_EXISTING;
		reply << dbID;
		reply << *pMailbox;
	}
	else
	{
		reply << LOAD_FROM_DB_FAILED;
	}
}

BW_END_NAMESPACE

// load_entity_handler.cpp
