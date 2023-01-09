#ifndef ADD_SECONDARY_DB_ENTRY_TASK_HPP
#define ADD_SECONDARY_DB_ENTRY_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "background_task.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This thread task adds a secondary db entry.
 */
class AddSecondaryDBEntryTask : public MySqlBackgroundTask
{
public:
	AddSecondaryDBEntryTask( const IDatabase::SecondaryDBEntry & entry );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	IDatabase::SecondaryDBEntry entry_;
};

BW_END_NAMESPACE

#endif // ADD_SECONDARY_DB_ENTRY_TASK_HPP
