#ifndef REPLAY_DATA_FILE_READER_HPP
#define REPLAY_DATA_FILE_READER_HPP

#include "cstdmf/bw_string.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class ReplayDataFileReader;
class ReplayHeader;
class ReplayMetaData;

/**
 *	This interface defines methods to be called back by a ReplayDataFileReader.
 */
class IReplayDataFileReaderListener
{
public:
	enum ErrorType
	{
		ERROR_NONE = 0,
		ERROR_KEY_ERROR,
		ERROR_SIGNATURE_MISMATCH,
		ERROR_FILE_CORRUPTED
	};

	/** Destructor. */
	virtual ~IReplayDataFileReaderListener() {}


	static const char * errorTypeToString( int errorType );


	/**
	 *	This method is called when the replay file header has been read from
	 *	the file.
	 *
	 *	@param reader 		The reader object.
	 *	@param header 		The header.
	 *	@param errorString	If this listener decides to raise an error, false
	 *						should be returned, and this parameter should be
	 *						filled with a description of the error.
	 *	@return 			Whether an error has been raised (and further
	 *						reading should stop).
	 */
	virtual bool onReplayDataFileReaderHeader(
		ReplayDataFileReader & reader,
		const ReplayHeader & header,
		BW::string & errorString ) = 0;


	/**
	 *	This method is called when the meta-data block has been read. The
	 *	listener can elect to stop further reading based on the meta-data.
	 *
	 *	@param reader 		The reader object.
	 *	@param metaData 	The meta-data.
	 *	@param errorString 	If this listener decides to raise an error, false
	 *						should be returned, and this parameter should be
	 *						filled with a description of the error.
	 *
	 *	@return	 			Whether an error has besen raised (and further
	 *						reading should stop).
	 *
	 */
	virtual bool onReplayDataFileReaderMetaData( ReplayDataFileReader & reader,
		const ReplayMetaData & metaData,
		BW::string & errorString ) = 0;


	/**
	 *	This method is called when a game tick's data has been read from the
	 *	file.
	 *
	 *	@param reader 			The reader object.
	 *	@param time 			The game time.
	 *	@param isCompressed 	Whether the data is compressed or not. If
	 *							compressed, the tick data can be recovered
	 *							using a CompressionIStream.
	 *	@param stream 			The tick data stream.
	 *	@param errorString		If this listener decides to raise an error,
	 *							false should be returned, and this parameter
	 *							should be filled with a description of the
	 *							error.
	 *	@return 				Whether an error has been raised (and further
	 *							reading should stop).
	 */
	virtual bool onReplayDataFileReaderTickData( ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString ) = 0;


	/**
	 *	This method is called when an error occurs while reading a replay file.
	 *	This does not include errors signaled from the other callback methods.
	 *
	 *	@param reader 		The reader object.
	 *	@param errorType 	The error type code, one of the ErrorType enum
	 *						values.
	 *	@param errorDescription
	 *						The error description.
	 */
	virtual void onReplayDataFileReaderError( ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription ) = 0;


	/**
	 *	This method is called when the reader destructs.
	 *
	 *	@param reader 	The reader.
	 */
	virtual void onReplayDataFileReaderDestroyed(
		ReplayDataFileReader & reader ) = 0;


protected:
	/** Constructor. */
	IReplayDataFileReaderListener() {}
};


/**
 *	This class is used to read a replay data file from blobs of data read from
 *	a file or downloaded from network.
 */
class ReplayDataFileReader
{
public:
	ReplayDataFileReader( IReplayDataFileReaderListener & listener,
		const BW::string & verifyingKey,
		bool shouldDecompress = true,
		size_t verifyFromPosition = 0U );

	~ReplayDataFileReader();

	bool addData( BinaryIStream & data );
	bool addData( const void * data, size_t dataSize );
	void clearBuffer();
	void reset();

	bool hasError() const;
	const BW::string & lastError() const;
	void clearError();

	bool hasReadHeader() const;
	const ReplayHeader & header() const;

	bool hasReadMetaData() const;
	const ReplayMetaData & metaData() const;

	GameTime 	firstGameTime() const;
	uint 		numTicksRead() const;
	size_t 		numBytesRead() const;
	uint 		numChunksRead() const;
	size_t 		numBytesAdded() const;

	size_t 		verifiedToPosition() const;
	uint 		numChunksVerified() const;
	size_t 		numTicksFieldOffset() const;

private:
	class Impl;
	Impl * pImpl_;
};


BW_END_NAMESPACE


#endif // REPLAY_DATA_FILE_READER_HPP
