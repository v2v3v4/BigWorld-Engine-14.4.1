#include "format_strings.hpp"

#include "constants.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "tasks/write_id_string_pair_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param conn		Established MongoDB connection
 */
FormatStringsMongoDB::FormatStringsMongoDB( TaskManager & mongoDBTaskMgr,
		mongo::DBClientConnection & conn, const BW::string & collName ) :
	mongoDBTaskMgr_( mongoDBTaskMgr ),
	conn_( conn ),
	namespace_( collName ),
	currMaxID_( 0 )
{
}


/**
 * Write a new format string to db
 */
bool FormatStringsMongoDB::writeFormatStringToDB(
		LogStringInterpolator *pHandler )
{
	FormatStringID id = this->getNextAvailableID();
	BW::string fmtString = pHandler->formatString();

	fmtStrIDMap_[ fmtString ] = id;

	// Queue up a task to insert one format string to database
	mongoDBTaskMgr_.addBackgroundTask(
		new MongoDB::WriteIDStringPairTask< FormatStringID >
			( namespace_, MongoDB::FMT_STRS_KEY_ID, id,
				MongoDB::FMT_STRS_KEY_FMT_STR, fmtString ) );

	// Success status is not known until the task is completed by the
	// background TaskManager thread.
	return true;
}
 

/**
 * Read format strings from db. Throw exception upon error.
 */
bool FormatStringsMongoDB::init()
{
	try
	{
		std::auto_ptr< mongo::DBClientCursor > cursor = conn_.query(
				namespace_.c_str(),	mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "FormatStringsMongoDB::init: Failed to get cursor.\n" );
			return false;
		}

		while (cursor->more())
		{
			mongo::BSONObj rec = cursor->next();
			FormatStringID id = rec.getIntField( MongoDB::FMT_STRS_KEY_ID );
			BW::string fmtStr = rec.getStringField(
				MongoDB::FMT_STRS_KEY_FMT_STR );

			LogStringInterpolator *pHandler =
										new LogStringInterpolator( fmtStr );
			if (!pHandler->isOk())
			{
				ERROR_MSG( "FormatStringsMongoDB::init: "
					"Skipped invalid format string: %s\n", fmtStr.c_str() );

				delete pHandler;
				continue;
			}

			this->addFormatStringToMap( fmtStr, pHandler );

			fmtStrIDMap_[ fmtStr ] = id;

			currMaxID_ = std::max( currMaxID_, id );
		}
	}
	catch (mongo::DBException & e)
	{
		ERROR_MSG( "FormatStringsMongoDB::init: "
				"Couldn't read format strings from db: %s\n", e.what() );
		return false;
	}

	return true;
}


/**
 * Get the id of the format string. 
 * TODO: Think of a better to improve the performance, like some kind of cache
 */
FormatStringID FormatStringsMongoDB::getIdOfFmtString(
		const BW::string & fmtString )
{
	FmtStrIDMap::iterator iter = fmtStrIDMap_.find( fmtString );

	if (iter != fmtStrIDMap_.end())
	{
		return iter->second;
	}
	
	return 0;
}


/**
 * Get an ID for a new format string to be inserted to db
 */
FormatStringID FormatStringsMongoDB::getNextAvailableID()
{
	currMaxID_ += 1;
	return currMaxID_;
}


BW_END_NAMESPACE
