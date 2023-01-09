#ifndef UPDATE_SECONDARY_DBS_TASK_HPP
#define UPDATE_SECONDARY_DBS_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "background_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements the updateSecondaryDBs() function as a thread task.
 */
class UpdateSecondaryDBsTask : public MySqlBackgroundTask
{
public:
	UpdateSecondaryDBsTask( const SecondaryDBAddrs & addrs,
			IDatabase::IUpdateSecondaryDBshandler & handler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	IDatabase::IUpdateSecondaryDBshandler &			handler_;
	BW::string 									condition_;
	IDatabase::SecondaryDBEntries					entries_;
};

BW_END_NAMESPACE

#endif // UPDATE_SECONDARY_DBS_TASK_HPP
