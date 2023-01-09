#ifndef GET_DBID_TASK_HPP
#define GET_DBID_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "entity_task.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

class EntityTypeMapping;
class GetEntityTask;

/**
 *	This class performs a lookup to map an Identifier field into a DBID.
 */
class GetDbIDTask : public MySqlBackgroundTask
{
public:
	GetDbIDTask( const EntityTypeMapping * pEntityTypeMapping,
			const EntityDBKey & entityKey,
			IDatabase::IGetDbIDHandler & getDbIDHandler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	const EntityTypeMapping * pEntityTypeMapping_;
	EntityDBKey entityKey_;

	IDatabase::IGetDbIDHandler & getDbIDHandler_;
};

BW_END_NAMESPACE

#endif // GET_DBID_TASK_HPP
