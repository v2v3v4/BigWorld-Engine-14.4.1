#ifndef GET_ENTITY_TASK_HPP
#define GET_ENTITY_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "entity_task.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

class EntityTypeMapping;

/**
 *	This class encapsulates the MySqlDatabase::getEntity() operation so that
 *	it can be executed in a separate thread.
 */
class GetEntityTask : public EntityTask
{
public:
	GetEntityTask( const EntityTypeMapping * pEntityTypeMapping,
			const EntityDBKey & entityKey,
			BinaryOStream * pStream,
			bool shouldGetBaseEntityLocation,
			IDatabase::IGetEntityHandler & handler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performEntityMainThreadTask( bool succeeded );

	void onDatabaseIDLookupFailure();

	void updateEntityKey( const EntityDBKey & entityKey )
		{ entityKey_ = entityKey; }

protected:
	virtual void onRetry();

private:
	EntityDBKey entityKey_;

	BinaryOStream * pStream_;
	MemoryOStream threadStream_;


	IDatabase::IGetEntityHandler & handler_;

	EntityMailBoxRef baseEntityLocation_;

	// Put at the end for better packing
	bool shouldGetBaseEntityLocation_;
	bool hasBaseLocation_;
};

BW_END_NAMESPACE

#endif // GET_ENTITY_TASK_HPP
