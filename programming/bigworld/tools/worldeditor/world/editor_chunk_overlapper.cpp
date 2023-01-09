#include "pch.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "appmgr/options.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorChunkOverlapper
// -----------------------------------------------------------------------------

#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkOverlapper, overlapper, 0 )

bool EditorChunkOverlapper::s_drawAlways_ = false;
uint32 EditorChunkOverlapper::s_settingsMark_ = -16;

int EditorChunkOverlapper_token = 0;


BW::vector<Chunk*> EditorChunkOverlapper::drawList;

/**
 *	Constructor.
 */
EditorChunkOverlapper::EditorChunkOverlapper() :
	bound_( false )
{
}


/**
 *	Destructor.
 */
EditorChunkOverlapper::~EditorChunkOverlapper()
{
}


/**
 *	Load method. Creates an unappointed chunk for our overlapper.
 */
bool EditorChunkOverlapper::load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString )
{
	BW_GUARD;

	pOwnSect_ = pSection;

	overlapperID_ = pSection->asString();

	if (overlapperID_.empty())
	{
		if ( errorString )
		{
			*errorString = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_OVERLAPPER/FAIL_TO_LOAD");
		}

		return false;
	}

	pOverlappingChunk_ = new Chunk( overlapperID_, pChunk->mapping() );

	ChunkManager::instance().addChunkToSpace( pOverlappingChunk_, pChunk->space()->id() );

	return true;
}


/**
 *	Toss method. If we get moved to another chunk that is bound
 *	(or we are created in it) then we can do our bind action now.
 */
void EditorChunkOverlapper::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL) EditorChunkOverlappers::instance( *pChunk_ ).del( this );

	ChunkOverlapper::toss( pChunk );

	if (pChunk_ != NULL) EditorChunkOverlappers::instance( *pChunk_ ).add( this );

	if (pChunk_ != NULL && pChunk_->isBound())
	{
		this->bindStuff();
	}
}

/**
 *	Draw method. We add the chunk we refer to to the fringe drawing list
 *	if chunk overlappers are being drawn.
 */
void EditorChunkOverlapper::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (pOverlappingChunk_ == NULL || !pOverlappingChunk_->isBound())
	{
		return;
	}

	ChunkManager & cmi = ChunkManager::instance();

	if (cmi.cameraChunk()->drawMark() != s_settingsMark_)
	{
		bool drawAllShells = Options::getOptionInt(
			"render/scenery/shells/gameVisibility", s_drawAlways_ ? 0 : 1 ) == 0;
		bool hideAllOutside = Options::getOptionInt(
			"render/hideOutsideObjects" , s_drawAlways_ ? 1 : 0 ) == 1;

		s_drawAlways_ = drawAllShells || hideAllOutside;
		s_settingsMark_ = cmi.cameraChunk()->drawMark();
	}

	if (EditorChunkOverlapper::s_drawAlways_)
	{
		if (pOverlappingChunk_->drawMark() != cmi.cameraChunk()->drawMark() &&
			pOverlappingChunk_->fringePrev() == NULL)
		{
			if ( std::find( drawList.begin(), drawList.end(), pOverlappingChunk_ ) == drawList.end() ) 			
				drawList.push_back( pOverlappingChunk_ );
		}
	}
}


/**
 *	Lend method. We use this as a notification that the chunk has been
 *	bound and we are running in the main thread. This kind of machinery
 *	would normally go in the chunk itself (except it is editor specific),
 *	so I don't really feel a need to add a 'bind' method to ChunkItem.
 */
void EditorChunkOverlapper::lend( Chunk * pLender )
{
	BW_GUARD;

	this->bindStuff();
}


/**
 *	This method does the stuff we want to do when the chunk is bound,
 *	i.e. resolve our stub chunk and add it to the load queue if necessary.
 */
void EditorChunkOverlapper::bindStuff()
{
	BW_GUARD;

	if (bound_) return;

	pOverlappingChunk_ = pOverlappingChunk_->space()->findOrAddChunk(
							pOverlappingChunk_ );
	bound_ = true;

	if (!pOverlappingChunk_->isBound())
	{
		ChunkManager::instance().loadChunkExplicitly(
			pOverlappingChunk_->identifier(),
			WorldManager::instance().geometryMapping(),
			true );
	}
}



// -----------------------------------------------------------------------------
// Section: EditorChunkOverlappers
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
EditorChunkOverlappers::EditorChunkOverlappers( Chunk & chunk ) :
	pChunk_( &chunk )
{
}

/**
 *	Destructor.
 */
EditorChunkOverlappers::~EditorChunkOverlappers()
{
}


/**
 *	Add this overlapper item to our collection
 */
void EditorChunkOverlappers::add( EditorChunkOverlapperPtr pOverlapper )
{
	BW_GUARD;

	items_.push_back( pOverlapper );
}

/**
 *	Remove this overlapper item from our collection
 */
void EditorChunkOverlappers::del( EditorChunkOverlapperPtr pOverlapper )
{
	BW_GUARD;

	Items::iterator found =
		std::find( items_.begin(), items_.end(), pOverlapper );
	if (found != items_.end())
	{
		items_.erase( found );
	}
}


/**
 *	Make a new overlapper item in the chunk we are a cache for to
 *	specify the input chunk as an overlapper
 */
void EditorChunkOverlappers::form( Chunk * pOverlapper )
{
	BW_GUARD;

	// make the datasection element
	DataSectionPtr pSect =
		EditorChunkCache::instance( *pChunk_ ).pChunkSection();
	pSect = pSect->newSection( "overlapper" );
	pSect->setString( pOverlapper->identifier() );

	// we don't use the normal chunk item creation pathway here 'coz we
	// don't want to the normal undo/redo baggage.

	// now load that item, which will automatically add itself to our list
	MF_ASSERT( pChunk_->loadItem( pSect ) );

	// and flag ourselves as dirty
	BW::set<Chunk*> chunks;
	chunks.insert( pChunk_ );
	chunks.insert( pOverlapper );

	WorldManager::instance().changedChunks( 
		chunks,
		InvalidateFlags::FLAG_THUMBNAIL |
		InvalidateFlags::FLAG_NAV_MESH |
		InvalidateFlags::FLAG_SHADOW_MAP );

}

/**
 *	Get rid of the overlapper item in the chunk we are a cache for
 *	that specified the input chunk as an overlapper
 */
void EditorChunkOverlappers::cut( Chunk * pOverlapper )
{
	BW_GUARD;

	// find the item that points to this chunk (if any)
	for (Items::iterator it = items_.begin(); it != items_.end(); it++)
	{
		if ((*it)->pOverlapper() == pOverlapper)
		{

			// delete its datasection
			DataSectionPtr pParent =
				EditorChunkCache::instance( *pChunk_ ).pChunkSection();
			pParent->delChild( (*it)->pOwnSect() );

			// and delete the item itself
			pChunk_->delStaticItem( *it );

			// and it's gone, so get out
			//  (our iterator is stuffed now anyway)

			// flag ourselves as dirty
			WorldManager::instance().changedChunk( 
				pChunk_,
				InvalidateFlags::FLAG_THUMBNAIL |
				InvalidateFlags::FLAG_NAV_MESH |
				InvalidateFlags::FLAG_SHADOW_MAP );

			return;
		}
	}

	// we didn't find one. this is ok for now, but should be upgraded to
	// an error when all overlapping chunks have an 'overlapper' item in
	// the chunk they overlap.
	WARNING_MSG( "EditorChunkOverlappers::cut: "
		"No overlapper item in %s points to %s\n",
		pChunk_->identifier().c_str(), pOverlapper->identifier().c_str() );
}


/// Static instance accessor initialiser
ChunkCache::Instance<EditorChunkOverlappers> EditorChunkOverlappers::instance;
BW_END_NAMESPACE

