#include "script/first_include.hpp"

#include "dbapp_interface_utils.hpp"

#include "cstdmf/blob_or_null.hpp"
#include "cstdmf/debug.hpp"

#include "db/dbapp_interface.hpp"

#include "network/channel_sender.hpp"
#include "network/nub_exception.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

namespace DBAppInterfaceUtils
{
	/*~ function BigWorld.executeRawDatabaseCommand
	 *	@components{ base }
	 *	This script function executes the given raw database command on the
	 *	database. There is no attempt to parse the command - it is passed
	 * 	straight through to the underlying database. This feature is not
	 * 	supported for the XML database.
	 *
	 *  Please note that the entity data in the database may not be up to date,
	 * 	especially when secondary databases are enabled. It is highly
	 *  recommended that this function is not used to read or modify entity
	 * 	data.
	 *
	 *	@param command The command string specific to the present database layer.
	 *  For MySQL database it is an SQL query.
	 *	@param callback The object to call back (e.g. a function) with the
	 *	result of the command execution. 
	 *
	 *	For commands that return single result sets, the callback will be
	 *	called with 3 parameters: result set, number of affected rows and error
	 *	string.
	 *
	 * 	The result set parameter is a list of rows. Each row is a list of
	 * 	strings containing field values. The result set will be None for
	 * 	commands that do not return a result set e.g. DELETE, or if there was
	 * 	an error in executing the command.
	 *
	 * 	The number of affected rows parameter is a number indicating the
	 * 	number of rows affected by the command. This parameter is only relevant
	 * 	for commands to do not return a result set e.g. DELETE. This parameter
	 * 	is None for commands that do return a result set or if there was and
	 * 	error in executing the command.
	 *
	 * 	The error string parameter is a string describing the error that
	 * 	occurred if there was an error in executing the command. This parameter
	 * 	is None if there was no error in executing the command.
	 *
	 * 	For commands that return multiple result sets (for example, calling
	 * 	stored procedures, or passing multiple semicolon delimited statements
	 * 	as the command), the callback will instead be called with a list of
	 * 	tuples containing the result set, the number of affected rows and the
	 * 	error string. 
	 */
	/**
	 * 	NOTE: The comment block below is a copy of the comment block above with
	 * 	an additional note for cell entity callbacks.
	 */
	/*~ function BigWorld.executeRawDatabaseCommand
	 *	@components{ cell }
	 *	This script function executes the given raw database command on the
	 *	database. There is no attempt to parse the command - it is passed
	 * 	straight through to the underlying database. This feature is not
	 * 	supported for the XML database.
	 *
	 *  Please note that the entity data in the database may not be up to date,
	 * 	especially when secondary databases are enabled. It is highly
	 *  recommended that this function is not used to read or modify entity
	 * 	data.
	 *
	 *	@param command The command string specific to the present database layer.
	 *  For MySQL database it is an SQL query.
	 *	@param callback The object to call back (e.g. a function) with the
	 *	result of the command execution. 
	 *
	 *	For commands that return single result sets, the callback will be
	 *	called with 3 parameters: result set, number of affected rows and error
	 *	string.
	 *
	 * 	The result set parameter is a list of rows. Each row is a list of
	 * 	strings containing field values. The result set will be None for
	 * 	commands that do not return a result set e.g. DELETE, or if there was
	 * 	an error in executing the command.
	 *
	 * 	The number of affected rows parameter is a number indicating the
	 * 	number of rows affected by the command. This parameter is only relevant
	 * 	for commands to do not return a result set e.g. DELETE. This parameter
	 * 	is None for commands that do return a result set or if there was and
	 * 	error in executing the command.
	 *
	 * 	The error string parameter is a string describing the error that
	 * 	occurred if there was an error in executing the command. This parameter
	 * 	is None if there was no error in executing the command.
	 *
	 * 	For commands that return multiple result sets (for example, calling
	 * 	stored procedures, or passing multiple semicolon delimited statements
	 * 	as the command), the callback will instead be called with a list of
	 * 	tuples containing the result set, the number of affected rows and the
	 * 	error string. 
	 *
	 * 	Please note that due to the itinerant nature of cell entities, the
	 * 	callback function should handle cases where the cell
	 * 	entity has been converted to a ghost or has been destroyed. In general,
	 * 	the callback function can be a simple function that calls an entity
	 * 	method defined in the entity definition file to do the actual work.
	 * 	This way, the call will be forwarded to the real entity if the current
	 * 	entity has been converted to a ghost.
	 */
	/**
	 *	This class handles the response from DBApp to an executeRawCommand
	 * 	request.
	 */
	class ExecRawDBCmdWaiter : public Mercury::ReplyMessageHandler
	{
	public:
		ExecRawDBCmdWaiter( PyObjectPtr pResultHandler ) :
			pResultHandler_( pResultHandler.getObject() ),
			pResultSets_( PyList_New( 0 ) )
		{
			Py_XINCREF( pResultHandler_ );

			// These Python references for pResultHandler_ and pResultSets_
			// will be released in done() after calling Script::call().
		}

		// Mercury::ReplyMessageHandler overrides
		virtual void handleMessage(const Mercury::Address& source,
			Mercury::UnpackedMessageHeader& header,
			BinaryIStream& data, void * arg)
		{
			TRACE_MSG( "ExecRawDBCmdWaiter::handleMessage: "
				"DB call response received\n" );

			while (data.remainingLength() > 0)
			{
				this->processTabularResult( data );
			}

			this->done();
		}

		void processTabularResult( BinaryIStream& data )
		{
			PyObject* pResultSet;
			PyObject* pAffectedRows;
			PyObject* pErrorMsg;

			BW::string errorMsg;
			data >> errorMsg;

			if (errorMsg.empty())
			{
				pErrorMsg = this->newPyNone();

				uint32 numColumns;
				data >> numColumns;

				if (numColumns > 0)
				{	// Command returned tabular data.
					pAffectedRows = this->newPyNone();

					uint32 numRows;
					data >> numRows;
					// Make list of list of strings.
					pResultSet = PyList_New( numRows );
					for ( uint32 i = 0; i < numRows; ++i )
					{
						PyObject* pRow = PyList_New( numColumns );
						for ( uint32 j = 0; j < numColumns; ++j )
						{
							BlobOrNull cell;
							data >> cell;

							PyObject* pCell = (cell.isNull()) ?
									this->newPyNone() :
									PyString_FromStringAndSize( cell.pData(),
												cell.length() );
							PyList_SET_ITEM( pRow, j, pCell );
						}
						PyList_SET_ITEM( pResultSet, i, pRow );
					}
				}
				else
				{	// Empty result set - only affected rows returned.
					uint64	numAffectedRows;
					data >> numAffectedRows;

					pResultSet = this->newPyNone();
					pAffectedRows = Script::getData( numAffectedRows );
					pErrorMsg = this->newPyNone();
				}
			}
			else	// Error has occurred.
			{
				pResultSet = this->newPyNone();
				pAffectedRows = this->newPyNone();
				pErrorMsg = Script::getData( errorMsg );
			}

			PyObject * pResultSetTuple = Py_BuildValue( "(OOO)",
				pResultSet, pAffectedRows, pErrorMsg );

			PyList_Append( pResultSets_, pResultSetTuple );
			Py_DECREF( pResultSetTuple );

			Py_DECREF( pResultSet );
			Py_DECREF( pAffectedRows );
			Py_DECREF( pErrorMsg );
		}

		void handleException(const Mercury::NubException& exception, void* arg)
		{
			// This can be called during Channel destruction which can happen
			// after Script has been finalised.
			if (!Script::isFinalised())
			{
				BW::stringstream errorStrm;
				errorStrm << "Nub exception " <<
						Mercury::reasonToString( exception.reason() );
				ERROR_MSG( "ExecRawDBCmdWaiter::handleException: %s\n",
						errorStrm.str().c_str() );

				PyObject * pResultSetTuple = Py_BuildValue( "(OON)",
					Py_None, Py_None,
					Script::getData( errorStrm.str() ) );
				PyList_Append( pResultSets_, pResultSetTuple );
				Py_DECREF( pResultSetTuple );

				this->done();
			}
		}

	private:
		void done()
		{
			if (pResultHandler_)
			{
				if (PyList_Size( pResultSets_ ) == 1)
				{
					// Special case to be backwards compatible with single
					// result set queries.

					Script::call( pResultHandler_,
						PySequence_GetItem( pResultSets_, 0 ),
						"ExecRawDBCmdWaiter callback", /*okIfFnNull: */ false );
					// 'call' does the decref of pResultHandler_ and first
					// element of pResultSets_ for us

					Py_DECREF( pResultSets_ );
				}
				else
				{
					Script::call( pResultHandler_,
						Py_BuildValue( "(O)", pResultSets_ ),
						"ExecRawDBCmdWaiter callback", /*okIfFnNull: */ false );

					// 'call' does the decref of pResultHandler_ and
					// pResultSets_ for us
				}
			}

			delete this;
		}

		static PyObject* newPyNone()
		{
			PyObject* pNone = Py_None;
			Py_INCREF( pNone );
			return pNone;
		}

		PyObject * 		pResultHandler_;
		PyObject *		pResultSets_;
	};

	/**
	 *	This function sends a message to the DBApp to an run an
	 * 	executeRawDatabaseCommand request. When the result is sent back from
	 * 	DBApp, pResultHandler will be called if specified.
	 */
	bool executeRawDatabaseCommand( const BW::string & command,
			PyObjectPtr pResultHandler, Mercury::Channel & channel )
	{
		if (pResultHandler && !PyCallable_Check( pResultHandler.get() ) )
		{
			PyErr_SetString( PyExc_TypeError,
				"BigWorld.executeRawDatabaseCommand() "
				"callback must be callable if specified" );
			return false;
		}

		Mercury::Bundle & bundle = channel.bundle();
		bundle.startRequest( DBAppInterface::executeRawCommand,
			new ExecRawDBCmdWaiter( pResultHandler ) );
		bundle.addBlob( command.data(), command.size() );

		channel.send();

		return true;
	}

	/**
	 *	This function sends a message to the DBApp to an run an
	 * 	executeRawDatabaseCommand request. When the result is sent back from
	 * 	DBApp, pResultHandler will be called if specified.
	 */
	bool executeRawDatabaseCommand( const BW::string & command,
			PyObjectPtr pResultHandler, Mercury::NetworkInterface & interface,
			const Mercury::Address& dbAppAddr )
	{
		if (pResultHandler && !PyCallable_Check( pResultHandler.get() ) )
		{
			PyErr_SetString( PyExc_TypeError,
				"BigWorld.executeRawDatabaseCommand() "
				"callback must be callable if specified" );
			return false;
		}

		Mercury::ChannelSender sender(
				interface.findOrCreateChannel( dbAppAddr ) );
		Mercury::Bundle & bundle = sender.bundle();

		bundle.startRequest( DBAppInterface::executeRawCommand,
			new ExecRawDBCmdWaiter( pResultHandler ) );
		bundle.addBlob( command.data(), command.size() );

		return true;
	}
} // namespace DBAppInterfaceUtils

BW_END_NAMESPACE

// dbapp_interface_utils.cpp
