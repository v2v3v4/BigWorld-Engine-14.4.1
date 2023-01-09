#include "write_to_db_reply.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
WriteToDBReplyStruct::WriteToDBReplyStruct( WriteToDBReplyHandler * pHandler ) :
	succeeded_( false ),
	backedUp_( false ),
	writtenToDB_( false ),
	pHandler_( pHandler ) 
{
}


/**
 *	This method is called when writing to the database has completed. The
 *	handler is called if the backup has also completed.
 */
void WriteToDBReplyStruct::onWriteToDBComplete( bool succeeded )
{
	MF_ASSERT( !writtenToDB_ );
	succeeded_ = succeeded;
	writtenToDB_ = true;

	if (pHandler_ && (!succeeded_ || backedUp_))
	{
		pHandler_->onWriteToDBComplete( succeeded_ );

		// The handler callback method should only be called once.
		pHandler_ = NULL;
	}
}


/**
 *	This method is called when backing up the base entity to another BaseApp
 *	has completed. The handler is called if writing to the database has also
 *	completed.
 */
void WriteToDBReplyStruct::onBackupComplete()
{
	MF_ASSERT( !backedUp_ );
	backedUp_ = true;

	if (writtenToDB_ && pHandler_)
	{
		pHandler_->onWriteToDBComplete( succeeded_ );

		// The handler callback method should be called once.
		pHandler_ = NULL;
	}
}

BW_END_NAMESPACE

// write_to_db_reply.cpp
