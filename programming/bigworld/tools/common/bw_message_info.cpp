#include "pch.hpp"
#include "bw_message_info.hpp"

#include "appmgr/commentary.hpp"

#include "cstdmf/debug_filter.hpp"
#include "cstdmf/debug_message_callbacks.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_item.hpp"

#include "chunk/editor_chunk_item.hpp"

#define MAX_INFO_MESSAGES 1024


BW_BEGIN_NAMESPACE

void BWMessageInfo::registerItem()
{
	BW_GUARD;

	ChunkItem * tmp = realItem();
	if (tmp)
	{
		tmp->recordMessage( this );
	}
}


void BWMessageInfo::unregisterItem()
{
	BW_GUARD;

	ChunkItem * tmp = realItem();
	if (tmp)
	{
		tmp->deleteMessage( this );
	}
}


void BWMessageInfo::hideSelf()
{
	BW_GUARD;

	DWORD v = priority_;
	if ((v & PRIORITY_HIDDEN_MASK) != PRIORITY_HIDDEN_MASK)
	{
		priority_ = v | PRIORITY_HIDDEN_MASK;
		isItemHidden_ = true;
	}
}


void BWMessageInfo::displaySelf()
{
	BW_GUARD;

	DWORD v = priority_;
	if ((v & PRIORITY_HIDDEN_MASK) == PRIORITY_HIDDEN_MASK)
	{
		priority_ = v & (~PRIORITY_HIDDEN_MASK);
		isItemHidden_ = false;
	}
}

static MsgHandler* s_pMsgHandler = NULL;
static bool s_finalised = false;


MsgHandler::MsgHandler():
	newMessages_( false ),
	numInfoMessages_( 0 ),
	needsRedraw_( false )
{
	BW_GUARD;

	DebugFilter::instance().addMessageCallback( this );
}

MsgHandler::~MsgHandler()
{
	BW_GUARD;

	DebugFilter::instance().deleteMessageCallback( this );
}

/*static*/ MsgHandler& MsgHandler::instance()
{
	BW_GUARD;

	MF_ASSERT( !s_finalised );

	if (s_pMsgHandler == NULL)
	{
		s_pMsgHandler = new MsgHandler();
	}
	return *s_pMsgHandler;
}


/*static*/ void MsgHandler::fini()
{
	BW_GUARD;

	s_finalised = true;
	bw_safe_delete(s_pMsgHandler);
}


void MsgHandler::clear()
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );
	messages_.clear();
	newMessages_ = true;
	numInfoMessages_ = 0;
}

const bool MsgHandler::updateMessages() 
{
	SimpleMutexHolder smh( mutex_ );
	bool temp = newMessages_;
	newMessages_ = false;
	return temp;
}

const bool MsgHandler::forceRedraw() 
{
	SimpleMutexHolder smh( mutex_ );
	bool temp = needsRedraw_;
	needsRedraw_ = false;
	return temp;
}

// returns a copy of the array because it's not a thread-safe vector
const BW::vector< BWMessageInfoPtr > MsgHandler::messages() const
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );
	return messages_;
}

bool MsgHandler::addAssetErrorMessage( BW::string msg, Chunk* chunk, ChunkItem* item, const char* stackTrace )
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	//Replace any newline characters with spaces
	std::replace( msg.begin(), msg.end(), '\n', ' ' );
	
	assetErrorList_[ msg ]++;

	DebugMessagePriority messagePriority = MESSAGE_PRIORITY_ASSET;
	const char * priorityStr = messagePrefix( messagePriority );
	
	//Get the current date and time
	char dateStr [9];
    char timeStr [9];
    _strdate( dateStr );
    _strtime( timeStr );
	
	if ( messages_.size() == messages_.capacity() )
		messages_.reserve( messages_.size() + 256 );
	messages_.push_back( new BWMessageInfo( messagePriority, dateStr, timeStr, priorityStr, msg.c_str(), chunk, item, stackTrace ));
	newMessages_ = true;

	return true;
}

void MsgHandler::removeAssetErrorMessages()
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	assetErrorList_.clear();

	BW::vector< BWMessageInfoPtr >::iterator it = messages_.begin();
	while ( it != messages_.end() )
	{
		if ( (*it)->priority() == MESSAGE_PRIORITY_ASSET )
			it = messages_.erase(it);
		else
			++it;
	}
	needsRedraw_ = true;
	newMessages_ = true;
}


/*
 *	Override from DebugMessageCallback
 */
bool MsgHandler::handleMessage(
	DebugMessagePriority messagePriority, const char * /*pCategory*/,
	DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
	const char * pFormat, va_list argPtr )
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	int filterLevel;

	//Anything that is not an error, warning or notice should be filtered with info
	if ((messagePriority != MESSAGE_PRIORITY_ERROR) &&
		(messagePriority != MESSAGE_PRIORITY_WARNING) &&
		(messagePriority != MESSAGE_PRIORITY_NOTICE))
	{
		filterLevel = MESSAGE_PRIORITY_INFO;
	}
	else
	{
		filterLevel = messagePriority;
	}

	const char * priorityStr = messagePrefix( messagePriority );

   //Get the current date and time
	char dateStr [9];
    char timeStr [9];
    _strdate( dateStr );
    _strtime( timeStr );
	
	class TempString
	{
	public:
		TempString( int size )
		{
			buffer_ = new char[ size + 1 ];
		}
		~TempString()
		{
			bw_safe_delete_array(buffer_);
		}
		char* buffer_;
	private:
		// unimplemented, hidden methods
		TempString( const TempString& other ) {};
		TempString& operator=( const TempString& other ) { return *this; };
	};

	int size = _vscprintf( pFormat, argPtr );

	TempString tempString( size );

	size = vsnprintf( tempString.buffer_, size, pFormat, argPtr );

	if ( tempString.buffer_[ size - 1 ] == '\n' ) // If the message ends with a newline...
		tempString.buffer_[ size - 1 ] = 0;		// remove it
	else
		tempString.buffer_[ size ] = 0;

	if (filterLevel != MESSAGE_PRIORITY_INFO || MAX_INFO_MESSAGES > 0)
	{
		if (filterLevel == MESSAGE_PRIORITY_INFO)
		{
			for (BW::vector< BWMessageInfoPtr >::iterator it = messages_.begin();
				it != messages_.end() && numInfoMessages_ >= MAX_INFO_MESSAGES;)
			{
				if ((*it)->priority() == MESSAGE_PRIORITY_INFO)
				{
					it = messages_.erase( it );
					--numInfoMessages_;
					continue;
				}
				++it;
			}
			++numInfoMessages_;
		}
		if (messages_.size() == messages_.capacity())
		{
			messages_.reserve( messages_.size() + 256 );
		}

		messages_.push_back( new BWMessageInfo( filterLevel, dateStr, timeStr,
								priorityStr, tempString.buffer_ ));
		newMessages_ = true;
	}

	if (messagePriority == MESSAGE_PRIORITY_ERROR)
	{
		Commentary::instance().addMsg( tempString.buffer_,	Commentary::ERROR_LEVEL );
	}

	return false;
}


int MsgHandler::numAssetMsgs( const BW::string& msg )
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	return (assetErrorList_.find( msg ) == assetErrorList_.end()) ? 0 : assetErrorList_[ msg ];
}

BW_END_NAMESPACE

