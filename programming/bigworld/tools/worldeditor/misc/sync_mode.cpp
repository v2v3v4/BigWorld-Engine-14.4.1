#include "pch.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "chunk/chunk_manager.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	size_t	s_nestCount		= 0;
	bool	s_ok			= true;
}


/**
 *  This class, while in scope, stops WE's background calculations and turns
 *	on synchronous mode in the chunk manager.  This allows for explicit 
 *	chunk loading.
 *
 *  If there was a failure in setting things up then operator bool will
 *  return false (see below).
 */
SyncMode::SyncMode()
{
	BW_GUARD;

	if (s_nestCount++ == 0)
	{
		s_ok = true;

		// Make sure that we can get exclusive access
		if (WorldManager::instance().checkForReadOnly())
		{
			s_ok = false;
		}
		else
		{
			// Kill WorldEditor's background calculation so we don't clobber each 
			// other:
			WorldManager::instance().stopBackgroundCalculation();
			EditorChunkOverlapper::drawList.clear();
			ChunkManager::instance().switchToSyncMode(true);
		}
	}
}


/**
 *  This functions sets BB back into synchronous mode and undoes what the
 *  ctor did.
 */
SyncMode::~SyncMode()
{
	BW_GUARD;

	if (--s_nestCount == 0)
	{
		if (s_ok)
		{
			ChunkManager::instance().switchToSyncMode(false);
			EditorChunkOverlapper::drawList.clear();
		}
	}
}


/**
 *  This function returns true if we could get exclusive access to chunks.
 */
SyncMode::operator bool() const
{
    return s_ok;
}
BW_END_NAMESPACE

