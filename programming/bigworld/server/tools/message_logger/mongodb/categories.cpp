#include "categories.hpp"

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
CategoriesMongoDB::CategoriesMongoDB( TaskManager & mongoDBTaskMgr,
		mongo::DBClientConnection & conn, const BW::string & collName ):
	mongoDBTaskMgr_( mongoDBTaskMgr ),
	conn_( conn ),
	namespace_( collName )
{
}


/**
 * Write a category record to database
 */
bool CategoriesMongoDB::writeCategoryToDB(
	MessageLogger::CategoryID newCategoryID, const BW::string & categoryName )
{
	// Queue up a task to insert the category map to database
	mongoDBTaskMgr_.addBackgroundTask(
		new MongoDB::WriteIDStringPairTask< MessageLogger::CategoryID >
			( namespace_, MongoDB::CATEGORIES_KEY_ID, newCategoryID,
				MongoDB::CATEGORIES_KEY_NAME, categoryName ) );

	// Success status is not known until the task is completed by the
	// background TaskManager thread.
	return true;
}


/**
 * Init category names from database. Throw exception upon error.
 */
bool CategoriesMongoDB::init()
{
	try
	{
		std::auto_ptr< mongo::DBClientCursor > cursor = conn_.query(
				namespace_.c_str(),	mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "CategoriesMongoDB::init: Failed to get cursor.\n" );
			return false;
		}

		while (cursor->more())
		{
			mongo::BSONObj rec = cursor->next();
			MessageLogger::CategoryID id = rec.getIntField(
				MongoDB::CATEGORIES_KEY_ID );
			BW::string name = rec.getStringField(
				MongoDB::CATEGORIES_KEY_NAME );

			this->addCategoryToMap( id, name );
		}
	}
	catch (mongo::DBException & e)
	{
		ERROR_MSG( "CategoriesMongoDB::init: "
				"Couldn't read categories from db: %s\n", e.what() );
		return false;
	}

	return true;
}


BW_END_NAMESPACE
