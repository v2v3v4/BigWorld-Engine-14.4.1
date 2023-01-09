#include "write_space_data_task.hpp"

#include "../query.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
WriteSpaceDataTask::WriteSpaceDataTask( BinaryIStream & spaceData ) :
	MySqlBackgroundTask( "WriteSpaceDataTask" ),
	numSpaces_( 0 )
{
	data_.transfer( spaceData, spaceData.remainingLength() );
}


/**
 *	This static method reads from the stream containing space data and inserts
 * 	the data into the database. Returns the number spaces in the stream.
 */
uint32 WriteSpaceDataTask::writeSpaceDataStreamToDB( MySql & connection,
		BinaryIStream & stream )
{
	static const Query insertSpaceIDQuery(
			"INSERT INTO bigworldSpaces (id) VALUES (?)" );
	static const Query insertSpaceDataQuery(
			"INSERT INTO bigworldSpaceData "
					"(id, spaceEntryID, entryKey, data) "
					"VALUES (?, ?, ?, ?)" );

	uint32 numSpaces;
	stream >> numSpaces;

	for (uint32 spaceIndex = 0; spaceIndex < numSpaces; ++spaceIndex)
	{
		SpaceID spaceID;
		stream >> spaceID;

		insertSpaceIDQuery.execute( connection, spaceID, NULL );

		uint32 numData;
		stream >> numData;

		for (uint32 dataIndex = 0; dataIndex < numData; ++dataIndex)
		{
			uint64 spaceKey;
			uint16 dataKey;
			BW::string data;

			stream >> spaceKey;
			stream >> dataKey;
			stream >> data;

			insertSpaceDataQuery.execute( connection,
					spaceID, spaceKey, dataKey, data, NULL );
		}
	}

	return numSpaces;
}


/**
 *	This method writes the space data into the database.
 */
void WriteSpaceDataTask::performBackgroundTask( MySql & conn )
{
	static const Query delSpaceIDsQuery( "DELETE from bigworldSpaces" );
	static const Query delSpaceDataQuery( "DELETE from bigworldSpaceData" );

	delSpaceIDsQuery.execute( conn, NULL );
	delSpaceDataQuery.execute( conn, NULL );

	numSpaces_ = this->writeSpaceDataStreamToDB( conn, data_ );
}


/**
 *	This method is called if the background task fails and should be retried.
 */
void WriteSpaceDataTask::onRetry()
{
	data_.rewind();
}


/*
 *	This method returns the results back to the main thread.
 */
void WriteSpaceDataTask::performMainThreadTask( bool succeeded )
{
}

BW_END_NAMESPACE

// write_space_data_task.cpp
