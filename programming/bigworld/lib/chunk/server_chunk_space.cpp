#include "pch.hpp"

#include "server_chunk_space.hpp"
#include "chunk.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ServerChunkSpace
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ServerChunkSpace::ServerChunkSpace( ChunkSpaceID id ) :
	BaseChunkSpace( id )
{
}


void ServerChunkSpace::seeChunk( Chunk * pChunk )
{
	// find out where it is in the focus grid
	const Vector3 & cen = pChunk->centre();
	int nx = int(cen.x / gridSize());	if (cen.x < 0.f) nx--;
	int nz = int(cen.z / gridSize());	if (cen.z < 0.f) nz--;

	// open all the columns, and mark them as having seen this chunk
	for ( int x = nx - 1; x <= nx + 1; ++x )
	{
		for ( int z = nz - 1; z <= nz + 1; ++z )
		{
			ServerChunkSpace::Column * pCol = currentFocus_( x, z );		
			if (pCol != NULL) pCol->openAndSee( pChunk );
		}
	}
}


/**
 *	This method sets the focus point for this space
 */
void ServerChunkSpace::focus()
{

	// focus any chunks have been blurred.

	while ( blurred_.size() > 0 )
	{
		Chunk * pChunk = *(blurred_.begin());
		blurred_.erase( blurred_.begin() );

		// find out where it is in the focus grid
		const Vector3 & cen = pChunk->centre();
		int nx = int(cen.x / gridSize());	if (cen.x < 0.f) nx--;
		int nz = int(cen.z / gridSize());	if (cen.z < 0.f) nz--;


		// see if this chunk is new to (nx,nz) and adjacent columns,
		// close them if it isn't
		for ( int x = nx - 1; x <= nx + 1; ++x ) 
		{
			for ( int z = nz - 1; z <= nz + 1; ++z )
			{
				Column * pCol = currentFocus_( x, z );
				if ( pCol != NULL ) {
					pCol->shutIfSeen( pChunk );
				}
			}
		}

		// do the actual focussing work
		pChunk->focus();

		// it's ok for a chunk to re-add itself on failure to focus,
		// 'coz it'll go to the end of our vector (not that chunks
		// currently ever fail to focus)

		// open all the columns, and mark them as having seen this chunk
		for ( int x = nx - 1; x <= nx + 1; ++x )
		{
			for ( int z = nz - 1; z <= nz + 1; ++z )
			{
				Column * pCol = currentFocus_( x, z );

				if (pCol != NULL) pCol->openAndSee( pChunk );
			}
		}

	}

	// TODO: will need to think about the stuff below eventually. Eg when 
	// the server finds out about moving platforms.

	/*
	// let every column know if it's soft (this sucks I know!)
	for (int x = cx - ColumnGrid::SPANH; x <= cx + ColumnGrid::SPANH; x++)
	{
		for (int z = cz - ColumnGrid::SPANH; z <= cz + ColumnGrid::SPANH; z++)
		{
			Column * pCol = currentFocus_( x, z );
			if (pCol != NULL) pCol->soft(
				x <= cx-(FOCUS_RANGE-1) || x >= cx+(FOCUS_RANGE-1) ||
				z <= cz-(FOCUS_RANGE-1) || z >= cz+(FOCUS_RANGE-1) );
		}
	}
	*/

	// process column caches
//	Column::cacheControl( false );

/*
	// if we focussed any chunks then see if any homeless items
	// would prefer to live in them now instead
	if (bs != blurred_.size())
	{
		for (int i = int(homeless_.size()-1); i >= 0; i--)
		{
			i = std::min( i, int(homeless_.size()-1) );
			ChunkItemPtr pHomelessItem = homeless_[i];
			pHomelessItem->nest( static_cast<ChunkSpace*>( this ) );
		}
	}
*/

}


/**
 *	This method is called to make any changes for new grid bounds
 */
void ServerChunkSpace::recalcGridBounds()
{
	// min/max gridx/gridy have already been calculated by our caller
	currentFocus_.rect( minGridX_, minGridY_,
		maxGridX_-minGridX_+1, maxGridY_-minGridY_+1 );
}


void ServerChunkSpace::ColumnGrid::rect(
	int xOriginNew, int zOriginNew, int xSizeNew, int zSizeNew )
{
	// make thew new grid vector
	BW::vector< Column * > gridNew( xSizeNew * zSizeNew );

	// copy or delete existing columns
	for (int x = xOrigin_; x < xOrigin_+xSize_; x++)
	{
		bool xInSpan = (x >= xOriginNew && x < xOriginNew+xSizeNew);
		for (int z = zOrigin_; z < zOrigin_+zSize_; z++)
		{
			bool zInSpan = (z >= zOriginNew && z < zOriginNew+zSizeNew);

			Column *& col = grid_[ (x-xOrigin_) + xSize_*(z-zOrigin_) ];

			if (xInSpan && zInSpan)
				gridNew[ (x-xOriginNew) + xSizeNew*(z-zOriginNew) ] = col;
			else
				delete col;

			col = NULL;
		}
	}

	// set the new variables
	xOrigin_ = xOriginNew;
	zOrigin_ = zOriginNew;
	xSize_ = xSizeNew;
	zSize_ = zSizeNew;

	// and swap the grid vectors around
	grid_.swap( gridNew );
}

BW_END_NAMESPACE

// server_chunk_space.cpp
