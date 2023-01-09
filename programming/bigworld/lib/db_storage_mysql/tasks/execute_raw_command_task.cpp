#include "script/first_include.hpp"

#include "execute_raw_command_task.hpp"

#include "cstdmf/blob_or_null.hpp"

#include "../database_exception.hpp"
#include "../query.hpp"
#include "../result_set.hpp"
#include "../wrapper.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ExecuteRawCommandTask
// -----------------------------------------------------------------------------

ExecuteRawCommandTask::ExecuteRawCommandTask( const BW::string & command,
		IDatabase::IExecuteRawCommandHandler & handler ) :
	MySqlBackgroundTask( "ExecuteRawCommandTask" ),
	command_( command ),
	handler_( handler )
{
}


/**
 *	This method executes a raw database command.
 */
void ExecuteRawCommandTask::performBackgroundTask( MySql & conn )
{
	const Query query( command_, /* shouldPartition */false );

	ResultSet resultSet;
	query.execute( conn, &resultSet );

	BinaryOStream & stream = handler_.response();

	do
	{
		if (resultSet.hasResult())
		{
			stream << ""; // no error.
			uint32 numFields =  uint32( resultSet.numFields() );
			stream << numFields;
			stream << uint32( resultSet.numRows() );

			ResultRow row;

			while (row.fetchNextFrom( resultSet ))
			{
				for (uint32 i = 0; i < numFields; ++i)
				{
					BlobOrNull value;
					row.getField( i, value );
					stream << value;
				}
			}
		}
		else
		{
			if (conn.fieldCount() == 0)
			{
				stream << BW::string();	// no error.
				stream << int32( 0 ); 		// no fields.
				stream << uint64( conn.affectedRows() );
			}
			else
			{
				// Otherwise an error occurred, and the call to
				// nextResult() should raise the actual exception and
				// onException() will be called.
			}
		}
	}
	while (conn.nextResult( &resultSet ));
}


void ExecuteRawCommandTask::onRetry()
{
	// Should rewind stream, if possible
}


void ExecuteRawCommandTask::onException( const DatabaseException & e )
{
	handler_.response() << e.what();
}


/**
 *	This method is called in the main thread after run() completes.
 */
void ExecuteRawCommandTask::performMainThreadTask( bool succeeded )
{
	handler_.onExecuteRawCommandComplete();
}

BW_END_NAMESPACE

// execute_raw_command_task.cpp
