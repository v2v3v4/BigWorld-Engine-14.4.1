#include "script/first_include.hpp"

#include "py_replay_data_file_reader.hpp"
#include "py_replay_header.hpp"

#include "connection/replay_metadata.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.PyReplayDataFileReader
 *	@components{ base }
 *
 *	This class is used to parse a replay data file, working on sequential blobs
 *	of data read from a replay file, and outputting the header and tick data to
 *	a listener callback object. It can optionally verify the data using a
 *	elliptic curve public key in PEM format.
 *
 *	Additionally, it provides data on the number of ticks read from the added
 *	data blobs.
 */
PY_TYPEOBJECT( PyReplayDataFileReader )

PY_BEGIN_METHODS( PyReplayDataFileReader )
	/*~ function PyReplayDataFileReader.addData
	 *	@components{ base }
	 *
	 *	This method adds a blob of data to be processed. Calls to this method
	 *	may call back on the listener callback object.
	 *
	 *	@return None on success, else an appropriate exception will be raised.
	 */
	PY_METHOD( addData )

	/*~ function PyReplayDataFileReader.reset
	 *	@components{ base }
	 *
	 *	This method resets the reader state to accept blobs from the start of a
	 *	replay file. An exception is the attribute verifiedToPosition does not
	 *	change, so that this persists when re-reading the same replay file. If
	 *	reading new files, a new instance of ReplayDataFileReader should be
	 *	created.
	 */
	PY_METHOD( reset )

	/*~ function PyReplayDataFileReader.clearBuffer
	 *	@components{ base }
	 *
	 *	This method clears any buffered data, resetting the state of the reader
	 *	to the location of the last fully parsed position (that is,
	 *	numBytesAdded will be reset to numBytesRead).
	 */
	PY_METHOD( clearBuffer )

	/*~ function PyReplayDataFileReader.clearError
	 *	@components{ base }
	 *
	 *	This method clears out any error, allowing for addData() to be called 
	 *	again with the same reader state as after the last successful call to
	 *	addData().
	 */
	PY_METHOD( clearError )
PY_END_METHODS()


PY_BEGIN_ATTRIBUTES( PyReplayDataFileReader )
	/*~ attribute PyReplayDataFileReader.header
	 *	@components{ base }
	 *
	 *	This attribute returns the header if one has been read, or None
	 *	otherwise.
	 */
	PY_ATTRIBUTE( header )

	/*~ attribute PyReplayDataFileReader.metaData
	 *	@components{ base }
	 *
	 *	This attribute returns the meta-data as a dictionary.
	 */
	PY_ATTRIBUTE( metaData )

	/*~ attribute PyReplayDataFileReader.listener
	 *	@components{ base }
	 *
	 *	This can be set to an object that has attribute functions as below:
	 *
	 *	onReplayDataFileReaderHeader( reader, header )
	 *	onReplayDataFileReaderMetaData( reader, metaData )
	 *	onReplayDataFileReaderTickData( reader, gameTime, isCompressed, data )
	 *	onReplayDataFileReaderError( reader, errorDescription )
	 *
	 *	The onReplayDataFileReaderHeader() and onReplayDataFileReaderTickData()
	 *	methods can return a non-None object, which is interpreted as a bool.
	 *	If an exception occurs during these callbacks, or they return a
	 *	non-None value equivalent to False, then the reader is placed in an
	 *	error state, and must be reset before data can be added again using
	 *	addData().
	 */
	PY_ATTRIBUTE( listener )

	/*~ attribute PyReplayDataFileReader.firstGameTime
	 *	@components{ base }
	 *
	 *	This attribute gives the game time of the first tick read from the
	 *	file, or 0 if no tick has been read yet.
	 */
	PY_ATTRIBUTE( firstGameTime )

	/*~ attribute PyReplayDataFileReader.numTicksRead
	 *	@components{ base }
	 *
	 *	This attribute gives the number of ticks read from the file. This does
	 *	not include data that has been buffered but not processed yet.
	 */
	PY_ATTRIBUTE( numTicksRead )

	/*~ attribute PyReplayDataFileReader.numBytesRead
	 *	@components{ base }
	 *
	 *	This attribute gives the number of bytes read from the file. This does
	 *	not include data that has been buffered but not processed yet.
	 */
	PY_ATTRIBUTE( numBytesRead )

	/*~ attribute PyReplayDataFileReader.numChunksRead
	 *	@components{ base }
	 *
	 *	This attribute gives the number of signed chunks that have been read.
	 */
	PY_ATTRIBUTE( numChunksRead )

	/*~ attribute PyReplayDataFileReader.numBytesAdded
	 *	@components{ base }
	 *
	 *	This attribute gives the number of bytes that have been added via
	 *	addData().
	 */
	PY_ATTRIBUTE( numBytesAdded )

	/*~ attribute PyReplayDataFileReader.verifiedToPosition
	 *	@components{ base }
	 *
	 *	This attribute gives the file position to which the replay file data
	 *	has been verified. If verification is enabled this will grow as data is
	 *	added.
	 */
	PY_ATTRIBUTE( verifiedToPosition )

	/*~ attribute PyReplayDataFileReader.numChunksVerified
	 *	@components{ base }
	 *
	 *	This attribute gives the number of signed chunks that have been read
	 *	and verified.
	 */
	PY_ATTRIBUTE( numChunksVerified )

	/*~ attribute PyReplayDataFileReader.numTicksFieldOffset
	 *	@components{ base }
	 *
	 *	This attribute gives the offset of the number of ticks field in the
	 *	replay file header.
	 */
	PY_ATTRIBUTE( numTicksFieldOffset )


	/*~ attribute PyReplayDataFileReader.lastError
	 *	@components{ base }
	 *
	 *	This attributes gives the string description of the last error, or None
	 *	if there is no error.
	 */
	PY_ATTRIBUTE( lastError )

PY_END_ATTRIBUTES()


PY_FACTORY_NAMED( PyReplayDataFileReader, "ReplayDataFileReader", BigWorld )


/**
 *	Constructor.
 *
 *	@param verifyingKey 		The elliptic curve verifying key, in PEM format.
 *								This can be empty, in which case no
 *								verification occurs.
 *	@param shouldDecompress 	Whether the data should be decompressed.
 *	@param verifyFromPosition 	If verifying, the file position to verify from.
 *	@param pType 				The Python type object.
 */
PyReplayDataFileReader::PyReplayDataFileReader( 
			const BW::string & verifyingKey, bool shouldDecompress, 
			size_t verifyFromPosition, PyTypeObject * pType ) :
		PyObjectPlus( pType ),
		IReplayDataFileReaderListener(),
		reader_( *this, verifyingKey, shouldDecompress, verifyFromPosition ),
		pListener_( ScriptObject::none() )
{
}


/**
 *	Destructor.
 */
PyReplayDataFileReader::~PyReplayDataFileReader()
{
}


/** 
 *	This method implements the header attribute.
 */
PyObject * PyReplayDataFileReader::pyGet_header()
{
	if (!reader_.hasReadHeader())
	{
		Py_RETURN_NONE;
	}
	return new PyReplayHeader( reader_.header() );
}


/**
 *	This method implements the metaData attribute.
 */
PyObject * PyReplayDataFileReader::pyGet_metaData()
{
	if (!reader_.hasReadMetaData())
	{
		Py_RETURN_NONE;
	}

	return this->metaDataAsScriptDict().get();
}


/**
 *	This method converts the meta-data collection on the reader to a
 *	ScriptDict.
 */
ScriptDict PyReplayDataFileReader::metaDataAsScriptDict() const
{
	const ReplayMetaData & metaData = reader_.metaData();
	ScriptDict metaDataDict = ScriptDict::create( metaData.size() );

	ReplayMetaData::const_iterator iter = metaData.begin();
	while (iter != metaData.end())
	{
		metaDataDict.setItem( iter->first.c_str(),
			ScriptString::create( iter->second ),
			ScriptErrorPrint() );
		++iter;
	}

	return metaDataDict;
}


/**
 *	This method implements the listener attribute getter.
 */
PyObject * PyReplayDataFileReader::pyGet_listener()
{
	return pListener_.newRef();
}


/**
 *	This method implements the listener attribute setter.
 */
int PyReplayDataFileReader::pySet_listener( PyObject * pListener )
{
	pListener_ = pListener;
	return 0;
}


/**
 *	This method implements the lastError attribute getter.
 */
PyObject * PyReplayDataFileReader::pyGet_lastError()
{
	if (!reader_.hasError())
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( reader_.lastError() );
}


/**
 *	This method implements the addData method.
 */
bool PyReplayDataFileReader::addData( const BW::string & data )
{
	if (reader_.hasError())
	{
		PyErr_SetString( PyExc_ValueError, "Reader in error state" );
		return false;
	}

	if (!reader_.addData( data.data(), data.size() ))
	{
		if (!PyErr_Occurred())
		{
			PyErr_SetString( PyExc_ValueError, reader_.lastError().c_str() );
		}
		return false;
	}

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool PyReplayDataFileReader::onReplayDataFileReaderHeader(
		ReplayDataFileReader & reader,
		const ReplayHeader & header,
		BW::string & errorString )
{
	ScriptObject returnValue = this->callListenerCallback( 
		"onReplayDataFileReaderHeader",
		ScriptArgs::create( this,
			ScriptObject( new PyReplayHeader( header ),
				ScriptObject::FROM_NEW_REFERENCE ) ),
		&errorString );

	if (!returnValue.exists())
	{
		return false;
	}
	else if (returnValue.exists() && 
			!returnValue.isNone() && 
			!returnValue.isTrue( ScriptErrorPrint() ))
	{
		errorString = "User callback rejected header";
		return false;
	}

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool PyReplayDataFileReader::onReplayDataFileReaderMetaData(
		ReplayDataFileReader & reader,
		const ReplayMetaData & metaData,
		BW::string & errorString )
{
	ScriptObject returnValue = this->callListenerCallback(
		"onReplayDataFileReaderMetaData",
		ScriptArgs::create( this, this->metaDataAsScriptDict() ),
		&errorString );

	if (!returnValue.exists())
	{
		return false;
	}
	else if (returnValue.exists() &&
			!returnValue.isNone() &&
			!returnValue.isTrue( ScriptErrorPrint() ))
	{
		errorString = "User callback rejected meta-data";
		return false;
	}

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool PyReplayDataFileReader::onReplayDataFileReaderTickData(
		ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString )
{
	int streamSize = stream.remainingLength();

	BW::string tickData( 
		static_cast<const char * >( stream.retrieve( streamSize ) ),
		streamSize );

	ScriptObject returnValue = this->callListenerCallback( 
		"onReplayDataFileReaderTickData",
		ScriptArgs::create( this, time, isCompressed, tickData ),
		&errorString );
	
	if (!returnValue.exists())
	{
		errorString = "User callback raised an exception";
		return false;
	}
	else if (returnValue.exists() && 
			!returnValue.isNone() && 
			!returnValue.isTrue( ScriptErrorClear() ))
	{
		errorString = "User callback rejected tick data";
		return false;
	}

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
void PyReplayDataFileReader::onReplayDataFileReaderError( 
		ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription )
{
	this->callListenerCallback( "onReplayDataFileReaderError",
		ScriptArgs::create( this, errorDescription ), /* pErrorString */ NULL );
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
void PyReplayDataFileReader::onReplayDataFileReaderDestroyed(
		ReplayDataFileReader & reader )
{
	// This is empty on purpose - since we own the reader, we know when it's
	// destroyed.
}


/**
 *	This method calls the given listener callback with the given arguments.
 *
 *	If an exception occurs, it will be printed to the logging system. A
 *	description of the exception can be captured.
 *
 *	@param methodName 		The name of the method to call on the listener
 *							object.
 *	@param args				The arguments to use in the method call.
 *	@param pErrorString 	If set, and an exception occurs during callback,
 *							this is filled with a description of the exception.
 *
 *	@return 				The return value, or an empty ScriptObject if an
 *							exception occurred. 
 */
ScriptObject PyReplayDataFileReader::callListenerCallback( 
		const char * methodName, const ScriptArgs & args,
		BW::string * pErrorString )
{
	if (pListener_.isNone())
	{
		return ScriptObject::none();
	}

	if (!pListener_.hasAttribute( methodName ))
	{
		return ScriptObject::none();
	}

	ScriptObject returnValue = pListener_.callMethod( methodName, args, 
		ScriptErrorRetain(),
		/* allowNullMethod */ true );

	return returnValue;
}


/**
 *	This method implements the tp_repr and tp_str function slots of the
 *	PyReplayDataFileReader object.
 */
PyObject * PyReplayDataFileReader::pyRepr() const
{
	BW::string headerString( "(not read)" );

	if (reader_.hasReadHeader())
	{
		ScriptObject pHeader( new PyReplayHeader( reader_.header() ),
			ScriptObject::FROM_NEW_REFERENCE ); 
		pHeader.str( ScriptErrorPrint() ).getString( headerString );
	}

	return PyString_FromFormat( "<ReplayDataFileReader header: %s; "
			"read/added: %zu / %zu>", 
		headerString.c_str(), 
		reader_.numBytesRead(),
		reader_.numBytesAdded() );
}


BW_END_NAMESPACE


// py_replay_data_file_reader.cpp
