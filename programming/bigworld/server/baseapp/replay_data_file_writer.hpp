#ifndef REPLAY_DATA_FILE_WRITER_HPP
#define REPLAY_DATA_FILE_WRITER_HPP

#include "connection/replay_checksum_scheme.hpp"

#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/md5.hpp"

#include "network/basictypes.hpp"
#include "network/compression_type.hpp"

#include "recording_recovery_data.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class MemoryOStream;
class ReplayDataFileWriter;
class ReplayMetaData;


/**
 *	This class is used to accumulate game tick data.
 */
class TickReplayData
{
public:
	TickReplayData();
	~TickReplayData();

	void init( GameTime tick, uint numCells );
	void transferFromCell( BinaryIStream & data );
	void finalise( BWCompressionType compressionType );

	GameTime tick() const 			{ return tick_; }

	uint numExpectedCells() const 	{ return numExpectedCells_; }
	void numExpectedCells( uint newValue );

	uint numCellsLeft() const 		{ return numCellsLeft_; }
	BinaryIStream & data() 			{ return *pBlob_; }
	int size() const				{ return pBlob_->size(); }
	
	static int tickHeaderSize();
	static void streamTickHeader( BinaryOStream & stream, GameTime tick );

private:

	GameTime 		tick_;
	uint 			numExpectedCells_;
	uint 			numCellsLeft_;
	MemoryOStream * pBlob_;
};


/**
 *	This interface is used for handling events from a ReplayDataFileWriter
 *	instance.
 */
class ReplayDataFileWriterListener
{
public:
	/** Destructor. */
	virtual ~ReplayDataFileWriterListener() {}

	/**
	 *	This method is called when the given writer is destroyed.
	 */
	virtual void onReplayDataFileWriterDestroyed(
		ReplayDataFileWriter & writer ) = 0;

	/**
	 *	This method handles a file writing error in the given file writer.
	 */
	virtual void onReplayDataFileWritingError(
		ReplayDataFileWriter & writer ) = 0;

	/**
	 *	This method is called to handle when the file writing is completed.
	 */
	virtual void onReplayDataFileWritingComplete(
		ReplayDataFileWriter & writer ) = 0;
};


/**
 *	This class implements writing to a replay data file.
 */
class ReplayDataFileWriter : private BackgroundFileWriterListener
{
public:
	ReplayDataFileWriter( IBackgroundFileWriter * pWriter,
		ChecksumSchemePtr pChecksumScheme,
		BWCompressionType compressionType,
		const MD5::Digest & digest,
		uint numTicksToSign,
		const ReplayMetaData & metaData,
		const BW::string & nonce = BW::string( "" ) );

	ReplayDataFileWriter( IBackgroundFileWriter * pWriter,
		ChecksumSchemePtr pChecksumScheme,
		const RecordingRecoveryData & recoveryData );

	virtual ~ReplayDataFileWriter();


	void addTickData( GameTime tick, uint numCells, BinaryIStream & blob );

	void addListener( ReplayDataFileWriterListener * pListener );

	const BW::string & path() const { return path_; }

	bool isFinalising() const { return isFinalising_; }
	bool isClosed() const { return isFinalising_ && !pWriter_.get(); }

	void close( bool shouldFinalise = true );

	void finalise();

	bool hasError() const;
	BW::string errorString() const;

	BWCompressionType compressionType() const { return compressionType_; }
	uint numTicksToSign()				{ return numTicksToSign_; }

	GameTime numTicksWritten() const 	{ return numTicksWritten_; }
	GameTime lastTickWritten() const 	{ return lastTickWritten_; }
	GameTime lastTickPendingWrite() const { return lastTickPendingWrite_; }
	off_t lastChunkPosition() const 	{ return lastChunkPosition_; }
	uint lastChunkLength() const 		{ return lastChunkLength_; }

	const RecordingRecoveryData recoveryData() const;

	static bool existsForPath( const BW::string & path );
	static void closeAll( ReplayDataFileWriterListener * pListener,
		bool shouldFinalise = true );
	static bool haveAllClosed();


private:
	void commonInit();

	void finaliseCompletedTickData( TickReplayData & pTickData );
	void signAndWriteCompletedTickData( bool isLastChunk = false );
	void setReadOnly();
	void closeWriter();

	void notifyListenersOfError();
	void notifyListenersOfCompletion();
	void notifyListenersOfDestruction();

	// Overrides from BackgroundFileWriterListener
	virtual void onBackgroundFileWritingError(
		IBackgroundFileWriter & fileWriter );

	virtual void onBackgroundFileWritingComplete(
		IBackgroundFileWriter & fileWriter, long filePosition, int userArg );


	BWCompressionType				compressionType_;

	GameTime 						currentTick_;
	GameTime 						lastTickPendingWrite_;
	GameTime						lastTickWritten_;
	off_t 							lastChunkPosition_;
	uint							lastChunkLength_;
	MemoryOStream 					lastSignature_;
	uint							numTicksToSign_;

	ChecksumSchemePtr 				pChecksumScheme_;

	typedef BW::map< GameTime, TickReplayData > TickData;
	TickData 						bufferedTickData_;
	GameTime 						numTicksWritten_;

	BW::string 						path_;
	bool 							isFinalising_;
	IBackgroundFileWriterPtr 		pWriter_;

	typedef BW::vector< ReplayDataFileWriterListener * > Listeners;
	Listeners listeners_;

	typedef BW::map< BW::string, ReplayDataFileWriter * > Writers;
	static Writers s_activeWriters_;
};


BW_END_NAMESPACE


#endif // REPLAY_DATA_FILE_WRITER_HPP

