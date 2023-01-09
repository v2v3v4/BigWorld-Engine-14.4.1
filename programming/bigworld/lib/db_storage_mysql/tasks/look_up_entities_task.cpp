#include "script/first_include.hpp"

#include "look_up_entities_task.hpp"

#include "../mappings/entity_type_mapping.hpp"
#include "../database_exception.hpp"
#include "../query.hpp"
#include "../query_runner.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param entityTypeMapping 	The entity type's MySQL mapping.
 *	@param criteria				The property queries to match.
 *	@param handler				The handler to callback on.
 */
LookUpEntitiesTask::LookUpEntitiesTask( 
			const EntityTypeMapping & entityTypeMapping,
			const LookUpEntitiesCriteria & criteria,
			IDatabase::ILookUpEntitiesHandler & handler ):
		MySqlBackgroundTask( "LookUpEntitiesTask" ),
		mapping_( entityTypeMapping ),
		criteria_( criteria ),
		handler_( handler )
{
}


/**
 *	Perform the look up in the worker thread.
 *
 *	@param conn 	A connection to the database.
 */
void LookUpEntitiesTask::performBackgroundTask( MySql & conn )
{
	try
	{
		if (!mapping_.lookUpEntitiesByProperties( conn, criteria_, handler_ ))
		{
			this->setFailure();
		}
	}
	catch (DatabaseException & dbe)
	{
		ERROR_MSG( "Database exception occurred while looking up "
				"on entity type %s: %s\n", 
			mapping_.typeName().c_str(), dbe.what() );
		throw;
	}
}


/**
 *	Finish the task in the main thread.
 *
 *	@param succeeded 	Whether the background task completed successfully.
 */
void LookUpEntitiesTask::performMainThreadTask( bool succeeded ) 
{
	handler_.onLookUpEntitiesEnd( !succeeded );
}

BW_END_NAMESPACE

// look_up_entities_task.cpp

