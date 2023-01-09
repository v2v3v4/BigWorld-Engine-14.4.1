#include "pch.hpp"
#include "chunk_item_amortise_delete.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/profile.hpp"
#include "appmgr/options.hpp"


//#define CIAD_ENABLED 1

#define CIAD_DEBUG_INFO 0


#if CIAD_DEBUG_INFO
#include <strstream> // Used for debugging only
#endif


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( AmortiseChunkItemDelete )


/**
 *	Constructor.
 *
 *	@param baseTimeSlice		Base time in microseconds to free chunk items.
 *								The default value is 15000 microseconds and
 *								there is a minimum clamp of 100 microseconds.
 *	@param maxChunkItemCount	This is the maximum number of chunk items that
 *								can be stored in the delete list.  The default
 *								is 10000.
 *	@param curbAt				This is the point at which baseTimeSlice will
 *								start to be scaled so that more chunk items can
 *								be processed in a single frame.
 */
AmortiseChunkItemDelete::AmortiseChunkItemDelete(
		double baseTimeSlice /*= 15000.0*/,
		uint32 maxChunkItemCount /*= 10000*/,
		uint32 curbAt /*= 4000*/ ) :
	baseTimeSlice_( std::max< double >( 100.0, baseTimeSlice ) ),
	maxChunkItemCount_( std::max< uint32 >( curbAt, maxChunkItemCount ) ),
	curbAt_( std::min< uint32 >( curbAt, maxChunkItemCount ) ),
	chunkItemListSizePreviousFrame_( 0 ),
	timeSlicePreviousFrame_( baseTimeSlice_ )
{
}


/**
 *	Destructor purges the list before being destroyed.
 */
AmortiseChunkItemDelete::~AmortiseChunkItemDelete()
{
	this->purge();
}


/**
 *	Used to add a chunk item to the deletion list.
 *
 *	@param pItem	The chunk item to be added to the deletion list.
 */
void AmortiseChunkItemDelete::add( ChunkItemPtr pItem )
{
#ifndef CIAD_ENABLED
	return;
#endif
	SimpleMutexHolder chunkItemListMutex( chunkItemListMutex_ );
	chunkItemList_.push_back( pItem );
}


/**
 *	Used to purge the deletion list.
 */
void AmortiseChunkItemDelete::purge()
{
#ifndef CIAD_ENABLED
	return;
#endif
	SimpleMutexHolder chunkItemListMutex( chunkItemListMutex_ );
	chunkItemList_.clear();
}


/**
 *	This method should be called each frame to delete items from the deletion
 *	list.
 */
void AmortiseChunkItemDelete::tick()
{
#ifndef CIAD_ENABLED
	return;
#endif
	SimpleMutexHolder chunkItemListMutex( chunkItemListMutex_ );
	size_t chunkItemListSize = chunkItemList_.size();
	double scaleFactor = 1.0;
	uint32 releaseMinimumCount = 0;

	// If there are no items in the chunk item list, return
	if (chunkItemListSize == 0)
	{
		return;
	}

	// Are we past the curb point
	if (chunkItemListSize > curbAt_)
	{
		// If we have exceeded the maximum limit, clear out at least as many
		// chunk items as were added in the previous frame
		if (chunkItemListSize > maxChunkItemCount_)
		{
			releaseMinimumCount = 
				static_cast<uint32>(chunkItemListSize - chunkItemListSizePreviousFrame_);

#if CIAD_DEBUG_INFO
			// Used for debugging only
			NOTICE_MSG( "AmortiseChunkItemDelete: OVERSHOOT - %d", chunkItemListSize );
#endif //CIAD_DEBUG_INFO
		}

		// Scale the time slice based on how close we are to the maximum limit
		// relative to the curb point.  Note that we subtract releaseMinimumCount
		// in case we have gone past the maximum limit, to make sure curbMaxValue
		// is less than 1.
		double curbMaxValue =
				double(chunkItemListSize - releaseMinimumCount - curbAt_) /
							double(maxChunkItemCount_ - curbAt_);
		// Prevent divide by zero
		double oneMinusCurbMaxValue = std::max< double >( 0.0001, 1.0 - curbMaxValue );
		// Inverse square law - if it works for Gravity it works for me!!!
		scaleFactor = 1.0 / ( oneMinusCurbMaxValue * oneMinusCurbMaxValue * oneMinusCurbMaxValue );
	}

	// Get the current time
	static double usPerStamp = 1000000.0 / stampsPerSecondD();
	double startTime = ((double)(int64)timestamp()) * usPerStamp;
	double elapsedTime = 0.0;
	double previousElapsedTime = 0.0;
	BW::string typeName;

	// Release chunkItems until we have used up all of our allotted time
#if CIAD_DEBUG_INFO
	int chunkItemCount = 0;
#endif //CIAD_DEBUG_INFO
	while (
		releaseMinimumCount > 0 ||
		(!chunkItemList_.empty() &&
		elapsedTime < (2 * timeSlicePreviousFrame_) &&
		elapsedTime < (scaleFactor * baseTimeSlice_)))
	{
#if CIAD_DEBUG_INFO
		// Used for debugging only
		ChunkItemPtr chunkItem = chunkItemList_.front();
		typeName =
#ifdef EDITOR_ENABLED
			BW::string(chunkItem->edClassName());
#else
			"Unknown type";
#endif
		previousElapsedTime = elapsedTime;
		++chunkItemCount;
#endif //CIAD_DEBUG_INFO

		if (releaseMinimumCount > 0)
		{
			--releaseMinimumCount;
		}		

		chunkItemList_.pop_front();
		elapsedTime = ((double)(int64)timestamp()) * usPerStamp - startTime;
	}

	// Store the number of items left in the list
	chunkItemListSizePreviousFrame_ = static_cast<uint32>(chunkItemList_.size());
	// Use the minimum between the current time slice and
	// twice the previous time slice
	timeSlicePreviousFrame_ = std::min< double >(
		scaleFactor * baseTimeSlice_, 2 * timeSlicePreviousFrame_ );

#if CIAD_DEBUG_INFO
	// Used for debugging only
	std::strstream ss;
	ss << 
		chunkItemList_.size() << " left. " <<
		scaleFactor << " scaleFactor. " <<
		chunkItemCount << " released this frame. " <<
		NiceTime( uint64(elapsedTime / usPerStamp) ) << " spent this frame. " <<
		"Last item was a \"" << typeName << "\" and took " <<
		NiceTime( uint64((elapsedTime - previousElapsedTime) / usPerStamp) ) <<
		"." << std::endl;
	ss << std::ends;
	NOTICE_MSG( ss.str() );
#endif //CIAD_DEBUG_INFO
}

BW_END_NAMESPACE

// chunk_item_amortise_delete.cpp
