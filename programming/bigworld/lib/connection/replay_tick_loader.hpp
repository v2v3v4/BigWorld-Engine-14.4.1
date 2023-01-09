#ifndef REPLAY_TICK_LOADER_HPP
#define REPLAY_TICK_LOADER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include "connection/replay_data_file_reader.hpp"
#include "connection/replay_header.hpp"

BW_BEGIN_NAMESPACE

class IFileProvider;
typedef SmartPointer< IFileProvider > IFileProviderPtr;

class IReplayTickLoaderListener;

class ReplayController;
typedef SmartPointer< ReplayController > ReplayControllerPtr;

class ReplayTickData;
class TaskManager;


/**
 *	This class is for loading ticks from the temporary background file created
 *	for storing tick data.
 */
class ReplayTickLoader : public SafeReferenceCount
{
public:
	typedef int TickIndex;

	enum RequestType
	{
		NO_REQUEST,
		READ_HEADER,
		PREPEND,
		APPEND
	};

	enum ErrorType
	{
		ERROR_NONE,
		ERROR_KEY_ERROR,
		ERROR_FILE_MISSING,
		ERROR_FILE_SIGNATURE_MISMATCH,
		ERROR_FILE_CORRUPTED
	};

	ReplayTickLoader( TaskManager & taskManager,
		IReplayTickLoaderListener * pListener,
		const BW::string & replayFilePath,
		const BW::string & verifyingKey,
		IFileProviderPtr pFileProvider = NULL );

	/** Destructor. */
	~ReplayTickLoader() {}

	void addHeaderRequest();
	void addRequest( RequestType type, TickIndex start, TickIndex end );

	const BW::string & replayFilePath() const 	{ return replayFilePath_; }

	RequestType nextQueued() const 				{ return requestType_; }

	void onListenerDestroyed( IReplayTickLoaderListener * pListener );

	// Callbacks from ReplayTickLoaderTask.

	void onReplayTickLoaderTaskError( ErrorType errorType,
		const BW::string & errorString );

	void onReplayTickLoaderTaskFinished( RequestType type,
		GameTime startTime, 
		TickIndex start, ReplayTickData * pStart,
		TickIndex end, ReplayTickData * pEnd,
		uint numTicksRetrieved,
		const ReplayDataFileReader & reader );

private:
	void scheduleNewTask();

	TaskManager & 			taskManager_;

	IReplayTickLoaderListener * pListener_;
	IFileProviderPtr		pFileProvider_;
	BW::string 				replayFilePath_;

	BW::string				verifyingKey_;
	size_t 					verifiedToPosition_;

	RequestType 			requestType_;
	TickIndex 				requestStart_;
	TickIndex 				requestEnd_;

	bool 					isRegisteredForBackground_;
};

typedef SmartPointer< ReplayTickLoader > ReplayTickLoaderPtr;


/**
 *	This is the callback interface for ReplayTickLoader.
 */
class IReplayTickLoaderListener
{
public:
	/** Destructor. */
	virtual ~IReplayTickLoaderListener() {}


	/**
	 *	This method is called when the tick loader has read the replay file
	 *	header.
	 */
	virtual void onReplayTickLoaderReadHeader( const ReplayHeader & header,
		GameTime firstGameTime ) = 0;


	/**
	 *	This method is called when the tick loader has completed a PREPEND
	 *	operation. The ownership of the linked list of replay tick data 
	 *	passes to this object.
	 *
	 *	@param pStart 		The start of the linked list of replay tick data.
	 *	@param pEnd 		The end of the linked list of replay tick data.
	 *	@param totalTicks 	The number of ticks in the the list.
	 */
	virtual void onReplayTickLoaderPrependTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, uint totalTicks ) = 0;


	/**
	 *	This method is called when the tick loader has completed an APPEND
	 *	operation. The ownership of the linked list of replay tick data 
	 *	passes to this object.
	 *
	 *	@param pStart 		The start of the linked list of replay tick data.
	 *	@param pEnd 		The end of the linked list of replay tick data.
	 *	@param totalTicks 	The number of ticks in the the list.
	 */
	virtual void onReplayTickLoaderAppendTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, uint totalTicks ) = 0;


	/**
	 *	This method is called when the tick loader encounters an error while
	 *	reading from the replay file.
	 *
	 *	@param errorType 	One of the ReplayTickLoader::ErrorType values.
	 *	@param errorString 	The error description.
	 */
	virtual void onReplayTickLoaderError( ReplayTickLoader::ErrorType errorType,
		const BW::string & errorString ) = 0;
};


/**
 *	This class is used for mocking file operations when testing
 *	ReplayTickLoader.
 *
 *	TODO: Generalise and move to unit test lib.
 */
class IFileProvider : public SafeReferenceCount
{
public:

	/** Destructor. */
	virtual ~IFileProvider() {}

	/**
	 *	This method implements the fopen() operation.
	 *
	 *	@param path 	The file path.
	 *	@param mode 	The file mode to open with.
	 *	@return true on success, false otherwise.
	 */
	virtual bool FOpen( const char * path, const char * mode ) = 0;


	/**
	 *	This method implements the fclose() operation.
	 */
	virtual void FClose() = 0;

	/**
	 *	This method implements the fread() operation.
	 *
	 *	@param buffer 		The buffer to read into.
	 *	@param memberSize 	The size of the data member(s) to read.
	 *	@param numMembers	The number of members to read.
	 *
	 *	@return the result of the fread() operation.
	 */
	virtual size_t FRead( char * buffer, size_t memberSize, 
		size_t numMembers ) = 0;

	
	/**
	 *	This method implements the feof() operation.
	 *
	 *	@return the result of the feof() operation.
	 */
	virtual int FEOF() = 0;


	/**
	 *	This method implements the ferror() operation.
	 *
	 *	@return the result of the ferror() operation.
	 */
	virtual int FError() = 0;


	/**
	 *	This method implements the clearerr() operation.
	 */
	virtual void ClearErr() = 0;


protected:
	/** Constructor. */
	IFileProvider() : SafeReferenceCount() 
	{}
};

typedef SmartPointer< IFileProvider > IFileProviderPtr;


BW_END_NAMESPACE


#endif // REPLAY_TICK_LOADER_HPP

