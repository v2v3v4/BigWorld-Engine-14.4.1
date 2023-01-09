#include "log_component_names.hpp"

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
LogComponentNamesMongoDB::LogComponentNamesMongoDB(
		TaskManager & mongoDBTaskMgr, mongo::DBClientConnection & conn,
		const BW::string & collName ) :
	mongoDBTaskMgr_( mongoDBTaskMgr ),
	conn_( conn ),
	namespace_( collName ),
	currMaxID_( 0 )
{
}


/**
 * Write a new component name to db
 */
bool LogComponentNamesMongoDB::writeComponentNameToDB(
		const BW::string & componentName )
{
	ComponentNameID id = this->getNextAvailableID();

	componentIDMap_[ componentName ] = id;

	// Queue up a task to insert the category map to database
	mongoDBTaskMgr_.addBackgroundTask(
		new MongoDB::WriteIDStringPairTask< ComponentNameID >
			( namespace_, MongoDB::COMPONENTS_KEY_ID, id,
				MongoDB::COMPONENTS_KEY_NAME, componentName ) );

	// Success status is not known until the task is completed by the
	// background TaskManager thread.
	return true;
}


/**
 * Read component names from db
 */
bool LogComponentNamesMongoDB::init()
{
	try
	{
		std::auto_ptr<mongo::DBClientCursor> cursor = conn_.query(
				namespace_.c_str(), mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "LogComponentNamesMongoDB::init: "
				"Failed to get cursor.\n" );
			return false;
		}

		while (cursor->more())
		{
			mongo::BSONObj rec = cursor->next();
			ComponentNameID id = rec.getIntField( MongoDB::COMPONENTS_KEY_ID );
			BW::string name = rec.getStringField(
				MongoDB::COMPONENTS_KEY_NAME );

			this->pushbackComponentName( name );

			componentIDMap_[ name ] = id;

			currMaxID_ = std::max( currMaxID_, id );
		}
	}
	catch (mongo::DBException & e)
	{
		ERROR_MSG( "LogComponentNamesMongoDB::init: "
				"Couldn't read component names from db: %s\n", e.what() );
		return false;
	}

	return true;
}


/**
 * Get the id of the component name
 */
ComponentNameID LogComponentNamesMongoDB::getIDOfComponentName(
	const BW::string & componentName )
{
	// This is just to make sure that this component name will be stored
	// into database
	this->getAppTypeIDFromName(	componentName );

	ComponentIDMap::iterator iter = componentIDMap_.find( componentName );
	if (iter != componentIDMap_.end())
	{
		return iter->second;
	}
	
	return 0;
}


/**
 * Get an ID for a new component name
 */
ComponentNameID LogComponentNamesMongoDB::getNextAvailableID()
{
	currMaxID_ += 1;
	return currMaxID_;
}


BW_END_NAMESPACE

