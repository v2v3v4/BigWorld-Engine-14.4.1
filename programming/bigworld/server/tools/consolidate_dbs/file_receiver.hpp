#ifndef CONSOLIDATE_DBS_FILE_RECEIVER_HPP
#define CONSOLIDATE_DBS_FILE_RECEIVER_HPP

#include "msg_receiver.hpp"

#include "network/basictypes.hpp"
#include "network/endpoint.hpp"
#include "network/interfaces.hpp"

#include <cstdio>
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class FileReceiverMgr;

/**
 * 	Receives a secondary database file.
 */
class FileReceiver : public Mercury::InputNotificationHandler
{
public:
	FileReceiver( int socket, uint32 ip, uint16 port, FileReceiverMgr & mgr );
	virtual ~FileReceiver();

	const Mercury::Address & srcAddr() const
		{ return srcAddr_; }

	const BW::string & destPath() const
		{ return destPath_; }

	const BW::string & srcPath() const
		{ return srcPath_; }

	uint64 lastActivityTime() const 
		{ return lastActivityTime_; }

	bool deleteRemoteFile();
	bool deleteLocalFile();
	void abort();

	// Mercury::InputNotificationHandler override
	virtual int handleInputNotification( int fd );

private:
	size_t recvCommand();
	size_t recvSrcPathLen();
	size_t recvSrcPath();
	size_t recvFileLen();
	size_t recvFileContents();
	size_t recvErrorLen();
	size_t recvErrorStr();

	bool closeFile();

	typedef size_t (FileReceiver::*MessageProcessorFn)();

	Endpoint			endPoint_;
	FileReceiverMgr & 	mgr_;
	MsgReceiver			msgReceiver_;
	MessageProcessorFn	pMsgProcessor_;
	const char *		curActionDesc_;
	uint64				lastActivityTime_;
	Mercury::Address	srcAddr_;	// Cached for error info
	BW::string			srcPath_;
	BW::string			destPath_;
	uint32				expectedFileSize_;
	uint32				currentFileSize_;
	FILE *				destFile_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS_FILE_RECEIVER_HPP

