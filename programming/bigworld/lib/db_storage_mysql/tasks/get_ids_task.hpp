#ifndef GET_IDS_TASK_HPP
#define GET_IDS_TASK_HPP

#include "background_task.hpp"

#include "db_storage/idatabase.hpp"


BW_BEGIN_NAMESPACE

class MySql;


/**
 *	This class encapsulates the MySqlDatabase::getIDs() operation
 *	so that it can be executed in a separate thread.
 */
class GetIDsTask : public MySqlBackgroundTask
{
public:
	GetIDsTask( int numIDs, IDatabase::IGetIDsHandler & handler );

	// MySqlBackgroundTask overrides
	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

protected:
	virtual void onRetry();

private:
	int getUsedIDs( MySql & conn, int numIDs, BinaryOStream & stream );
	void getNewIDs( MySql & conn, int numIDs, BinaryOStream & stream );

	int							numIDs_;
	IDatabase::IGetIDsHandler &	handler_;
};

BW_END_NAMESPACE

#endif // GET_IDS_TASK_HPP
