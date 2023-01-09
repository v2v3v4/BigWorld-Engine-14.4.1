#ifndef PY_REPLAY_DATA_FILE_WRITER_HPP
#define PY_REPLAY_DATA_FILE_WRITER_HPP

#include "cstdmf/md5.hpp"

#include "pyscript/pyobject_plus.hpp"

#include "replay_data_file_writer.hpp"


BW_BEGIN_NAMESPACE


class PyReplayDataFileWriterListener;


/*~ class NoModule.PyReplayDataFileWriter
 *	@components{ base }
 *
 *	This class is used to write replay data out to a file.
 *
 *	Tick data is queued on the writer to be written out to the output file
 *	using addTickData().
 *
 *	Once recording has finished, the file is finalised by calling finalise().
 *	If the object's reference count reaches zero before finalise() is called,
 *	it will automatically finalise itself.
 *
 *	Code Example:
 *	@{
 *	writer = BigWorld.createReplayDataFileWriter( path )
 *	writer.setListener( callback )
 *	writer.addTickData( gameTime, numCells, blob )
 *	@}
 */
/**
 *	This class exposes the ReplayDataFileWriter class to Python.
 */
class PyReplayDataFileWriter : public PyObjectPlus
{
	Py_Header( PyReplayDataFileWriter, PyObjectPlus )

public:
	PyReplayDataFileWriter( const BW::string & resourcePath,
		BackgroundFileWriter * pWriter,
		ChecksumSchemePtr pChecksumScheme,
		uint numTicksToSign,
		BWCompressionType compressionType,
		const MD5::Digest & digest,
		const ReplayMetaData & metaData,
		PyTypeObject * pType = &PyReplayDataFileWriter::s_type_ );

	PyReplayDataFileWriter( const BW::string & resourcePath,
		BackgroundFileWriter * pWriter,
		ChecksumSchemePtr pChecksumScheme,
		const RecordingRecoveryData & recoveryData,
		PyTypeObject * pType = &PyReplayDataFileWriter::s_type_ );

	~PyReplayDataFileWriter();


	/**
	 *	This method returns whether this writer is finalising.
	 */
	bool isFinalising() const
	{
		return pWriter_ ? pWriter_->isFinalising() : true;
	}


	/**
	 *	This method returns whether this writer is closed.
	 */
	bool isClosed() const
	{
		return pWriter_ ? pWriter_->isClosed() : true;
	}

	PY_RO_ATTRIBUTE_DECLARE( resourcePath_, resourcePath );

	PyObject * pyGet_path();
	PY_RO_ATTRIBUTE_SET( path );

	PY_RO_ATTRIBUTE_DECLARE( pWriter_->compressionType(), compressionType );

	PyObject * pyGet_numTicksWritten();
	PY_RO_ATTRIBUTE_SET( numTicksWritten );

	PyObject * pyGet_lastTickWritten();
	PY_RO_ATTRIBUTE_SET( lastTickWritten );

	PyObject * pyGet_lastTickPendingWrite();
	PY_RO_ATTRIBUTE_SET( lastTickPendingWrite );

	PyObject * pyGet_lastChunkPosition();
	PY_RO_ATTRIBUTE_SET( lastChunkPosition );

	PyObject * pyGet_lastChunkLength();
	PY_RO_ATTRIBUTE_SET( lastChunkLength );

	PyObject * pyGet_recoveryData();
	PY_RO_ATTRIBUTE_SET( recoveryData );

	PyObject * pyGet_numTicksToSign();
	PY_RO_ATTRIBUTE_SET( numTicksToSign );


	PY_RO_ATTRIBUTE_DECLARE( isFinalising(), isFinalising );
	PY_RO_ATTRIBUTE_DECLARE( isClosed(), isClosed );

	PY_RO_ATTRIBUTE_DECLARE( pWriter_->hasError(), hasError );

	PyObject * pyGet_errorString();
	PY_RO_ATTRIBUTE_SET( errorString )

	PY_METHOD_DECLARE( py_addTickData );
	PY_METHOD_DECLARE( py_setListener );
	PY_METHOD_DECLARE( py_close );
private:
	BW::string resourcePath_;
	ReplayDataFileWriter * pWriter_;

	PyReplayDataFileWriterListener * pListener_;
};


BW_END_NAMESPACE


#endif // PY_REPLAY_DATA_FILE_WRITER_HPP
