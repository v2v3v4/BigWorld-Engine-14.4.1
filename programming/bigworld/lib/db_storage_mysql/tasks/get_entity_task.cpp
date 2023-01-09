#include "script/first_include.hpp"

#include "get_entity_task.hpp"

#include "db_storage_mysql/mappings/entity_type_mapping.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: GetEntityTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GetEntityTask::GetEntityTask( const EntityTypeMapping * pEntityTypeMapping,
			const EntityDBKey & entityKey,
			BinaryOStream * pStream,
			bool shouldGetBaseEntityLocation,
			IDatabase::IGetEntityHandler & handler ) :
	EntityTask( *pEntityTypeMapping, entityKey.dbID, "GetEntityTask" ),
	entityKey_( entityKey ),
	pStream_( pStream ),
	threadStream_(),
	handler_( handler ),
	baseEntityLocation_(),

	shouldGetBaseEntityLocation_( shouldGetBaseEntityLocation ),
	hasBaseLocation_( false )
{
}


/**
 *	This method performs this task in a background thread.
 */
void GetEntityTask::performBackgroundTask( MySql & conn )
{
	bool isOkay = true;

	MF_ASSERT( entityKey_.dbID != 0);

	if (pStream_ != NULL)
	{
		isOkay = entityTypeMapping_.getStreamByID( conn, entityKey_.dbID,
			threadStream_ );
	}

	if (isOkay && shouldGetBaseEntityLocation_)
	{
		// Try to get base mailbox
		hasBaseLocation_ = entityTypeMapping_.getLogOnRecord( conn,
							entityKey_.dbID, baseEntityLocation_ );
	}

	if (!isOkay)
	{
		this->setFailure();
	}
}


/**
 *	This method is called in the main thread after run() is complete.
 */
void GetEntityTask::performEntityMainThreadTask( bool succeeded )
{
	// Merge the thread stream data back into the main result stream
	if (pStream_ != NULL)
	{
		pStream_->transfer( threadStream_, threadStream_.size() );
	}

	handler_.onGetEntityComplete( succeeded, entityKey_,
			hasBaseLocation_ ? &baseEntityLocation_ : NULL );
}


/**
 *	This method is called if the background task fails and should be retried.
 */
void GetEntityTask::onRetry()
{
	threadStream_.reset();
}


/**
 *	This method calls the failure callback on the handler.
 *
 *	This is used when performing a DatabaseID lookup from an Identifier in a
 *	GetDbIDTask.
 */
void GetEntityTask::onDatabaseIDLookupFailure()
{
	handler_.onGetEntityComplete( /*succeeded:*/ false, entityKey_,
		/*hasBaseLocation:*/ NULL );
}

BW_END_NAMESPACE

// get_entity_task.cpp
