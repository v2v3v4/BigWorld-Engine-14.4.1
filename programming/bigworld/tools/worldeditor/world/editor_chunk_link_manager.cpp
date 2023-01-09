#include "pch.hpp"
#include "worldeditor/world/editor_chunk_link_manager.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"
#include "chunk/chunk.hpp"
#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    const float MAX_TIME_BETWEEN_RECALCS	=	3.0f;	// max time betweeen recalculation attempts once the need is triggered
    const float WAIT_FOR_MORE_CHUNKS		=	1.0f;	// time we're willing to wait for additional chunks to contribute
    const int   RECALC_LIMIT_PER_FRAME		=	-1;		// maximum number of links that can be recalculated per frame
}


/**
 *	Constructor.
 */
EditorChunkLinkManager::EditorChunkLinkManager() :
	recalcComplete_(true),
	recalcTimerStarted_(false),
	recalcsRemaining_(RECALC_LIMIT_PER_FRAME),
    totalRecalcWaitTime_(0.0),
    recalcWaitTime_(0.0)
{
	BW_GUARD;

	setValid(true);
}


/**
 *	Destructor.
 */
EditorChunkLinkManager::~EditorChunkLinkManager()
{
	BW_GUARD;

	setValid(false);
}


/**
 *	Singleton accessor method.
 *
 *	@return	Singleton instance pointer.
 */
EditorChunkLinkManager* EditorChunkLinkManager::instancePtr()
{
	// Static singleton instance
	static EditorChunkLinkManager s_instance_;
	return &s_instance_;
}


/**
 *	Singleton accessor method.
 *
 *	@return	Singleton instance reference.
 */
EditorChunkLinkManager& EditorChunkLinkManager::instance()
{
	return *instancePtr();
}


/**
 *	Method that checks if the manager is valid.
 *
 *	@return	Boolean success or failure.
 */
bool EditorChunkLinkManager::isValid()
{
	return valid_;
}


/**
 *	Method that sets if the manager is valid.
 *
 *	@param	valid Boolean set validity.
 */
void EditorChunkLinkManager::setValid(bool valid)
{
	valid_ = valid;
}


/**
 *	Method used to register a link with the manager.
 *
 *	@param	pEcl	The link to be registered.
 */
void EditorChunkLinkManager::registerLink(EditorChunkLink* pEcl)
{
	BW_GUARD;

	// Make sure that a valid pointer was passed in
	MF_ASSERT
		(
			pEcl &&
			"EditorChunkLinkManager::registerLink : "
			"pEcl is NULL"
		);

	// Lock mutex
	SimpleMutexHolder regLinksMutex( regLinksMutex_ );
	
	// Add to registered list
	registeredLinks_.insert(pEcl);
}


/**
 *	Method used to unregister a link with the manager.
 *
 *	@param	pEcl	The link to be unregistered.
 */
void EditorChunkLinkManager::unregisterLink(EditorChunkLink* pEcl)
{
	BW_GUARD;

	// Make sure that a valid pointer was passed in
	MF_ASSERT
		(
			pEcl &&
			"EditorChunkLinkManager::unregisterLink : "
			"pEcl is NULL"
		);

	// Lock mutex
	SimpleMutexHolder regLinksMutex( regLinksMutex_ );
	
	// Remove from registered list
	registeredLinks_.erase( pEcl );
}


/**
 *	Method called to inform the link manager that a chunk has been loaded.
 *
 *	@param	pChunk	The newly loaded chunk.
 */
void EditorChunkLinkManager::chunkLoaded(Chunk* pChunk)
{
	BW_GUARD;

	// Lock mutex
	SimpleMutexHolder regLinksMutex( regLinksMutex_ );

	// Iterate through the list of registered links
	RegLinksIt it;
	for (it = registeredLinks_.begin(); it != registeredLinks_.end(); ++it)
	{
		// Inform the link that the passed chunk has loaded
		(*it)->chunkLoaded(pChunk->identifier());
	}
}


/**
 *	Method called to inform the link manager that a chunk has been tossed.
 *
 *	@param	pChunk	The newly loaded chunk.
 */
void EditorChunkLinkManager::chunkTossed(Chunk* pChunk)
{
	BW_GUARD;

	// Lock mutex
	SimpleMutexHolder regLinksMutex( regLinksMutex_ );

	// Iterate through the list of registered links
	RegLinksIt it;
	for (it = registeredLinks_.begin(); it != registeredLinks_.end(); ++it)
	{
		// Inform the link that the passed chunk has loaded
		(*it)->chunkTossed(pChunk->identifier());
	}
}


/**
 *	Method informs the manager that a newly added chunk is on a links path, so
 *	the link will need to be updated.
 */
void EditorChunkLinkManager::coveringLoadedChunk()
{
	BW_GUARD;

	if (!recalcTimerStarted_)
		totalRecalcWaitTime_ = 0.0;

	// Reset and start the timer
	recalcComplete_ = false;
	recalcTimerStarted_ = true;
	recalcWaitTime_ = 0.0;
}


/**
 *	Method called by WorldEditor each frame.
 */
void EditorChunkLinkManager::update( float dTime )
{
	BW_GUARD;

	if (recalcComplete_)
		recalcTimerStarted_ = false;
	
	recalcsRemaining_ = 
		Options::getOptionInt( "render/links/maxRecalcsPerFrame", RECALC_LIMIT_PER_FRAME );
	
	totalRecalcWaitTime_ += dTime;
    recalcWaitTime_ += dTime;
}


/**
 *	Method to inform a link if it can draw itself or not.
 */
bool EditorChunkLinkManager::canRecalc()
{
	BW_GUARD;

	if
		(
			((recalcTimerStarted_ && totalRecalcWaitTime_ >= 
				Options::getOptionFloat( "render/links/maxTimeBetweenRecalcs", MAX_TIME_BETWEEN_RECALCS )) ||	// Max wait timer
			 (recalcTimerStarted_ && recalcWaitTime_ >=
				Options::getOptionFloat( "render/links/maxTimeWaitForMoreChunks", WAIT_FOR_MORE_CHUNKS )))		// Wait for additional chunks timer
		)
	{
		if ((recalcsRemaining_ == -1 || recalcsRemaining_ > 0))	// We haven't recalculated more than our quota
		{
			if (recalcsRemaining_ > 0)
				--recalcsRemaining_;

			recalcComplete_ = true;

			return true;
		}
		else
		{
			recalcComplete_ = false;

			return false;
		}
	}

	return false;
}
BW_END_NAMESPACE

