#ifndef EXECUTE_RAW_COMMAND_TASK_HPP
#define EXECUTE_RAW_COMMAND_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "background_task.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class encapsulates the MySqlDatabase::executeRawCommand() operation
 *	so that it can be executed in a separate thread.
 */
class ExecuteRawCommandTask : public MySqlBackgroundTask
{
public:
	ExecuteRawCommandTask( const BW::string & command,
			IDatabase::IExecuteRawCommandHandler & handler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

protected:
	void onRetry();
	void onException( const DatabaseException & e );

	BW::string									command_;
	IDatabase::IExecuteRawCommandHandler &		handler_;
};

BW_END_NAMESPACE

#endif // EXECUTE_RAW_COMMAND_TASK_HPP
