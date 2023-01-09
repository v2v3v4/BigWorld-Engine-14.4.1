#ifndef THREAD_SLOT_TRACKER__HPP
#define THREAD_SLOT_TRACKER__HPP

#include "config.hpp"

#if !ENABLE_SLOT_TRACKER
#define MEMTRACKER_DECLARE( id, name, flags )
#define MEMTRACKER_BEGIN( id )
#define MEMTRACKER_END()
#define MEMTRACKER_SCOPED( id )
#define MEMTRACKER_REFERENCE( id )
#else

#include "cstdmf/concurrency.hpp"
#include "cstdmf/stdmf.hpp"

//-----------------------------------------------------------------------------

#define MEMTRACKER_DECLARE( id, name, flags ) \
	ScopedSlot g_trackerSlot_##id( name, flags )

#define MEMTRACKER_BEGIN( id ) \
	BW::ThreadSlotTracker::instance().begin( g_trackerSlot_##id )

#define MEMTRACKER_END() \
	BW::ThreadSlotTracker::instance().end()

#define MEMTRACKER_SCOPED( id ) \
	BW::ScopedSlotTracker g_scopedSlotTracker_##id( g_trackerSlot_##id )

#define MEMTRACKER_REFERENCE( id ) \
	extern ScopedSlot g_trackerSlot_##id;

#define MEMTRACKER_BREAK_ON_ALLOC( slotId, allocId ) \
	ScopedSlotBreak break_##slotTrackerId_##allocId( \
        g_trackerSlot_##slotId, allocId )

//-----------------------------------------------------------------------------
namespace BW
{

class ScopedSlot;
class ScopedSlotBreak;
class ScopedSlotTracker;
        
class ThreadSlotTracker
{
    friend class ScopedSlot;
    friend class ScopedSlotBreak;
    friend class ScopedSlotTracker;
public:

	enum
	{
		MAX_SLOT_NAME_LENGTH= 32,			// The maximum length of a slot name
		MAX_SLOTS			= 256,			// The maximum number of slots
		MAX_THREADS			= 16,			// The maximum number of threads
		SLOT_STACK_DEPTH	= 64,			// The slot stack size
		MAX_BREAKS			= 16			// The maximum number of user breaks
	};

	// Initialise the global thread slot tracker
	static void init();
	// Cleanup the global thread slot tracker
	static void fini();

	// Return the global thread slot tracker, must be initialised first
	CSTDMF_DLL static ThreadSlotTracker& instance();

	// Returns the slot index for the current thread
	int					currentThreadSlot() const;

    CSTDMF_DLL void		begin( uint slotId );
	CSTDMF_DLL void		end();

	// Retrieve slot properties
	const char*			slotName( uint slotId ) const;
	uint32				slotFlags( uint slotId ) const;

private:

	ThreadSlotTracker();
    
    // These functions are used by the MEMTRACKER_ macros to control
	// declaration and usage of slots
	CSTDMF_DLL int		declareSlot( const char * name, uint32 flags );
    CSTDMF_DLL void		popSlot( uint slotId );

	// User defined break on an allocation within a slot.
	int					declareBreak( uint slotId, uint allocId );
    void                popBreak( uint slotId );

	// Stores the slot stack for each thread
	struct ThreadState
	{
		int					curSlot_;			// Current slot
		int					slotStack_[SLOT_STACK_DEPTH];	// The stack of slots
		uint				slotStackPos_;					// The number of slots on the stack
	};

	// Stores a user defined break on an allocation within a slot
	struct Break
	{
		uint				slotId;
		uint				allocId;
	};

	SimpleMutex				mutex_;

	uint					numSlots_;				// Number of declared slots
	char					slotNames_[MAX_SLOTS][MAX_SLOT_NAME_LENGTH];
	uint32					slotFlags_[MAX_SLOTS];

	uint					numBreaks_;			// Number of declared breaks
	Break					breaks_[MAX_BREAKS];// Array of all breaks

	static THREADLOCAL( ThreadState ) s_threadState;
};

// -----------------------------------------------------------------------------

// Class that pushes and pops a slot while its in scope, used by the
// SLOTTRACKER_SCOPED macro.

class ScopedSlotTracker
{
public:
	ScopedSlotTracker( int id )
	{
		ThreadSlotTracker::instance().begin( id );
	}
	~ScopedSlotTracker()
	{
		ThreadSlotTracker::instance().end();
	}
};

class ScopedSlot
{
    uint id_;
    ScopedSlot( const ScopedSlot & );
    ScopedSlot & operator=( const ScopedSlot & );
public:
    ScopedSlot( const char* name, uint32 flags ) :
        id_( ThreadSlotTracker::instance().declareSlot( name, flags ) )
    {
    }
    ~ScopedSlot()
    {
        ThreadSlotTracker::instance().popSlot( id_ );
    }
    operator uint ()
    {
        return id_;
    }
};

class ScopedSlotBreak
{
    uint id_;
    ScopedSlotBreak( const ScopedSlotBreak & );
    ScopedSlotBreak & operator=( const ScopedSlotBreak & );
public:
    ScopedSlotBreak( uint slotId, uint allocId ) :
        id_( ThreadSlotTracker::instance().declareBreak( slotId, allocId ) )
    {
    }
    ~ScopedSlotBreak()
    {
        ThreadSlotTracker::instance().popBreak( id_ );
    }
    operator uint ()
    {
        return id_;
    }
};

} // namespace BW

#endif // ENABLE_SLOT_TRACKER

#endif // THREAD_SLOT_TRACKER__HPP
