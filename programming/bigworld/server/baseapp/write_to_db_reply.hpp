#ifndef WRITE_TO_DB_REPLY_HPP
#define WRITE_TO_DB_REPLY_HPP

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class handles a reply from the DBApp.
 */
class WriteToDBReplyHandler
{
public:
	virtual ~WriteToDBReplyHandler() {};
	virtual void onWriteToDBComplete( bool succeeded ) = 0;
};


/**
 *	This class calls the reply handler once the entity has been written
 *	to disk, and optionally, backed up to it's backup baseapp.
 */
class WriteToDBReplyStruct : public ReferenceCount
{
public:
	WriteToDBReplyStruct( WriteToDBReplyHandler * pHandler );
	bool expectsReply()	const { return pHandler_ != NULL; }
	void onWriteToDBComplete( bool succeeded );
	void onBackupComplete();

private:
	bool succeeded_;
	bool backedUp_;
	bool writtenToDB_;
	WriteToDBReplyHandler * pHandler_;
};

typedef SmartPointer<WriteToDBReplyStruct> WriteToDBReplyStructPtr;

BW_END_NAMESPACE

#endif // WRITE_TO_DB_REPLY_HPP
