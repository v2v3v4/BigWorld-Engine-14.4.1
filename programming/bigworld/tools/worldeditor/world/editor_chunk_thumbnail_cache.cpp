#include "pch.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "editor_chunk_thumbnail_cache.hpp"
#include "appmgr/options.hpp"
#include "common/editor_chunk_cache_base.hpp"
#include "common/editor_chunk_terrain_base.hpp"
#include "common/editor_chunk_terrain_cache.hpp"
#include "worldeditor/height/height_map.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "worldeditor/project/chunk_photographer.hpp"
#include "worldeditor/project/space_map.hpp"

BW_BEGIN_NAMESPACE

ChunkCache::Instance<EditorChunkThumbnailCache> EditorChunkThumbnailCache::instance;


void EditorChunkThumbnailCache::chunkThumbnailMode( bool mode )
{
	BW_GUARD;

	static bool hideOutsideFlag = false;

	const char * flagNames[] = {
		"render/gameObjects", 
		"render/lighting",
		"render/environment",
		"render/misc/drawNavMesh",
		"render/scenery",
		"render/scenery/particle",
		"render/scenery/drawWater",
		"render/terrain",
		"render/proxys"
	};

	const int defFlags[] = {
		0,	//render/gameObjects
		0,	//render/lighting
		0,	//render/environment
		0,	//render/misc/drawNavMesh
		1,	//render/scenery
		1,	//render/scenery/particle
		1,	//render/scenery/drawWater
		1,	//render/terrain
		0	//render/proxys
	};

	static int flags[] = {
		0,  //render/gameObjects
		0,	//render/lighting
		0,	//render/environment
		0,	//render/misc/drawNavMesh
		0,	//render/scenery
		0,	//render/scenery/particle
		0,	//render/scenery/drawWater
		0,	//render/terrain
		0	//render/proxys

	};

	if (mode)
	{
		hideOutsideFlag = EditorChunkItem::hideAllOutside();
		EditorChunkItem::hideAllOutside( false );
	}
	else
	{
		EditorChunkItem::hideAllOutside( hideOutsideFlag );
	}

	for (size_t i = 0; i < sizeof( flagNames ) / sizeof( char * ); ++i)
	{
		if (mode)
		{
			flags[i] = Options::getOptionInt( flagNames[i], defFlags[i] );
			Options::setOptionInt( flagNames[i], defFlags[i] );
		}
		else
		{
			Options::setOptionInt( flagNames[i], flags[i] );
		}
	}

	OptionsHelper::tick();
}


EditorChunkThumbnailCache::EditorChunkThumbnailCache( Chunk& chunk ):
	chunk_( chunk ),
	thumbnailDirty_( true )
{
	BW_GUARD;
	MF_ASSERT( chunk.isOutsideChunk() );
	invalidateFlag_ = InvalidateFlags::FLAG_THUMBNAIL;
}


void EditorChunkThumbnailCache::touch( Chunk& chunk )
{
	BW_GUARD;
	if (chunk.isOutsideChunk())
	{
		EditorChunkThumbnailCache::instance( chunk );
	}
}

void EditorChunkThumbnailCache::loadCleanFlags( const ChunkCleanFlags& cf )
{
	BW_GUARD;
	thumbnailDirty_ = cf.thumbnail_ ? false : true;
}

void EditorChunkThumbnailCache::saveCleanFlags( ChunkCleanFlags& cf )
{
	BW_GUARD;
	cf.thumbnail_ = this->dirty() ? 0 : 1;
}

bool EditorChunkThumbnailCache::load( DataSectionPtr, DataSectionPtr cdata )
{
	BW_GUARD;

	// Load the thumbnail and clone it.  We need to create a clone of the 
	// thumbnail otherwise the binary data refers back to it's parent which is 
	// the whole .cdata file and is not used and is rather large.
	pThumbSection_ = cdata->openSection( "thumbnail.dds" );
	if (pThumbSection_)
	{
		if (BinaryPtr oldThumbData = pThumbSection_->asBinary())
		{
			BinaryPtr newThumbData = 
				new BinaryBlock( oldThumbData->data(), oldThumbData->len(), "BinaryBlock/EditorChunkThumbnailCache/ethumbnail" );

			pThumbSection_->setBinary( newThumbData );
		}
		else
		{
			// We don't have actual data in this section. There was a bug that
			// produced bad cdata so that the thumbnail was stored at 
			// "thumbnail.dds/thumbnail.dds". This code will fix these legacy 
			// chunks by deleting the section and mark the thumbnail as dirty 
			// so it gets regenerated correctly.
			BWResource::instance().purge( chunk_.binFileName(), true );
			DataSectionPtr tmp_cdatasection = BWResource::openSection( chunk_.binFileName() );
			tmp_cdatasection->deleteSection("thumbnail.dds");
			tmp_cdatasection->save();

			pThumbSection_ = NULL;
		}
	}

	// Make sure our thumbnails get regenerated as a priority.
	if (!this->hasThumbnail())
	{
		WorldManager::instance().dirtyThumbnail( &chunk_ );
	}

	return true;
}


void EditorChunkThumbnailCache::saveCData( DataSectionPtr cdata )
{
	BW_GUARD;
	cdata->deleteSections( "thumbnail.dds" );

	// save the thumbnail, if it exists
	if (pThumbSection_)
	{
		// If there is a cached thumbnail section then copy its binary data:
		if (DataSectionPtr tSect = cdata->openSection( "thumbnail.dds", true ) )
		{
			BinaryPtr data = pThumbSection_->asBinary();
			tSect->setBinary(data);
		}
	}
}


bool EditorChunkThumbnailCache::readyToCalculate( ChunkProcessorManager* manager )
{
	BW_GUARD;
	return !EditorChunkTerrainCache::instance( chunk_ ).dirty() &&
		!EditorChunkCacheBase::instance( chunk_ ).dirty();
}


bool EditorChunkThumbnailCache::recalc( ChunkProcessorManager* manager, UnsavedList& )
{
	BW_GUARD;
	bool retv;

	chunkThumbnailMode( true );
	retv = ChunkPhotographer::photograph( chunk_ );
	chunkThumbnailMode( false );

	thumbnailDirty_ = false;

	manager->updateChunkDirtyStatus( &chunk_ );

	return retv;
}


/**
*	Invalidate to recalculate data.
*	@param mgr				manager
*	@param spread			not used in this case.
*	@param pChangedItem		the changed item that caused the chunk changed,
*								NULL means it's other reason caused the chunk changed
*	@return true on success.
*/

bool EditorChunkThumbnailCache::invalidate( 
		ChunkProcessorManager* manager,
		bool spread,
		EditorChunkItem* pChangedItem /*= NULL*/ )
{
	BW_GUARD;
	thumbnailDirty_ = true;

	SpaceMap::instance().dirtyThumbnail( &chunk_ );
	HeightMap::instance().dirtyThumbnail( &chunk_, false );

	manager->updateChunkDirtyStatus( &chunk_ );

	return true;
}

/**
*	Check if the thumbnail is dirty.
*	@return true if recalculating is needed.
*/

bool EditorChunkThumbnailCache::dirty()
{
	return !this->hasThumbnail();
}

/**
 *	This gets the cached thumbnail section.  If no thumbnail section exists
 *	then an empty BinSection with the section name "thumbnail.dds" is created.
 */
DataSectionPtr EditorChunkThumbnailCache::pThumbSection()
{
	BW_GUARD;

	if (!pThumbSection_)
	{
		pThumbSection_ = new BinSection( "thumbnail.dds", NULL );
	}

	return pThumbSection_;
}


/**
 *	This gets the thumbnail texture if it exists.
 */
Moo::BaseTexturePtr EditorChunkThumbnailCache::thumbnail()
{
	BW_GUARD;

	if (pThumbSection_)
	{
		// Give the resource id a mangled bit so that it is not confused with
		// a file on disk.
		BW::string resourceName = "@@chunk.thumbnail";

		return Moo::TextureManager::instance()->get(
			pThumbSection_, resourceName, true, false, true );
	}

	return NULL;
}


/**
 *	This returns whether there is a cached thumbnail.
 */
bool EditorChunkThumbnailCache::hasThumbnail() const
{
	BW_GUARD;

	if (thumbnailDirty_)
	{
		return false;
	}

	if (pThumbSection_)
	{
		BinaryPtr data = pThumbSection_->asBinary();

		return data != NULL && data->len() > 0;
	}

	return false;
}

BW_END_NAMESPACE

