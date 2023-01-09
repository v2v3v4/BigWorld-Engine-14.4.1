#include "pch.hpp"
#include "slot_tracker.hpp"

#if ENABLE_SLOT_TRACKER

#include "string_utils.hpp"

namespace 
{
//-----------------------------------------------------------------------------

// Memory for ThreadSlotTracker instance
char* s_threadSlotTrackerMem[ sizeof( BW::ThreadSlotTracker ) ];
BW::ThreadSlotTracker* s_threadSlotTracker;

//-----------------------------------------------------------------------------

// Dummy declaration of default slot - set by ThreadSlotTracker::init(). This is
// required for using MEMTRACKER_BREAK_ON_ALLOC(slotId,allocId) on default slot.
int s_trackerSlot_Default = -1;

}

namespace BW
{

/*static*/ THREADLOCAL( BW::ThreadSlotTracker::ThreadState ) ThreadSlotTracker::s_threadState;

//-----------------------------------------------------------------------------

/*static*/ void ThreadSlotTracker::init()
{
	if (!s_threadSlotTracker)
	{
		s_threadSlotTracker = new (s_threadSlotTrackerMem) ThreadSlotTracker;
		s_trackerSlot_Default = s_threadSlotTracker->declareSlot("Default", 0);
		MF_ASSERT_NOALLOC( s_trackerSlot_Default == 0 );
	}
}

//-----------------------------------------------------------------------------

/*static*/ void ThreadSlotTracker::fini()
{
	if (s_threadSlotTracker)
	{
		s_threadSlotTracker->~ThreadSlotTracker();
		s_threadSlotTracker = 0;
	}
}

//-----------------------------------------------------------------------------

/*static*/ ThreadSlotTracker& ThreadSlotTracker::instance()
{
	MF_ASSERT_NOALLOC( s_threadSlotTracker );
	return *s_threadSlotTracker;
}

//-----------------------------------------------------------------------------

ThreadSlotTracker::ThreadSlotTracker() :
	numSlots_(0),
	numBreaks_(0)
{
}

//-----------------------------------------------------------------------------

int ThreadSlotTracker::currentThreadSlot() const
{
	// In some configurations THREADLOCAL is wrapped in a template class,
	// the deref is required in this instance
	ThreadState* ts = &s_threadState;
	return ts->curSlot_;
}

//-----------------------------------------------------------------------------

const char* ThreadSlotTracker::slotName( uint slotId ) const
{
	MF_ASSERT_NOALLOC( slotId < MAX_SLOTS );
	return slotNames_[ slotId ];
}

//-----------------------------------------------------------------------------

uint32 ThreadSlotTracker::slotFlags( uint slotId ) const
{
	MF_ASSERT_NOALLOC( slotId < MAX_SLOTS );
	return slotFlags_[ slotId ];
}

//-----------------------------------------------------------------------------

int ThreadSlotTracker::declareSlot( const char* name, uint32 flags )
{
	SimpleMutexHolder mutexHolder( mutex_ );

	const int slot = numSlots_;
	MF_ASSERT_NOALLOC( slot < MAX_SLOTS );
	numSlots_ = slot + 1;
	size_t nameLength = strlen(name);
	MF_ASSERT( nameLength < MAX_SLOT_NAME_LENGTH );
	bw_str_copy( slotNames_[ slot ], ARRAY_SIZE( slotNames_[ slot ] ),
        name, nameLength );
	slotFlags_[ slot ] = flags;

	return slot;
}

//-----------------------------------------------------------------------------

void ThreadSlotTracker::popSlot( uint slotId )
{
	SimpleMutexHolder mutexHolder( mutex_ );

	MF_ASSERT_NOALLOC( slotId == numSlots_ - 1 );
    memset( slotNames_[ slotId ], 0, ARRAY_SIZE( slotNames_[ slotId ]) );
	slotFlags_[ slotId ] = 0;
	--numSlots_;
}

//-----------------------------------------------------------------------------

int ThreadSlotTracker::declareBreak( uint slotId, uint allocId )
{
	SimpleMutexHolder mutexHolder( mutex_ );

	MF_ASSERT_NOALLOC( numBreaks_ < ARRAY_SIZE( breaks_ ) );

	breaks_[numBreaks_].slotId	= slotId;
	breaks_[numBreaks_].allocId	= allocId;

	return numBreaks_++;
}

//-----------------------------------------------------------------------------

void ThreadSlotTracker::popBreak( uint breakId )
{
	SimpleMutexHolder mutexHolder( mutex_ );

	MF_ASSERT_NOALLOC( breakId == numBreaks_ - 1 );
    breaks_[breakId].slotId	= 0;
	breaks_[breakId].allocId = 0;
	--numBreaks_;
}

//-----------------------------------------------------------------------------

void ThreadSlotTracker::begin( uint slotId )
{
	ThreadState* ts = &s_threadState;
	MF_ASSERT_NOALLOC( slotId < numSlots_ );
	MF_ASSERT_NOALLOC( ts->slotStackPos_ < ARRAY_SIZE( ts->slotStack_ ) );
	ts->slotStack_[ts->slotStackPos_++] = ts->curSlot_;
	ts->curSlot_ = slotId;
}

//-----------------------------------------------------------------------------

void ThreadSlotTracker::end( )
{
	ThreadState* ts = &s_threadState;
	MF_ASSERT_NOALLOC( ts->slotStackPos_ > 0 );
	ts->curSlot_ = ts->slotStack_[--ts->slotStackPos_];
}

}

#endif
