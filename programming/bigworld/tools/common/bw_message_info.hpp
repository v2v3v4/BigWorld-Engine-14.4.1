#ifndef BW_MESSAGE_INFO
#define BW_MESSAGE_INFO

#include "cstdmf/bw_map.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

class Chunk;
class ChunkItem;

const int PRIORITY_HIDDEN_MASK = 0x80000000;

class BWMessageInfo: public SafeReferenceCount
{
public:
	BWMessageInfo( int priority, const char* dateStr, const char* timeStr, const char* priorityStr, const char* msgStr,
		Chunk* chunk = NULL, ChunkItem* item = NULL, const char* stackTrace = NULL ):
	priority_(priority),
	chunk_(chunk),
	item_(item),
	isItemHidden_( false ),
	stackTrace_(NULL)
	{
		BW_GUARD;

		dateStr_ = new char[ strlen(dateStr) + 1 ];
		strcpy( dateStr_, dateStr );

		timeStr_ = new char[ strlen(timeStr) + 1 ];
		strcpy( timeStr_, timeStr );
		
		priorityStr_ = new char[ strlen(priorityStr) + 1 ];
		strcpy( priorityStr_, priorityStr );
		
		msgStr_ = new char[ strlen(msgStr) + 1 ];
		strcpy( msgStr_, msgStr );

		registerItem();

		if (stackTrace)
		{
			stackTrace_ = new char[ strlen(stackTrace) + 1 ];
			strcpy( stackTrace_, stackTrace );
		}
	}
	~BWMessageInfo()
	{
		BW_GUARD;

		unregisterItem();
		bw_safe_delete_array(dateStr_);
		bw_safe_delete_array(timeStr_);
		bw_safe_delete_array(priorityStr_);
		bw_safe_delete_array(msgStr_);
		bw_safe_delete_array(stackTrace_);
	}
	int priority() { return priority_; }
	const char* dateStr() { return dateStr_; }
	const char* timeStr() { return timeStr_; }
	const char* priorityStr() { return priorityStr_; }
	const char* msgStr() { return msgStr_; }
	Chunk* chunk() { return isItemHidden_ ? NULL : chunk_; }
	ChunkItem* item() { return isItemHidden_ ? NULL : item_; }
	ChunkItem* realItem() { return item_; }
	const char* stackTrace() { return stackTrace_; }
	void deleteItem() { item_ = NULL; }
	void hideSelf();
	void displaySelf();

private:	
	int priority_;
	char* dateStr_;
	char* timeStr_;
	char* priorityStr_;
	char* msgStr_;
	Chunk* chunk_;
	ChunkItem* item_;
	char* stackTrace_;
	bool isItemHidden_;

	void registerItem();
	void unregisterItem();
};

typedef SmartPointer< BWMessageInfo > BWMessageInfoPtr;

class MsgHandler: public DebugMessageCallback
{
public:
	MsgHandler();
	virtual ~MsgHandler();

	static MsgHandler& instance();
	static void fini();

	void clear();

	const bool updateMessages();

	const bool forceRedraw();

	void forceRedraw( bool redraw )
	{
		newMessages_ = redraw;
		needsRedraw_ = redraw;
	}

	// returns a copy of the array because it's not a thread-safe vector
	const BW::vector< BWMessageInfoPtr > messages() const;

	bool addAssetErrorMessage( BW::string msg, Chunk* chunk = NULL, ChunkItem* item = NULL, const char * stacktrace = NULL );

	void removeAssetErrorMessages();
		
	bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );

	int numAssetMsgs( const BW::string& msg );

private:
	mutable SimpleMutex mutex_;
	BW::vector< BWMessageInfoPtr > messages_;
	bool newMessages_;
	int numInfoMessages_;
	BW::map< BW::string, int > assetErrorList_;
	bool needsRedraw_;
};

BW_END_NAMESPACE
#endif
