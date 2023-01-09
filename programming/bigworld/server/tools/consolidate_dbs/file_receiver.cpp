#include "file_receiver.hpp"

#include "file_receiver_mgr.hpp"
#include "file_transfer_progress_reporter.hpp"

#include "cstdmf/timestamp.hpp"

#include "network/event_dispatcher.hpp"

#include "resmgr/file_system.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: FileReceiver
// -----------------------------------------------------------------------------
/**
 *	Constructor.
 */
FileReceiver::FileReceiver( int socket, uint32 ip, uint16 port,
		FileReceiverMgr & mgr ) :
	mgr_( mgr ),
	msgReceiver_( 512 ),
	pMsgProcessor_( &FileReceiver::recvCommand ),
	curActionDesc_( "receive command" ),
	lastActivityTime_( timestamp() ),
	srcAddr_( ip, port ),
	destPath_( mgr.consolidationDir() ),
	expectedFileSize_( 0 ),
	currentFileSize_( 0 ),
	destFile_( NULL )
{
//	DEBUG_MSG( "FileReceiver::FileReceiver: Accepted incoming connection from "
//			"%s\n", srcAddr_.c_str() );

	endPoint_.setFileDescriptor( socket );
	endPoint_.setnonblocking( true );

	msgReceiver_.setMsgSize( sizeof( char ) );	 // To match recvCommand()

	mgr_.dispatcher().registerFileDescriptor( endPoint_.fileno(), this,
		"FileReceiver" );
}


/**
 *	Destructor
 */
FileReceiver::~FileReceiver()
{
	mgr_.dispatcher().deregisterFileDescriptor( endPoint_.fileno() );
}


/**
 * 	Mercury::InputNotificationHandler override
 */
int FileReceiver::handleInputNotification( int fd )
{
	lastActivityTime_ = timestamp();

	if (pMsgProcessor_ == NULL)
	{
		ERROR_MSG( "FileReceiverMgr::handleInputNotification: Receiving "
				"data but we are currently in %s mode\n", curActionDesc_ );
		return 0;
	}

	bool isMore;
	do
	{
		isMore = false;
		bool isOK = msgReceiver_.recvMsg( endPoint_ );
		if (isOK)
		{
			if (msgReceiver_.isDone())
			{
				size_t nextMsgSize = (this->*pMsgProcessor_)();
				if (pMsgProcessor_ != NULL)
				{
					bool wasAlwaysDone = msgReceiver_.isAlwaysDone();

					msgReceiver_.setMsgSize( nextMsgSize );

					// Prevent infinite loop when nextMsgSize is 0. In that
					// case, msgReceiver_.isDone() is always true.
					// NOTE: This is handling a few cases:
					// Case 1: Transition from not-always-done to always-done:
					//			- Do another loop.
					// Case 2: Remaining on always-done:
					//			- Don't do another loop.
					// Case 3: Transition from always-done to not-always-done:
					//			- Do another loop.
					// Case 4: Remaining on not-always-done:
					//			- Do another loop.
					isMore = !(msgReceiver_.isAlwaysDone() && wasAlwaysDone);
				}
			}
		}
		else
		{
			ERROR_MSG( "FileReceiver::handleInputNotification: "
					"Error communicating with file transfer process on %s. "
					"Failed to %s.\n", 
				srcAddr_.c_str(), curActionDesc_ );

			pMsgProcessor_ = NULL;
			curActionDesc_ = "wait for termination after error";
			mgr_.onFileReceiveError();
		}
	} while (isMore);

	return 0;
}


/**
 * 	Processes a "command" from the remote end.
 */
size_t FileReceiver::recvCommand()
{
	MF_ASSERT( msgReceiver_.msgLen() == sizeof(char) );

	const char* pCommand =
			reinterpret_cast< const char * >( msgReceiver_.msg() );

	size_t nextMsgSize;
	switch (*pCommand)
	{
		case 'n':
			nextMsgSize = sizeof( uint16 );
			pMsgProcessor_ = &FileReceiver::recvSrcPathLen;
			curActionDesc_ = "receive source file path length";
			break;
		case 'e':
			nextMsgSize = sizeof( uint16 );
			pMsgProcessor_ = &FileReceiver::recvErrorLen;
			curActionDesc_ = "receive error string length";
			break;
		default:
			ERROR_MSG( "FileReceiver::recvCommand: Received "
					"invalid command %c\n", *pCommand );
			nextMsgSize = 0;
			pMsgProcessor_ = NULL;
			curActionDesc_ = "wait for termination after error";
			mgr_.onFileReceiveError();
			break;
	}

	return nextMsgSize;
}


/**
 * 	Receive the length of the source file's path.
 */
size_t FileReceiver::recvSrcPathLen()
{
	MF_ASSERT( msgReceiver_.msgLen() == sizeof( uint16 ) );

	uint16 	pathLen;
	memcpy( &pathLen, msgReceiver_.msg(), sizeof( pathLen ) );

	pMsgProcessor_ = &FileReceiver::recvSrcPath;
	curActionDesc_ = "receive source file path";

	return pathLen;
}


/**
 * 	Receives the source file's path.
 */
size_t FileReceiver::recvSrcPath()
{
	srcPath_.assign( reinterpret_cast< const char * >( msgReceiver_.msg() ),
			msgReceiver_.msgLen() );

	// Extract file name and add to destPath_
	BW::string::size_type pos = srcPath_.find_last_of( '/' );
	if (pos == BW::string::npos)
		pos = 0;
	destPath_.insert( destPath_.end(), srcPath_.begin() + pos + 1,
			srcPath_.end() );

	// Check that file doesn't already exist and append suffix if it does.
	BW::string origDestPath = destPath_;
	int			suffix = 1;
	while (NativeFileSystem::getAbsoluteFileType( destPath_ ) !=
		IFileSystem::FT_NOT_FOUND)
	{
		++suffix;

		BW::stringstream ss;
		ss << origDestPath << '-' << suffix;
		destPath_ = ss.str();
	}
	if (destPath_ != origDestPath)
	{
		WARNING_MSG( "FileReceiver::recvSrcPath: Default destination file '%s' "
				"already exists. Saving to new destination '%s'\n",
				origDestPath.c_str(), destPath_.c_str() );
	}

	MF_ASSERT( destFile_ == NULL );

	// Open file
	size_t nextMsgSize = sizeof( expectedFileSize_ );
	destFile_ = fopen( destPath_.c_str(), "wb" );
	if (destFile_ != NULL)
	{
		pMsgProcessor_ = &FileReceiver::recvFileLen;
		curActionDesc_ = "receive file length";
	}
	else
	{
		ERROR_MSG( "FileReceiverMgr::recvSrcFileInfo: Failed to create local "
				"file '%s'\n", destPath_.c_str() );
		pMsgProcessor_ = NULL;
		curActionDesc_ = "wait for termination after error";
		nextMsgSize = 0;
		mgr_.onFileReceiveError();
	}

	return nextMsgSize;
}


/**
 *	Receives the file length from the socket.
 */
size_t FileReceiver::recvFileLen()
{
	MF_ASSERT( msgReceiver_.msgLen() == sizeof( expectedFileSize_ ) );

	memcpy( &expectedFileSize_, msgReceiver_.msg(), sizeof(expectedFileSize_) );

	if (expectedFileSize_ > 0)
	{
		TRACE_MSG( "FileReceiver::recvFileLen: Receiving data for "
				"file '%s' of size %u from '%s' on %s\n", 
			destPath_.c_str(),
			expectedFileSize_, srcPath_.c_str(), srcAddr_.c_str() );
		pMsgProcessor_ = &FileReceiver::recvFileContents;
		curActionDesc_ = "receive file contents";

		mgr_.progressReporter().onStartTransfer( expectedFileSize_ );
	}
	else
	{
		ERROR_MSG( "FileReceiverMgr::recvSrcFileInfo: Received empty file "
				"'%s'\n", destPath_.c_str() );
		pMsgProcessor_ = NULL;
		curActionDesc_ = "wait for termination after error";
		mgr_.onFileReceiveError();
	}

	return 0;
}


/**
 *	Receives the file contents and write it to the output file.
 */
size_t FileReceiver::recvFileContents()
{
	size_t numToWrite = msgReceiver_.msgLen();
	if (numToWrite == 0)
	{
		return 0;
	}

	int success = fwrite( msgReceiver_.msg(), numToWrite, 1, destFile_ );
	if (success == 0)
	{
		ERROR_MSG( "FileReceiverMgr::recvFileContents: Failed to write to "
				"file '%s'\n", destPath_.c_str() );
		pMsgProcessor_ = NULL;
		curActionDesc_ = "wait for termination after error";
		mgr_.onFileReceiveError();
		return 0;
	}

	mgr_.progressReporter().onReceiveData( numToWrite );

	currentFileSize_ += numToWrite;

	if (currentFileSize_ >= expectedFileSize_)
	{
		// TODO: Make sure we don't read more than necessary.
		MF_ASSERT( currentFileSize_ == expectedFileSize_ );

		this->closeFile();
		pMsgProcessor_ = NULL;
		curActionDesc_ = "wait for termination after finished";

		mgr_.onFileReceived( *this );
	}

	return 0;
}


/**
 *	Receives error string length from the socket.
 */
size_t FileReceiver::recvErrorLen()
{
	uint16 errorLen;
	memcpy( &errorLen, msgReceiver_.msg(), sizeof( errorLen ) );

	pMsgProcessor_ = &FileReceiver::recvErrorStr;
	curActionDesc_ = "receive error string";

	return errorLen;
}


/**
 *	Reads the error string from the socket.
 */
size_t FileReceiver::recvErrorStr()
{
	BW::string	errorStr( reinterpret_cast< const char * >(msgReceiver_.msg()),
			msgReceiver_.msgLen() );
	ERROR_MSG( "FileReceiver::recvErrorStr: "
			"Database transfer utility on %s reported error: %s\n",
		srcAddr_.c_str(), errorStr.c_str() );

	pMsgProcessor_ = NULL;
	curActionDesc_ = "wait for termination after error";

	mgr_.onFileReceiveError();

	return 0;
}


/**
 *	Closes the local file that we're writing to.
 */
bool FileReceiver::closeFile()
{
	bool isOK = true;
	if (fclose( destFile_ ) != 0)
	{
		char errnoBuf[1024];
		ERROR_MSG( "FileReceiverMgr::closeFile: Failed to close file '%s': %s",
				destPath_.c_str(),
				strerror_r( errno, errnoBuf, sizeof( errnoBuf ) ) );
		isOK = false;
	}
	destFile_ = NULL;

	return isOK;
}


/**
 *	Deletes the remote file.
 */
bool FileReceiver::deleteRemoteFile()
{
	bool isOK = true;

	// Tell remote end to delete their file.
	// TODO: Wait for acknowledgement?
	if (endPoint_.send( "d", 1 ) != 1)
	{
		char errnoBuf[1024];

		ERROR_MSG( "FileReceiver::deleteRemoteFile: Failed to delete file "
				"'%s' on %s: %s\n", 
			srcPath_.c_str(), srcAddr_.c_str(),
			strerror_r( errno, errnoBuf, sizeof( errnoBuf ) ) );

		isOK = false;
	}

	return isOK;
}


/**
 *	Deletes the local (transferred) file.
 */
bool FileReceiver::deleteLocalFile()
{
	if (destPath_ == mgr_.consolidationDir())
	{
		return true;	// Nothing to delete.
	}

	bool isOK = true;
	if (remove( destPath_.c_str() ) != 0)
	{
		char errnoBuf[1024];

		ERROR_MSG( "FileReceiver::deleteLocalFile: Failed to delete local file "
				"'%s': %s\n", destPath_.c_str(),
				strerror_r( errno, errnoBuf, sizeof( errnoBuf ) ) );

		isOK = false;
	}

	return isOK;
}


/**
 *	Abort file transfer.
 */
void FileReceiver::abort()
{
	if (pMsgProcessor_ != NULL)
	{
		pMsgProcessor_ = NULL;
		curActionDesc_ = "abort file transfer";
	}

	if (destFile_)
	{
		this->closeFile();
	}
}

BW_END_NAMESPACE

// file_receiver.cpp
