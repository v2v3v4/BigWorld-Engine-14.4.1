#include "script/first_include.hpp"

#include "get_dbid_task.hpp"

#include "db_storage_mysql/mappings/entity_type_mapping.hpp"
#include "db_storage_mysql/tasks/get_entity_task.hpp"

#include "db_storage_mysql/mysql_database.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: GetDbIDTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GetDbIDTask::GetDbIDTask( const EntityTypeMapping * pEntityTypeMapping,
			const EntityDBKey & entityKey,
			IDatabase::IGetDbIDHandler & getDbIDHandler ) :
	MySqlBackgroundTask( "GetDbIDTask" ),
	pEntityTypeMapping_( pEntityTypeMapping ),
	entityKey_( entityKey ),
	getDbIDHandler_( getDbIDHandler )
{
	MF_ASSERT( entityKey.dbID == 0 );
	MF_ASSERT( pEntityTypeMapping_->hasIdentifier() );
}


/**
 *	This method performs this task in a background thread.
 *
 *	Set the missing member of the EntityDBKey. If entity doesn't
 *	have a name property then entityKey.name is set to empty.
 *
 */
void GetDbIDTask::performBackgroundTask( MySql & connection )
{
	entityKey_.dbID = pEntityTypeMapping_->getDbID( connection,
						entityKey_.name );

	if (entityKey_.dbID == 0)
	{
		this->setFailure();
	}
}


/**
 *	This method is called in the main thread after run() is complete.
 */
void GetDbIDTask::performMainThreadTask( bool succeeded )
{
	getDbIDHandler_.onGetDbIDComplete( succeeded, entityKey_ );
}

BW_END_NAMESPACE

// get_dbid_task.cpp
