#include "pch.hpp"
#include "chunk_processor.hpp"
#include "chunk_processor_manager.hpp"

BW_BEGIN_NAMESPACE

/**
*	add an ChunkProcessor.
*/
void ChunkProcessors::addProcessor( ChunkProcessorPtr processor )
{
	BW_GUARD;
	MF_ASSERT( std::find(
		chunkProcessors_.begin(), 
		chunkProcessors_.end(), 
		processor ) == chunkProcessors_.end() );

	chunkProcessors_.push_back( processor );
}

/**
*	delete an ChunkProcessor.
*/
void ChunkProcessors::delProcessor( ChunkProcessorPtr processor )
{
	BW_GUARD;
	ChunkProcessorList::iterator listIt = std::find(
		chunkProcessors_.begin(), 
		chunkProcessors_.end(), 
		processor );

	if( listIt != chunkProcessors_.end() )
	{
		chunkProcessors_.erase( listIt );
	}
}


ChunkProcessor::ChunkProcessor() :
	BackgroundTask( "ChunkProcessor" ),
	backgroundResult_( false ),
	cleanedUp_( false )
{}


/**
 *	Add background and main thread tasks.
 *	@return true on success.
 */
bool ChunkProcessor::process( ChunkProcessorManager& mgr )
{
	BW_GUARD;

	if (mgr.numRunningThreads())
	{
		mgr.addBackgroundTask( this );

		return true;
	}

	// NOTE: This "background" process can't be aborted.
	backgroundResult_ = processInBackground( mgr );

	backgroundResult_ &= processInMainThread( mgr, backgroundResult_ );

	cleanup( mgr );

	return backgroundResult_;
}

/**
 * called if this task is canceled/dropped.
 */
void ChunkProcessor::cancel( TaskManager & mgr )
{
	BW_GUARD;

	// Put in main thread task list so that it can cleanup normally
	// When cancelled, we're never running processInBackground
	// ChunkProcessors depend on processInMainThread for the chunk caches to
	// clean up
	mgr.addMainThreadTask( this );
}

/**
 *	Derived from BackgroundTask.
 *	Add self to ChunkProcessorManager.
 */
void ChunkProcessor::doMainThreadTask( TaskManager& mgr )
{
	BW_GUARD;

	if ( !processInMainThread(
			static_cast<ChunkProcessorManager&>( mgr ),
			backgroundResult_ ) )
	{
		mgr.addMainThreadTask( this );
	}
	else
	{
		cleanup( static_cast<ChunkProcessorManager&>( mgr ) );
	}
}

/**
*	notify all our listeners when we finish,
*	all the inherited classes must call this in it's cleanup.
*/
void ChunkProcessor::cleanup( ChunkProcessorManager& manager )
{ 
	if ( !cleanedUp_ )
	{
		cleanedUp_ = true;
		notifyListenerFinished(); 
	}
}

/**
*	notify all the listeners that I'v finished.
*/
void ChunkProcessor::notifyListenerFinished()
{
	for (int32 i = 0; i < (int32)listeners_.size(); ++i)
	{
		bool hasUnregisteredFromListener = false;
		listeners_[i]->onChunkProcessorFinished( this, hasUnregisteredFromListener );
		if (hasUnregisteredFromListener)
		{
			// if ths listerner is unregistered after
			// onChunkProcessorFinished called, we should --i
			--i;
		}
	}

}


/**
*	Registers an ChunkProcessor listener instance.
*/
void ChunkProcessor::addListener(ChunkProcessorListener * pListener)
{
	BW_GUARD;
	ChunkProcessorListenerVector::iterator it = std::find(
		this->listeners_.begin(), 
		this->listeners_.end(), 
		pListener );

	if( it == this->listeners_.end() )
	{
		this->listeners_.push_back( pListener );
	}
}

/**
*	Unregisters an ChunkProcessor listener instance.
*/
void ChunkProcessor::delListener(ChunkProcessorListener * pListener)
{
	BW_GUARD;
	ChunkProcessorListenerVector::iterator listIt = std::find(
		this->listeners_.begin(),
		this->listeners_.end(), 
		pListener );

	if( listIt != this->listeners_.end() )
	{
		this->listeners_.erase( listIt );
	}
}


/**
 *	Derived from BackgroundTask.
 */
void ChunkProcessor::doBackgroundTask( TaskManager& mgr )
{
	BW_GUARD;
	backgroundResult_ = processInBackground(
		static_cast<ChunkProcessorManager&>( mgr ) );

	mgr.addMainThreadTask( this );
}

BW_END_NAMESPACE

// chunk_processor.cpp
