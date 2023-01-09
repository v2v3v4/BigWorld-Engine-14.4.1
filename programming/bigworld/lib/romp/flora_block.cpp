#include "pch.hpp"
#include "flora_block.hpp"
#include "flora_constants.hpp"
#include "flora_renderer.hpp"
#include "ecotype.hpp"
#include "flora.hpp"
#include "cstdmf/debug.hpp"
#include "math/vector2.hpp"
#include "math/planeeq.hpp"
#include "terrain/base_terrain_block.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

namespace Util
{

static float s_ecotypeBlur = 1.f;
void setEcotypeBlur( float amount )
{
	BW_GUARD;
	s_ecotypeBlur = amount;
	const BW::vector< Flora* > & floras = Flora::floras();
	for( unsigned i=0; i<floras.size(); i++)
	{
		floras[i]->initialiseOffsetTable(amount);
	}
	Flora::floraReset();
}

float getEcotypeBlur()
{
	return s_ecotypeBlur;
}

static float s_positionBlur = 1.3f;
void setPositionBlur( float amount )
{
	BW_GUARD;
	s_positionBlur = amount;
	const BW::vector< Flora* > & floras = Flora::floras();
	for( unsigned i=0; i<floras.size(); i++)
	{
		floras[i]->initialiseOffsetTable(amount);
	}
	Flora::floraReset();
}

float getPositionBlur()
{
	return s_positionBlur;
}


void initFloraBlurAmounts( DataSectionPtr pSection )
{
	BW_GUARD;
	//This occurs when loading/unloading spaces.  Don't call
	//through to the setPositionBlur/setEcotypeBlur fns as
	//we don't want unnecessarily recreate the existing flora
	//which is either already or just about to be unloaded.
	pSection->readFloat( "ecotypeBlur", s_ecotypeBlur );
	pSection->readFloat( "positionBlur", s_positionBlur );
}

} // namespace Util


#ifdef EDITOR_ENABLED
bool FloraBlock::recordAVisualCopy( void* pVisualCopy, uint32 offset )
{
	BW_GUARD;
	if (numRecordedVisualCopy_ >= MAX_RECORDED_VISUAL_COPY)
	{
		return false;
	}
	recordedVisualCopy_[ numRecordedVisualCopy_ ].pVisualCopy_ = pVisualCopy;
	recordedVisualCopy_[ numRecordedVisualCopy_++ ].offset_ = offset;
	return true;
}

void FloraBlock::findVisualCopyVerticeOffset( void* pVisualCopy, BW::vector< uint32 >& retOffsets )
{
	BW_GUARD;
	for (int i = 0; i < numRecordedVisualCopy_; ++i)
	{
		if (recordedVisualCopy_[i].pVisualCopy_ == pVisualCopy)
		{
			retOffsets.push_back( recordedVisualCopy_[i].offset_ );
		}
	}
}
#endif

/**
 *	Constructor.
 */
FloraBlock::FloraBlock() :
	bRefill_( true ),
	center_( 0.f, 0.f ),
	culled_( true ),
	blockID_( -1 )
#ifdef EDITOR_ENABLED
	,numRecordedVisualCopy_( 0 )
#endif
{	
	BW_GUARD;
}


/**
 *	Thie method initialises the flora block.  FloraBlocks must be given
 *	a position.
 *
 *	@param	pos		Vector2 describing the center of the block.
 *	@param	offset	number of vertexes offset from start of vertex memory
 */
void FloraBlock::init( const Vector2& pos, uint32 offset )
{
	BW_GUARD;
	static bool s_added = false;
	if (!s_added)
	{		
		s_added = true;
		MF_WATCH( "Client Settings/Flora/Ecotype Blur",
			&Util::getEcotypeBlur,
			&Util::setEcotypeBlur,
			"Creates a circle, with radius in metres, centered around each flora item. "
			"The ecotype sample point for an item will be somewhere within this circle." );
		MF_WATCH( "Client Settings/Flora/Position Blur",
			&Util::getPositionBlur,
			&Util::setPositionBlur,
			"Multiplier for positioning each flora object.  Set to a higher "
			"value to make flora objects encroach upon neighbouring blocks.");
	}

	offset_ = offset;
	this->center( pos );
}

class FloraItem
{
public:
	// Default constructor required for use with DynamicEmbeddedArray for now
	FloraItem() {}
	FloraItem( Ecotype * e, const Matrix & o ):
		ecotype_(e),
		objectToWorld_(o)
	{
	}

	Ecotype * ecotype_;
	Matrix	objectToWorld_;
};

typedef BW::DynamicEmbeddedArrayWithWarning<FloraItem, 1024> FloraItems;

/**
 *	This method fills a vertex allocation with appropriate stuff.
 *
 *	Note all ecotypes are reference counted.  This is so flora knows
 *	when an ecotype is not used, and can thus free its texture memory
 *	for newly activated ecotypes.
 */
void FloraBlock::fill( Flora * flora, uint32 numVertsAllowed )
{
	BW_GUARD;
	static DogWatch watch( "Flora fill" );
	ScopedDogWatch dw( watch );

#ifdef EDITOR_ENABLED
	this->resetRecordedVisualCopy();
#endif 

	Matrix objectToChunk;
	Matrix objectToWorld;

	//First, check if there are terrain blocks at our location.
	BoundingBox bb(bb_);
	float extends = BLOCK_WIDTH / 2.f + Util::s_positionBlur;
	bb.expandSymmetrically( extends, 0.f, extends );
	Vector2 pos;
	pos.set( bb.minBounds().x, bb.minBounds().z );
	if ( !Terrain::BaseTerrainBlock::findOutsideBlock(Vector3(pos.x,0.f,pos.y) ).pBlock_ )
		return;	//return now.  fill later.
	pos.set( bb.minBounds().x, bb.maxBounds().z );
	if ( !Terrain::BaseTerrainBlock::findOutsideBlock(Vector3(pos.x,0.f,pos.y) ).pBlock_ )
		return;	//return now.  fill later.
	pos.set( bb.maxBounds().x, bb.maxBounds().z );
	if ( !Terrain::BaseTerrainBlock::findOutsideBlock(Vector3(pos.x,0.f,pos.y) ).pBlock_ )
		return;	//return now.  fill later.
	pos.set( bb.maxBounds().x, bb.minBounds().z );
	if ( !Terrain::BaseTerrainBlock::findOutsideBlock(Vector3(pos.x,0.f,pos.y) ).pBlock_ )
		return;	//return now.  fill later.

	Vector2 center( bb_.centre().x, bb_.centre().z );
	Terrain::TerrainFinder::Details details = 
		Terrain::BaseTerrainBlock::findOutsideBlock( 
			Vector3(center.x,0.f,center.y) );
	if (!details.pBlock_)
		return;	//return now.  fill later.

	const Matrix& chunkToWorld = *details.pMatrix_;
	blockID_ = (int)flora->getTerrainBlockID( chunkToWorld, details.pBlock_->blockSize() );	
	Matrix worldToChunk = *details.pInvMatrix_;	
	uint32 numVertices = numVertsAllowed;	


	FloraItems items;

	//Seed the look up table of random numbers given
	//the center position.  This means we get a fixed
	//set of offsets given a geographical location.
	flora->seedOffsetTable( center );

	Matrix transform;
	Vector2 ecotypeSamplePt;
	uint32 idx = 0;

	while (1)
	{
		if (!nextTransform( flora, center, objectToWorld, ecotypeSamplePt ))
		{
			break;
		}

		objectToChunk.multiply( objectToWorld, worldToChunk );

		//If the spot underneath the flora object is an 'empty' ecotype, then
		//keep that - we don't want anyone encroaching on an uninhabitable spot.
		//Otherwise, lookup the ecotype at the sample point instead.
		Ecotype & accurate = flora->ecotypeAt(
			Vector2(objectToWorld.applyToOrigin().x,objectToWorld.applyToOrigin().z));		

		FloraItem fi(NULL, objectToWorld);

		if ( accurate.isEmpty() )
		{
			fi.ecotype_ = &accurate;			
		}
		else
		{
			fi.ecotype_ = &flora->ecotypeAt( ecotypeSamplePt );
		}

		//The ecotypes covering this FloraBlock are not yet fully loaded.
		//We will come back next frame and try again.  This is the main
		//reason for this initial while loop (check the loaded flags).
		if (fi.ecotype_->isLoading_)
		{				
			return;
		}

		items.push_back(fi);

		//Calling generate with NULL as the first parameter indicates we just want to know
		//the number of vertices that would have been generated, but leave the VB intact.
		uint32 nVerts = fi.ecotype_->generate( NULL, idx, numVertices, fi.objectToWorld_, objectToChunk, bb_ );
		if (nVerts == 0)
		{
			break;			
		}
		numVertices -= nVerts;
		if (numVertices == 0)
		{
			break;			
		}
		idx++;
	}

	//Alright so we have all the information for the items, and all the
	//ecotypes are loaded and valid.  Now produce vertices.
	idx = 0;
	numVertices = numVertsAllowed;
	//we will get a properly calculated bounding box from ecotype generation.
	bb_ = BoundingBox::s_insideOut_;
	FloraVertexContainer * pVerts = flora->pRenderer()->lock( offset_, numVertsAllowed );
	if (pVerts ==  NULL)
	{
		ERROR_MSG( "FloraBlock::fill - lock failed\n" );
		return;
	}
#ifdef EDITOR_ENABLED
	pVerts->pCurBlock_ = this;
#endif


	//Seed the look up table of random numbers again
	//This means the ecotype generators will end up using
	//exactly the same random numbers as in the first pass.
	flora->seedOffsetTable( center );

	// reserve some space to reduce allocations
	ecotypes_.reserve( items.size() );
	for (uint32 i=0; i<items.size(); i++)
	{
		FloraItem& fi = items[i];
		Ecotype& ecotype = *fi.ecotype_;
		ecotypes_.push_back( &ecotype );
		ecotype.incRef();
		objectToChunk.multiply( fi.objectToWorld_, worldToChunk );
		uint32 nVerts = ecotype.generate( pVerts, idx, numVertices, fi.objectToWorld_, objectToChunk, bb_ );
		if (nVerts == 0)			
		{
			break;			
		}
		idx++;
		numVertices -= nVerts;
		if (numVertices == 0)
		{
			break;
		}
	}


	//fill the rest of the given vertices with degenerate triangles.
	MF_ASSERT( (numVertices%3) == 0 );
	pVerts->clear( numVertices );
	flora->pRenderer()->unlock( pVerts );

	if ( bb_ == BoundingBox::s_insideOut_ )
	{
		blockID_ = -1;
	}
	bRefill_ = false;
}


/**
 *	This method should be called to move a flora block to a new position.
 *
 *	@param	c	The new center of the block.
 */
void FloraBlock::center( const Vector2& c )
{
	BW_GUARD;
	center_ = c;
	this->invalidate();
}


/**
 *	This method invalidates the flora block.  It sets the bRefill flag to true,
 *	and releases reference counts for all ecoytpes currently used.
 */
void FloraBlock::invalidate()
{
	BW_GUARD;
	bb_.setBounds(	Vector3(	center_.x,
								-20000.f, 
								center_.y ),
					Vector3(	center_.x,
								-20000.f, 
								center_.y) );
	blockID_ = -1;

	BW::vector<Ecotype*>::iterator it = ecotypes_.begin();
	BW::vector<Ecotype*>::iterator end = ecotypes_.end();

	while ( it != end )
	{
		if (*it)
			(*it)->decRef();
		it++;
	}

	ecotypes_.clear();

	bRefill_ = true;
}


void FloraBlock::cull()
{
	BW_GUARD;
	if (blockID_ != -1 )
	{
		bb_.calculateOutcode( Moo::rc().viewProjection() );
		culled_ = !!bb_.combinedOutcode();
	}
	else
	{
		culled_ = true;
	}
}


/**
 *	This method checks if the block's maximum bounds
 *  lie within the specified bounds.
 */
bool FloraBlock::withinBounds( const AABB& bounds ) const
{
	float extends = BLOCK_WIDTH / 2.f + Util::s_positionBlur;
	float minx = center_.x - extends;
	float minz = center_.y - extends;
	float maxx = center_.x + extends;
	float maxz = center_.y + extends;

	Vector3 pos[4] = {
		Vector3( minx, 0.f, minz ),
		Vector3( minx, 0.f, maxz ),
		Vector3( maxx, 0.f, maxz ),
		Vector3( maxx, 0.f, minz ) };

	for (int i = 0; i < 4; ++i)
	{
		if (!bounds.intersects( pos[i] ))
		{
			return false;
		}
	}

	return true;
}


/**
 *	This method calculates the next random transform
 *	for a block.  Each transform is aligned to the
 *	terrain and positions an object on the terrain.
 *
 *	It also returns an ecotype sample point.  This
 *	suggests where to choose an ecotype from (this
 *	itself is blurred slightly, in order to anti-
 *	alias the ecotype data)
 */
bool FloraBlock::nextTransform( Flora * flora, const Vector2 & center,
	Matrix & ret, Vector2 & retEcotypeSamplePt )
{
	BW_GUARD;
	//get the new position.
	Vector2 off = flora->nextOffset();
	float rotY( flora->nextRotation() );
	Vector3 pos( center.x + (off.x * Util::s_positionBlur), 0.f,
		center.y + (off.y * Util::s_positionBlur) );

	//get the new ecotype.  this is blurred so we can encroach on
	//neighbouring ecotypes
	off = flora->nextOffset();

	//convert s_ecotypeBlur to metres, to make it easy for artists to tweak.
	const float halfBlockWidth = BLOCK_WIDTH / 2.f;
	static float radius = sqrtf( 2.f * (halfBlockWidth * halfBlockWidth) );
	static float offsetToMetres = (1.f/radius);

	retEcotypeSamplePt.x = off.x * offsetToMetres * Util::s_ecotypeBlur + pos.x;
	retEcotypeSamplePt.y = off.y * offsetToMetres * Util::s_ecotypeBlur + pos.z;

	//get the terrain block, and the relative position of
	//pos within the terrain block.
	Vector3 relPos;
	Terrain::BaseTerrainBlockPtr pBlock = flora->getTerrainBlock( pos, relPos, NULL );

	if ( !pBlock )
	{
		return false;
	}
	else
	{		
		//sit on terrain
		pos.y = pBlock->heightAt( relPos.x, relPos.z );
		if ( pos.y == Terrain::BaseTerrainBlock::NO_TERRAIN )
			return false;
		Vector3 intNorm = pBlock->normalAt( relPos.x, relPos.z );

		//align to terrain
		const PlaneEq eq( intNorm, intNorm.dotProduct( pos ) );
		Vector3 xyz[2];
		xyz[0].set( 0.f, eq.y( 0.f, 0.f ), 0.f );
		xyz[1].set( 0.f, eq.y( 0.f, 1.f ), 1.f );
		Vector3 up = xyz[1] - xyz[0];
		up.normalise();
		ret.lookAt( Vector3( 0.f, 0.f, 0.f ),
			up, Vector3( eq.normal() ) );
		ret.invertOrthonormal();

		//rotate randomly
		Matrix rot;
		rot.setRotateY( rotY );
		ret.preMultiply( rot );

		//move to terrain block local coords
		ret.translation( pos );	
	}

	return true;
}

BW_END_NAMESPACE

// flora_block.cpp
