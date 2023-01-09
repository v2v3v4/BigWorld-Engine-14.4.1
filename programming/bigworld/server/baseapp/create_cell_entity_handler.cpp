#include "script/first_include.hpp"

#include "create_cell_entity_handler.hpp"

#include "base.hpp"

#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CreateCellEntityHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CreateCellEntityHandler::CreateCellEntityHandler( Base * pBase ) :
	pBase_( pBase )
{
	MF_ASSERT( !pBase->hasCellEntity() );
	MF_ASSERT( !pBase->isCreateCellPending() );
	MF_ASSERT( !pBase->isGetCellPending() );
}


/*
 *	This method handles the reply from the cellapp.
 */
void CreateCellEntityHandler::handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg )
{
	EntityID entityID;
	data >> entityID;

	// MF_ASSERT( entityID == pBase_->id() );
	if (entityID == pBase_->id())
	{
		// INFO_MSG( "CreateCellEntityHandler::handleMessage: "
		// 	"Cell entity (%lu) created\n",
		// 	entityID );
		pBase_->cellCreationResult( true );
	}
	else
	{
		WARNING_MSG( "CreateCellEntityHandler::handleMessage: "
			"Failed to create associated cell entity for %u.\n"
			"\tResponse was entity id %u\n", pBase_->id(), entityID );
		pBase_->cellCreationResult( false );
	}

	delete this;
}


/*
 *	This method handles the failure case for creating the cell entity.
 */
void CreateCellEntityHandler::handleException(
		const Mercury::NubException & exception, void * arg )
{
	// TODO: Should do more than this.
	ERROR_MSG( "CreateCellEntityHandler::handleException: "
			"Failed to create associated cell entity for %u.\n"
			"\tNub exception was %s\n",
		pBase_->id(), Mercury::reasonToString( exception.reason() ) );

	pBase_->cellCreationResult( false );

	delete this;
}


// -----------------------------------------------------------------------------
// Section: CreateCellEntityViaBaseHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CreateCellEntityViaBaseHandler::CreateCellEntityViaBaseHandler( Base * pBase,
			Mercury::ReplyMessageHandler * pHandler,
			EntityID nearbyID ) :
	pBase_( pBase ),
	pHandler_( pHandler ),
	nearbyID_( nearbyID )
{
}


/*
 *	This method handles cell entity creation failure.
 */
void CreateCellEntityViaBaseHandler::cellCreationFailure()
{
	pBase_->cellCreationResult( false );
	delete this;
}


/*
 *	This method handles the reply from the cellapp.
 */
void CreateCellEntityViaBaseHandler::handleMessage(
		const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg )
{
	Mercury::Address cellAddr;
	data >> cellAddr;

	if (cellAddr.isNone())
	{
		ERROR_MSG( "CreateCellEntityViaBaseHandler:handleMessage: "
				"Nearby base entity %u has no cell. Unable to create "
				"cell for base %u\n",
			nearbyID_, pBase_->id() );
		return this->cellCreationFailure();
	}

	if (pBase_->isDestroyed())
	{
		ERROR_MSG( "CreateCellEntityViaBaseHandler::handleMessage: "
				"Base %u is destroyed\n", pBase_->id() );
		return this->cellCreationFailure();
	}

	if (data.error())
	{
		ERROR_MSG( "CreateCellEntityViaBaseHandler::handleMessage: "
				"Invalid response from %u\n", nearbyID_ );
		return this->cellCreationFailure();
	}

	// Should be safe to create the cell entity now
	if (!pBase_->sendCreateCellEntity( pHandler_.get(), nearbyID_, cellAddr, "" ))
	{
		Script::printError();
	}

	pHandler_.release();

	delete this;
}


/*
 *	This method handles the failure case for creating the cell entity.
 */
void CreateCellEntityViaBaseHandler::handleException(
		const Mercury::NubException & exception, void * arg )
{
	// TODO: Should do more than this.
	ERROR_MSG( "CreateCellEntityViaBaseHandler::handleException: "
			"Failed to create associated cell entity for %u.\n"
			"\tNub exception was %s\n",
		pBase_->id(), Mercury::reasonToString( exception.reason() ) );

	return this->cellCreationFailure();
}

BW_END_NAMESPACE

// create_cell_entity_handler.cpp
