#include "edge_geometry_mappings.hpp"

#include "edge_geometry_mapping.hpp"
#include "geometry_mapper.hpp"

#include "chunk/chunk_space.hpp"

#include "cstdmf/profile.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EdgeGeometryMappings::EdgeGeometryMappings( GeometryMapper & mapper ) :
	mappings_(),
	mapper_( mapper )
{
}


/**
 *	Destructor.
 */
EdgeGeometryMappings::~EdgeGeometryMappings()
{
	MF_ASSERT( mappings_.empty() );
}


/**
 *	This method overrides the GeometryMappingFactory method to create a
 *	edge-specific mapping.
 */
GeometryMapping * EdgeGeometryMappings::createMapping( ChunkSpacePtr pSpace,
			const Matrix & m, const BW::string & path,
			DataSectionPtr pSettings )
{
	Vector3 initialPoint;
	bool hasInitialPoint = mapper_.initialPoint( initialPoint );

	EdgeGeometryMapping * pMapping = new EdgeGeometryMapping( pSpace,
			m, path, pSettings, *this, 
			hasInitialPoint ? &initialPoint :  NULL );

	mappings_.insert( pMapping );

	return pMapping;
}


/**
 *	This method removes a mapping from this collection. It is called by the
 *	destructor of EdgeGeometryMapping.
 */
void EdgeGeometryMappings::removeMapping( EdgeGeometryMapping * pMapping )
{
	MF_VERIFY( mappings_.erase( pMapping ) );
}


// When a line wants to expand/contract, it has to negotiate with the adjacent
// rows to give/take their columns. Now how can that work? When taking, if the
// column in question is busy, then the line just has to wait for it to be
// done I guess ... or else invent some other trickier corner ownership scheme?
// That could work if the rect stayed centered and always grew as a square,
// but since neither of those are likely to be true, I think we will have to
// stick with this dodgy negotiation thing. Hmm this idea isn't nearly as
// attractive any more. Maybe it should just pick one edge at a time and
// expand that one? Let's stick with the 4 edges idea for now.
// Right, I reckon the line can always take the corners over - if it's in the
// middle of loading/unloading that column, then great, that's probably what
// that line wanted to do with it anyway.

/**
 *	This method progresses with loading and/or unloading chunks, to cover
 *	the area we serve.
 */
bool EdgeGeometryMappings::tickLoading( const Vector3 & minB,
		const Vector3 & maxB, bool unloadOnly, SpaceID spaceID )
{
	AUTO_SCOPED_PROFILE( "geometryMappingsTick" );

	// Basic description:

	// the new algorithm works with two rectangles, being the queued-to-load
	// rectangle and the fully-loaded rectangle. the former is used to load
	// more chunks, the latter to unload chunks. the interiors of both
	// rectangles are always considered to be filled in, so only the lines
	// of the four working edges need to be stored. when its line is complete
	// (or emptied), the rectangle moves to incorporate or exclude that line

	// not sure if two distinct rectangles are really the way to go... what
	// happens if you want to change direction and start unloading when there
	// are still some queued-to-load chunks that haven't made it into the
	// fully-loaded rectangle yet? hmmmm...

	// oh, and there is not one rectangle per space, there is one rectangle
	// per GeometryMapping, so there are no issues with grid alignment.

	// also need to do something funky with a new geometry mapping - wait
	// until all cells have loaded all chunks before considering it for
	// restricting load balancing. not entirely sure how that's going to work

	// ---------------------------------------------------------------------

	// Further thoughts:

	// if we always scan from L to R, all we need to keep is a fixed small
	// number of in-flight records (can be a map, or even don't bother with
	// that), and then how far we are up to along each edge. yep that'll be
	// cool. this also solves the problem of reversing direction - those
	// map entries just stay 'busy' until the gird square has finished its
	// full loading rigmarole at which point it can be discarded.
	// ok, now how do we avoid unloading inside chunks that have multiple
	// chunks referring to them? probably just check around at the time,
	// rather than adding a ChunkCache to keep a reference count like that.

	// it would be good if each little op was ordered and directly reversible
	// from any point... that way we don't have to wait for a chunk to load
	// if we've changed our mind and now want to unload it. just have a
	// 'direction' flag in it which it looks at for each chunk tick to
	// decide what to do next. obviously you can't get chunks back from the
	// queue after you've started to load them, so we'll always have to wait
	// for that to complete, but it doesn't mean we then have to go and submit
	// all its overlappers and also wait for them to complete before we unload

	// ---------------------------------------------------------------------

	// Implementation:

	bool anyColumnsLoaded = false;

	Mappings::const_iterator iter = mappings_.begin();

	while (iter != mappings_.end())
	{
		// Condemned mappings will lose all reference counts
		// during a tick, so a smart pointer must be kept to the 
		// mapping to ensure its avaliable until iterating to the
		// next mapping.
		SmartPointer<EdgeGeometryMapping> pMapping( *iter );
		anyColumnsLoaded |= pMapping->tick( minB, maxB, unloadOnly );

		if (pMapping->hasFullyLoaded())
		{
			const BW::string name = pMapping->name().substr( 0,
					pMapping->name().find_first_of( '@' ) );

			mapper_.onSpaceGeometryLoaded( spaceID, name );
		}

		++iter;
	}

	return anyColumnsLoaded;
}


/**
 *	This function goes through any chunks that were submitted to the loading
 * 	thread and prepares them for deletion if the loading thread has finished
 * 	loading them. Also changes the loading flag to false for those that were
 * 	submitted but not loaded.
 */
void EdgeGeometryMappings::prepareNewlyLoadedChunksForDelete()
{
	Mappings::iterator iter = mappings_.begin();

	while (iter != mappings_.end())
	{
		(*iter)->prepareNewlyLoadedChunksForDelete();

		++iter;
	}
}


/**
 *	This private method does the best it can to determine an axis-aligned
 *	rectangle that has been loaded. If there is no geometry mapped into the
 *	space, then a very big rectangle is returned.
 */
void EdgeGeometryMappings::calcLoadedRect( BW::Rect & loadedRect ) const
{
	Mappings::const_iterator iter = mappings_.begin();

	while (iter != mappings_.end())
	{
		const EdgeGeometryMapping * pMapping = *iter;

		bool isFirst = (iter == mappings_.begin());
		++iter;
		bool isLast = (iter == mappings_.end());

		pMapping->calcLoadedRect( loadedRect, isFirst, isLast );
	}
}


/**
 *  This method adds the loadable bounds of this mapping to the rects
 *  list provided as a parameter.
 *  Returns false if bounds are not loaded yet, otherwise returns true.
 */
bool EdgeGeometryMappings::getLoadableRects( BW::list< BW::Rect > & rects ) const
{
	for (Mappings::const_iterator iter = mappings_.begin();
			iter != mappings_.end();
			++iter)
	{
		const EdgeGeometryMapping * pMapping = *iter;

		if (!pMapping->getLoadableRects( rects ))
		{
			return false;
		}
	}

	return true;
}


/**
 *	This method returns whether or not the space is fully unloaded.
 */
bool EdgeGeometryMappings::isFullyUnloaded() const
{
	Mappings::const_iterator iter = mappings_.begin();

	while (iter != mappings_.end())
	{
		if (!(*iter)->isFullyUnloaded())
		{
			return false;
		}

		++iter;
	}

	return true;
}

BW_END_NAMESPACE

// edge_geometry_mappings.cpp
