#ifndef PY_REPLAY_DATA_FILE_READER
#define PY_REPLAY_DATA_FILE_READER


#include "cstdmf/bw_string.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "script/py_script_object.hpp"

#include "connection/replay_data_file_reader.hpp"


BW_BEGIN_NAMESPACE

class PyReplayDataFileReader;


/**
 *	This class is used to expose the ReplayDataFileReader class to Python
 *	script.
 */
class PyReplayDataFileReader : public PyObjectPlus,
		private IReplayDataFileReaderListener
{
	Py_Header( PyReplayDataFileReader, PyObjectPlus )

public:
	PyReplayDataFileReader( const BW::string & verifyingKey,
		bool shouldDecompress, size_t verifyFromPosition,
		PyTypeObject * pType = &s_type_ );

	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE(
		PyReplayDataFileReader, 
		OPTARG( BW::string, "", 
		OPTARG( bool, true, 
		OPTARG( size_t, 0, END ))));

	virtual ~PyReplayDataFileReader();

	PyObject * pyGet_header();
	PY_RO_ATTRIBUTE_SET( header );

	PyObject * pyGet_metaData();
	PY_RO_ATTRIBUTE_SET( metaData );

	int pySet_listener( PyObject * pListener );
	PyObject * pyGet_listener();

	PY_RO_ATTRIBUTE_DECLARE( reader_.firstGameTime(), firstGameTime )
	PY_RO_ATTRIBUTE_DECLARE( reader_.numTicksRead(), numTicksRead )
	PY_RO_ATTRIBUTE_DECLARE( reader_.numBytesRead(), numBytesRead )
	PY_RO_ATTRIBUTE_DECLARE( reader_.numChunksRead(), numChunksRead )
	PY_RO_ATTRIBUTE_DECLARE( reader_.numBytesAdded(), numBytesAdded )
	PY_RO_ATTRIBUTE_DECLARE( reader_.verifiedToPosition(), verifiedToPosition )
	PY_RO_ATTRIBUTE_DECLARE( reader_.numChunksVerified(), numChunksVerified )
	PY_RO_ATTRIBUTE_DECLARE( reader_.numTicksFieldOffset(),
		numTicksFieldOffset )

	PyObject * pyGet_lastError();
	PY_RO_ATTRIBUTE_SET( lastError );

	bool addData( const BW::string & data );
	PY_AUTO_METHOD_DECLARE( RETOK, addData, ARG( BW::string, END ) );

	void reset() { reader_.reset(); }
	PY_AUTO_METHOD_DECLARE( RETVOID, reset, END );

	void clearBuffer() { reader_.clearBuffer(); }
	PY_AUTO_METHOD_DECLARE( RETVOID, clearBuffer, END );

	void clearError() { reader_.clearError(); }
	PY_AUTO_METHOD_DECLARE( RETVOID, clearError, END );

	PyObject * pyRepr() const;

private:
	ScriptObject callListenerCallback( const char * methodName, 
		const ScriptArgs & args, BW::string * pErrorString );

	ScriptDict metaDataAsScriptDict() const;


	// Overrides from IReplayDataFileReaderListener
	virtual bool onReplayDataFileReaderHeader(
		ReplayDataFileReader & reader,
		const ReplayHeader & header,
		BW::string & errorString );

	virtual bool onReplayDataFileReaderTickData( ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString );

	virtual bool onReplayDataFileReaderMetaData( ReplayDataFileReader & reader,
		const ReplayMetaData & metaData,
		BW::string & errorString );

	virtual void onReplayDataFileReaderError( ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription );

	virtual void onReplayDataFileReaderDestroyed(
		ReplayDataFileReader & reader );


	ReplayDataFileReader 	reader_;
	ScriptObject 			pListener_;
};

BW_END_NAMESPACE

#endif
