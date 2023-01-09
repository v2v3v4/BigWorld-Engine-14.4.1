/**
 *	@file
 */

#if ENABLE_PROFILER

// -----------------------------------------------------------------------------
// Section: Profiler
// -----------------------------------------------------------------------------

/**
 * This function safely requests a entry from the profileEntryBuffer_ via 
 * startNextEntry() and then fills it out. Note that the filling out of this 
 * entry is lazy with respect to thread safety (when it is read in the profile
 * processing it could be invalid as it may not have been completely written 
 * to yet). The processing code needs to be able to detect and ignore invalid 
 * entries, and so frameNumber_ is used. If this has been set to the same value 
 * as every other entry in this frame, then it is valid. 
 *
 * @param  text		name of the profile event
 * @param  e		type of profile event
 * @param  value	value (for counting)
 *
 */
inline void Profiler::addEntry( const char* text, EventType e, int32 value, 
								EventCategory category )
{
	if (!isActive_)
	{
		return;
	}

	ProfileEntry * newEntry = this->startNextEntry();

	if (newEntry == NULL)
	{
		return;
	}

	newEntry->name_ = text;
	newEntry->event_ = e;
	newEntry->category_ = category;
	newEntry->time_ = timestamp();
	newEntry->value_ = value;
	// store the index of the thread so we know where it came from
	newEntry->threadIdx_ = Profiler::getThreadIndex();
#if ENABLE_PER_CORE_PROFILER	
	newEntry->coreNumber_ = CpuInfo::getCurrentProcessorNumber() ;
#endif
	newEntry->frameNumber_ = g_profiler.profileFrameNumber_;
}


inline const char* Profiler::getThreadName(int threadId) const
{
	MF_ASSERT(threadId < MAXIMUM_NUMBER_OF_THREADS && threadId >= 0);

	if (threads_[threadId].thread_)
	{
		return threads_[threadId].name_.c_str();
	}
	return NULL;
}


inline Profiler::ProfileEntry * Profiler::startNextEntry()
{
	EntryBuffer * currentBuffer = profileEntryBuffer_;
	// safely increment the number of profile entries
	if (currentBuffer->entryCount_ >= maxEntryCount_)
	{
		currentBuffer->entryCount_ = maxEntryCount_;
		return NULL;
	}
	int32 index = BW_ATOMIC32_INC_AND_FETCH(&currentBuffer->entryCount_) - 1;
	if (index >= maxEntryCount_)
	{
		currentBuffer->entryCount_ = maxEntryCount_;
		return NULL;
	}
	return currentBuffer->entries_ + index;
}


#endif // ENABLE_PROFILER

// profiler.ipp
