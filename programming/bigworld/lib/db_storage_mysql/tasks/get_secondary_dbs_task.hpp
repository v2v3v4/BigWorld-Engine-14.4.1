#ifndef GET_SECONDARY_DBS_TASK_HPP
#define GET_SECONDARY_DBS_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "background_task.hpp"


BW_BEGIN_NAMESPACE

void getSecondaryDBEntries( MySql & connection,
		IDatabase::SecondaryDBEntries & entries,
		const BW::string & condition = BW::string() );

/**
 *	This class implements the getSecondaryDBs() function as a thread task.
 */
class GetSecondaryDBsTask : public MySqlBackgroundTask
{
public:
	typedef IDatabase::IGetSecondaryDBsHandler Handler;

	GetSecondaryDBsTask( Handler & handler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	Handler & handler_;
	IDatabase::SecondaryDBEntries entries_;
};

BW_END_NAMESPACE

#endif // GET_SECONDARY_DBS_TASK_HPP
