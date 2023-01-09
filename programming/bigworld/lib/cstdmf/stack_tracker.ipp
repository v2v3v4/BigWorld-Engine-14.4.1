/**
 *	@file
 */
 
// -----------------------------------------------------------------------------
// Section: StackTracker
// -----------------------------------------------------------------------------

inline void StackTracker::push(const char* name, const char*file, uint line, bool temp)
{
	assert( stackPos_ < THREAD_STACK_DEPTH );
	StackItem& stackPos = stack_[stackPos_];
	stackPos.name = name;
	stackPos.file = file;
	stackPos.line = line;
	stackPos.temp = temp;
	stackPos.annotation = 0;
	++stackPos_;
	
#ifdef _DEBUG
	if (stackPos_ > maxStackPos_)
	{
		maxStackPos_ = stackPos_;
	}
#endif
}

inline void StackTracker::pop()
{
	assert( stackPos_ > 0 );
	--stackPos_;
}

inline uint StackTracker::stackSize()
{
	return stackPos_;
}

// 0 == top of stack, stackSize-1 == bottom
inline StackTracker::StackItem& StackTracker::getStackItem(uint idx)
{
	assert( idx < stackPos_ );
	return stack_[ stackPos_-1 - idx ];
}
