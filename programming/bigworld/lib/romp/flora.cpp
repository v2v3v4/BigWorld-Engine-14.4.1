#include "pch.hpp"

#include "flora.hpp"
#include "flora_texture.hpp"
#include "flora_constants.hpp"
#include "flora_vertex_type.hpp"
#include "flora_renderer.hpp"
#include "enviro_minder.hpp"
#include "cstdmf/debug.hpp"
#include "chunk/chunk_flora.hpp"
#include "moo/device_callback.hpp"
#include "moo/moo_dx.hpp"
#include "moo/graphics_settings_picker.hpp"
#include "terrain/base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"
#include "terrain/dominant_texture_map.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "animation_grid.hpp"
#include "flora_block.hpp"
#include "time_of_day.hpp"
#include "moo/geometrics.hpp"

#include "ecotype_generators.hpp"


DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

namespace { // anonymous

bool s_cull = true;
int s_numDrawPrimitiveCalls = 0;
int s_numTris = 0;
bool s_debugBB = false;
bool s_debugStatus = false;
int s_allowedBlockMovesPerFrame = 20;
float s_alphaTestDistance = 25.f;	//I.E. < 25 metres
float s_decimationFadePct = 100.f;
float s_alphaTestFadePct = 70.f;
uint32 s_alphaTestRef = 64;
uint32 s_shadowAlphaTestRef = 0x00000046;	//70 DEC
uint32 s_maxVBSize = 1048576;

float s_tintBegin = 0.f;
float s_tintEnd = 0.15f;

struct FloraBlockDistancePair
{
	static bool lt( const FloraBlockDistancePair & a,
		const FloraBlockDistancePair & b )
	{
		return (a.distance_ < b.distance_);
	}

	float distance_;
	const FloraBlock * block_;
};

} // anonymous

/**
 *	This global method specifies the resources required by this file
 */
//static AutoConfigString s_mfmName( "environment/floraMaterial" );

/*static*/ bool Flora::s_enabled_ = true;
/*static*/ BW::vector< Flora* > Flora::s_floras_;
Flora::BlocksCompletedCallback Flora::s_blocksCompletedCallback_ = NULL;

namespace Util
{
extern void initFloraBlurAmounts(DataSectionPtr);
} // namespace Util

/**
 *	Constructor.
 */
Flora::Flora():
	data_( NULL ),
	lutSeed_( 0 ),
	lastPos_( -30000.f, -30000.f ),
	cameraTeleport_( true ),
	numVerticesPerBlock_( 0 ),
	maxVBSize_( 0 ),
	centerBlockX_(BLOCK_STRIDE/2),
	centerBlockZ_(BLOCK_STRIDE/2),	
	lastRefPt_( 1e23f, 1e23f ),
	pRenderer_( NULL ),
	vbSize_( 0 ),
	degenerateEcotype_( this ),
	pFloraTexture_( NULL ),
	terrainVersion_( 0 )
{
	BW_GUARD;
	cosMaxSlope_ = cosf( DEG_TO_RAD( 30.f ) );

	macroBB_[0].blocks_[0] = NULL;
	initialiseOffsetTable();

	//we set the isLoading flag for the degenerate ecotype.  this is
	//used so that flora blocks that are allocated the degenerate one
	//will never use it to fill vertices.
	degenerateEcotype_.isLoading_ = true;

	//Create the watchers once only
	static bool s_createdWatchers = false;
	if (!s_createdWatchers)
	{
		MF_WATCH( "Client Settings/Flora/Alpha Test Ref",
			s_alphaTestRef,
			Watcher::WT_READ_WRITE,
			"Alpha test reference value for the alpha tested drawing pass." );
		MF_WATCH( "Client Settings/Flora/Alpha Test Ref (Shadows)",
			s_shadowAlphaTestRef,
			Watcher::WT_READ_WRITE,
			"Alpha test reference value for the shadow drawing pass." );
		MF_WATCH( "Client Settings/Flora/Alpha Test Draw Distance",
			s_alphaTestDistance,
			Watcher::WT_READ_WRITE,
			"Range from the camera to draw the alpha tested drawing pass.  "
			"Set to 0 to disable the alpha test draw pass." );
		MF_WATCH( "Client Settings/Flora/Alpha Test Fade Pct",
			s_alphaTestFadePct,
			Watcher::WT_READ_WRITE,
			"Percent of the alpha test drawing range over which the flora fades out." );
		MF_WATCH( "Client Settings/Flora/Decimation Fade Pct",
			s_decimationFadePct,
			Watcher::WT_READ_WRITE,
			"Percent of the alpha blended drawing range over which the flora fades out." );
		MF_WATCH( "Client Settings/Flora/per frame block move max",
			s_allowedBlockMovesPerFrame,
			Watcher::WT_READ_WRITE,
			"Number of flora blocks that may be updated per-frame.  Set this value higher"
			"if the flora seems to be lagging behind the camera.  Higher values may incur"
			"a higher per-frame cost depending on the speed at which the camera moves." );
		MF_WATCH( "Client Settings/Flora/Debug BB",
			s_debugBB,
			Watcher::WT_READ_WRITE,
			"Enable visualisation of the flora blocks' bounding boxes, and the flora's"
			"macro bounding box culling system." );
		MF_WATCH( "Client Settings/Flora/Debug Status",
			s_debugStatus,
			Watcher::WT_READ_WRITE,
			"Enable visualisation of flora block's update status, and the status of the"
			"composite flora texture." );
		MF_WATCH( "Client Settings/Flora/Cull",
			s_cull,
			Watcher::WT_READ_WRITE,
			"Enabled or disable culling of flora.  If disabled, the flora will not move"
			"around with the camera or be clipped by its frustum." );
		MF_WATCH( "Client Settings/Flora/Num Draw Calls",
			s_numDrawPrimitiveCalls,
			Watcher::WT_READ_ONLY,			
			"Number of draw calls made by the Flora in the last frame." );
		MF_WATCH( "Client Settings/Flora/Num Triangles",
			s_numTris,
			Watcher::WT_READ_ONLY,
			"Number of triangles drawn by the Flora in the last frame." );

		MF_WATCH( "Client Settings/Flora/tintStart",
			s_tintBegin,
			Watcher::WT_READ_WRITE,
			"Relative height where terrain tile textures start to blend with flora texture." );

		MF_WATCH( "Client Settings/Flora/tintEnd",
			s_tintEnd,
			Watcher::WT_READ_WRITE,
			"Relative height where terrain color texture end to blend with flora texture." );

			s_createdWatchers = true;
	}

	memset( ecotypes_, 0, sizeof( Ecotype* ) * 256 );

	pFloraTexture_ = new FloraTexture();

	s_floras_.push_back( this );

}

Flora::~Flora()
{
	BW_GUARD;

	//Make sure to remove this flora from the floras vector
	BW::vector< Flora* >::iterator fit = s_floras_.begin();
	BW::vector< Flora* >::iterator fend = s_floras_.end();
	for (; fit != fend; ++fit)
	{
		if (*fit == this)
		{
			s_floras_.erase( fit );
			break;
		}
	}

	//Delete all the ecotypes
	{
		SimpleMutexHolder smh( Ecotype::s_deleteMutex_ );
		for ( int i=0; i<256; i++ )
		{
			bw_safe_delete(ecotypes_[i]);
		}
	}

	bw_safe_delete(pFloraTexture_);

	//Finally delete the renderer
	bw_safe_delete(pRenderer_);
}


/**
 *	This method initialises the flora singleton, and should be called once
 *	at startup.
 *
 *	Note : flora data section is "flora/flora.xml"
 *
 *	@param	pSection			The flora data section.
 *	@param	terrainVersion		The terrain version specified for this space.
 */
bool Flora::init( DataSectionPtr pSection, uint32 terrainVersion )
{
	BW_GUARD;

	data_			= pSection;
	terrainVersion_ = terrainVersion;

	if(!data_.exists())
	{
		return false;
	}

	s_alphaTestRef = pSection->readInt( "AlphaTestRef", s_alphaTestRef );
	s_shadowAlphaTestRef = pSection->readInt( "ShadowAlphaTestRef", s_shadowAlphaTestRef );
	s_alphaTestDistance = pSection->readFloat( "AlphaTestDistance", s_alphaTestDistance );
	s_alphaTestFadePct = pSection->readFloat( "AlphaTestFadePercent", s_alphaTestFadePct );
	s_allowedBlockMovesPerFrame = pSection->readInt( "MaxPerFrameBlockMove", s_allowedBlockMovesPerFrame );
	s_tintBegin = pSection->readFloat("TintBegin", s_tintBegin);
	s_tintEnd = pSection->readFloat("TintEnd", s_tintEnd);
	Util::initFloraBlurAmounts(pSection);

	texToEcotype_.clear();
	Ecotype::ID id = 0;
	DataSectionPtr pEcotypes = pSection->openSection( "ecotypes" );
	if (pEcotypes)
	{
		DataSection::iterator it = pEcotypes->begin();
		BW::vector<BW::string> pTextures;
		while (it != pEcotypes->end())
		{
			DataSectionPtr pSect = *it++;
			pTextures.clear();
			pSect->readStrings("texture", pTextures);
			for (uint32 i=0; i<pTextures.size(); i++)
			{
				BW::StringRef texRoot = BWResource::removeExtension( pTextures[i] );
				texToEcotype_[ texRoot ] = id;
			}
			id++;
		}
	}

	float maxSlope = pSection->readFloat( "maxSlope", 30.f );
	cosMaxSlope_ = cosf( DEG_TO_RAD( maxSlope ) );

	this->maxVBSize( pSection->readInt( "vb_size", s_maxVBSize ) );

	pFloraTexture_->init( pSection );

	return true;
}

void Flora::activate()
{
	BW_GUARD;
	MF_WATCH( "Client Settings/Flora/Repopulate",
		cameraTeleport_,
		Watcher::WT_READ_WRITE,
		"Set to 1 to reset and repopulate the flora.  Value will "
		"automatically be set back to 0 when complete." );

	if (maxVBSize() != 0)
	{
		if (this->pRenderer() != NULL)
		{
			this->floraReset();
			this->floraTexture()->activate();
			this->pRenderer_->activate();
		}
		else
		{
			ERROR_MSG("EnviroMinder::activate: Could not activate flora, the renderer is not initialised.");
		}
	}
}

void Flora::deactivate()
{
	BW_GUARD;
#if ENABLE_WATCHERS
	if (Watcher::hasRootWatcher())
	{
		Watcher::rootWatcher().removeChild("Client Settings/Flora/Repopulate");
	}
#endif

	if (maxVBSize() != 0)
	{
		if (this->pRenderer() != NULL)
		{
			this->floraTexture()->deactivate();
			this->pRenderer_->deactivate();
		}
	}
}

/**
 *	This method implements the Moo::DeviceCallback interface for creating
 *	DX resources not managed by DX.  Here we create our vertex buffer
 *	and shaders.
 */
void Flora::createUnmanagedObjects()
{
	BW_GUARD;
	//no createUnmanaged before init()
	if ( !data_ &&
		this->maxVBSize() != 0)
		return;

	if ( !pRenderer_ )
	{
		pRenderer_ = new FloraRenderer( this );
	}	

	float ratio = FloraSettings::instance().vbRatio();
	this->vbSize(uint32(this->maxVBSize() * ratio));
}

/**
 *	This method implements the Moo::DeviceCallback interface for deleting
 *	DX resources that were created by createUnmanagedObjects.
 */
void Flora::deleteUnmanagedObjects()
{
	BW_GUARD;

	//reset all flora blocks
	for (size_t z = 0; z < BLOCK_STRIDE; ++z)
	{
		for (size_t x = 0; x < BLOCK_STRIDE; ++x)
		{
			blocks_[z][x].invalidate();
		}
	}
}


/**
 *	This method sets a callback function to call when all blocks have updated.
 */
void Flora::setBlocksCompletedCallback(	BlocksCompletedCallback callback )
{
	if (s_enabled_)
	{
		s_blocksCompletedCallback_ = callback;
	}
	else
	{
		callback();
	}
}

/**
 *	This method calls the callback function and resets it to null.
 */
void Flora::CallBlocksCompletedCallback()
{
	if (s_blocksCompletedCallback_)
	{
		s_blocksCompletedCallback_();
		s_blocksCompletedCallback_ = NULL;
	}
}

bool Flora::moveBlocks( const Vector2& camPos2 )
{
	BW_GUARD;
	AABB spaceBounds =
		DeprecatedSpaceHelpers::cameraSpace()->bounds();

	//see if the center most block needs moving ( based on a 4 metre threshold )
	//if so, then we know that at least one row or column is now invalid.
	FloraBlock * centerBlock = &blocks_[centerBlockZ_][centerBlockX_];
	Vector2 delta = camPos2 - centerBlock->center();
	int moveX = 0; int moveZ = 0;
	float viz = BLOCK_WIDTH / 2.f;

	if ( delta.x > viz ) moveX = 1;
	else if ( delta.x < -viz ) moveX = -1;

	if ( delta.y > viz ) moveZ = -1;
	else if ( delta.y < -viz ) moveZ = 1;

	if ( moveX )
	{
		int left = (centerBlockX_ - (BLOCK_STRIDE/2) + BLOCK_STRIDE) % BLOCK_STRIDE;
		int right = (centerBlockX_ + ((BLOCK_STRIDE - 1)/2)) % BLOCK_STRIDE;
		Vector2 delta((float)moveX * MOVE_DIST, 0.f);
		int colToMove = (moveX > 0) ? left : right;
		for (int z=0; z<BLOCK_STRIDE; z++)
		{
			FloraBlock * block = &blocks_[z][colToMove];
			block->center(block->center() + delta);

			if (block->withinBounds( spaceBounds ))
			{
				movedBlocks_.insert( block );
			}
			else
			{
				movedBlocks_.erase( block );
			}
		}
		centerBlockX_ = (centerBlockX_ + moveX + BLOCK_STRIDE) % BLOCK_STRIDE;
	} else if ( moveZ )
	{
		int front = (centerBlockZ_ - (BLOCK_STRIDE/2) + BLOCK_STRIDE) % BLOCK_STRIDE;
		int rear = (centerBlockZ_ + ((BLOCK_STRIDE-1)/2)) % BLOCK_STRIDE;
		Vector2 delta(0.f, (float)moveZ * -MOVE_DIST);
		int rowToMove = (moveZ > 0) ? front : rear;
		for (int x=0; x<BLOCK_STRIDE; x++)
		{
			FloraBlock * block = &blocks_[rowToMove][x];
			block->center(block->center() + delta);

			if (block->withinBounds( spaceBounds ))
			{
				movedBlocks_.insert( block );
			}
			else
			{
				movedBlocks_.erase( block );
			}
		}
		centerBlockZ_ = (centerBlockZ_ + moveZ + BLOCK_STRIDE) % BLOCK_STRIDE;
	}	

	return moveX || moveZ;
}


void Flora::fillBlocks()
{
	BW_GUARD;
	if ( !movedBlocks_.size() )
		return;
	/**
	 *	Note : all flora blocks must be moved before any flora blocks can be refilled.
	 *	the reason being - when a flora block moves, it's ecotype reference may be deallocated.
	 *	if a whole row of flora blocks move out of an ecotype area, then ecotypes may be
	 *	fully deref'd, and therefore texture memory freed up.
	 *
	 *	Before we refill flora blocks, we need the available space.
	 */

	//Check if any flora blocks need refilling
	//If any new ecotypes are referenced, they will make sure their texture memory is switched in.
	int numMovedBlocks = 0;
	for( BW::set<FloraBlock*>::iterator it = movedBlocks_.begin();
		it != movedBlocks_.end();)
	{
		FloraBlock* block = *it;
		if ( (numMovedBlocks<s_allowedBlockMovesPerFrame) && block->needsRefill() )
		{			
			block->fill( this, numVerticesPerBlock_ );
			if (!block->needsRefill())
			{
				numMovedBlocks++;
			}
		}
		if ( !block->needsRefill() )
		{			
			// remove from list
			movedBlocks_.erase( it++ );
		}
		else
		{		
			++it;
		}
	}

	//Update the bounding boxes when if have actually changed
	//any of the flora blocks.
	if (numMovedBlocks > 0)
	{
		this->accumulateBoundingBoxes();
	}
}


/**
 *	This method updates the flora blocks ( moves them
 *	given the new position of the camera ), and updates
 *	the animation grid to suit.
 *
 *	@param	dTime	The change in time since the last frame.
 *	@param	enviro	Unused parameter.
 */
void Flora::update( float dTime, EnviroMinder& enviro )
{
	BW_GUARD;
	if ( !s_enabled_ ||
		!pRenderer_ ||
		!this->vbSize_ )
		return;

	Vector3 camPos( Moo::rc().invView().applyToOrigin() );
	Vector2 camPos2( camPos.x, camPos.z );

	//If we have pretty much teleported, then
	//repopulate all detail objects.
	if ( fabsf( lastPos_.x - camPos2.x ) > MOVE_DIST )
	{
		cameraTeleport_ = true;
	}
	else if ( fabsf( lastPos_.y - camPos2.y ) > MOVE_DIST )
	{
		cameraTeleport_ = true;
	}

#ifdef EDITOR_ENABLED
	//Checking for 0.f time elapsed from previous frame. (means game is paused)
	//To generate the same animation consistently for automated testing screenshots.
	if (dTime == 0.f)
	{
		srand( 0 );
	}
#endif

	//Now we move the flora blocks.  We either move them all ( as in the
	//case of the cameraTeleport flag being set ), or we ask each block
	//if it needs to move.
	if ( s_cull )
	{
		if ( cameraTeleport_ )
		{
			this->teleportCamera( camPos2 );
			cameraTeleport_ = false;
		}
		else
		{
			this->moveBlocks( camPos2 );
		}
	}

	lastPos_ = camPos2;

	if ( movedBlocks_.size() > 0 )
		this->fillBlocks();

	if (movedBlocks_.size() == 0)
	{
		CallBlocksCompletedCallback();
	}
}


void Flora::accumulateBoundingBoxes()
{
	BW_GUARD;
	int left = (centerBlockX_ - (BLOCK_STRIDE/2) + BLOCK_STRIDE) % BLOCK_STRIDE;
	int xEnd = left + BLOCK_STRIDE;
	int front = (centerBlockZ_ - (BLOCK_STRIDE/2) + BLOCK_STRIDE) % BLOCK_STRIDE;
	int zEnd = front + BLOCK_STRIDE;

	//TODO : remove hardcoding of blocks of 5.  using 5 now coz it is all 25 divides into easily
	int idx=0;
	int x = left;
	int z = front;
	BoundingBox bb = BoundingBox::s_insideOut_;

	for (int z=front; z<zEnd; z+=5)
	{
		for (int x=left; x<xEnd; x+=5)
		{
			MacroBB& mbb = macroBB_[idx];
			mbb.bb_ = BoundingBox::s_insideOut_;
			for (int zz=0; zz<5; zz++)
			{
				for (int xx=0; xx<5; xx++)
				{
					FloraBlock * block = &blocks_[(z+zz)%BLOCK_STRIDE][(x+xx)%BLOCK_STRIDE];
					if (block->blockID() != -1)
						mbb.bb_.addBounds( block->bounds() );
					mbb.blocks_[zz*5+xx] = block;
				}
			}
			idx++;
		}
	}
}


/**
 *	This method updates the culled flag for all flora blocks.
 *	Because it is too expensive to cull all 625 blocks, we
 *	accumulate each 5x5 square of blocks and cull that bounding box.
 */
void Flora::cull()
{
	BW_GUARD;
	static DogWatch cullFloraWatch( "Cull" );
	cullFloraWatch.start();

	for ( int i=0; i<25; i++ )
	{
		MacroBB& mbb = macroBB_[i];
		mbb.bb_.calculateOutcode( Moo::rc().viewProjection() );
		bool fullyCulled = !!mbb.bb_.combinedOutcode();
		bool fullyInside = !mbb.bb_.outcode();

		if ( fullyCulled )
		{
			for (int j=0; j<25; j++)
			{
				mbb.blocks_[j]->culled( true );
			}
		}
		else if ( fullyInside )
		{
			for (int j=0; j<25; j++)
			{
				FloraBlock* block = mbb.blocks_[j];
				if ( block->blockID() != -1 )
					block->culled( false );
				else
					block->culled( true );
			}
		}
		else
		{
			for (int j=0; j<25; j++)
			{
				mbb.blocks_[j]->cull();
			}
		}
	}

	cullFloraWatch.stop();
}


/**
 *	This method draws the flora.  There are a number of
 *	ways to do this.  This particular implementation just
 *	draws the whole damn VB ( 20,000 polys or so )
 */
void Flora::draw( float dTime, EnviroMinder& enviro )
{
	BW_GUARD;
	s_numDrawPrimitiveCalls = 0;                                            
	s_numTris = 0;    

	if ( !pRenderer_ ||
		 !s_enabled_ ||
		 !macroBB_[0].blocks_[0] ||
		 !vbSize_ )
		return;

	pRenderer_->update( dTime, enviro );

	//render states
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	if ( !pRenderer_->preDraw( enviro, s_tintBegin, s_tintEnd))
		return;

	if ( s_cull )
	{
		this->cull();
	}

	drawSorted(enviro);

	pRenderer_->postDraw();	

	Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );
}


void Flora::drawSorted( EnviroMinder & enviro )
{
	BW_GUARD;

	if (s_alphaTestDistance <= 0.f)
	{
		return;
	}

	//sort the flora blocks from back to front.  The sorted blocks
	//list is frame coherent
	const Vector3 & camPos = Moo::rc().invView().applyToOrigin();
	const Vector2 camPos2(camPos.x, camPos.z);

	const float adjBlockWidth = BLOCK_WIDTH * 2.f;
	const float squareDistance = s_alphaTestDistance * s_alphaTestDistance
		+ adjBlockWidth * adjBlockWidth;

	size_t numSortedBlocks = 0;
	FloraBlockDistancePair sortedBlocks[BLOCK_STRIDE * BLOCK_STRIDE];

	{
		BW_SCOPED_DOG_WATCHER( "Sort" );

		for (size_t z=0; z < BLOCK_STRIDE; ++z)
		{
			for (size_t x=0; x < BLOCK_STRIDE; ++x)
			{
				const FloraBlock & block = blocks_[z][x];
				if (!block.culled() && block.blockID() != -1)		
				{
					const float blockDistSq = 
						Vector2(block.center() - camPos2).lengthSquared();
					if (blockDistSq < squareDistance)
					{
						FloraBlockDistancePair & pair =
							sortedBlocks[numSortedBlocks++];
						pair.distance_ = blockDistSq;
						pair.block_ = &block;
					}
				}
			}
		}
		std::sort( sortedBlocks, sortedBlocks + numSortedBlocks,
			FloraBlockDistancePair::lt );	
	}

	{
		//draw the flora blocks
		BW_SCOPED_DOG_WATCHER( "Draw" );

		//draw alpha tested blocks front to back, only going for x blocks distance.
		//then draw alpha blended blocks back to front, only going for y blocks distance.
		pRenderer_->beginAlphaTestPass( s_alphaTestDistance, s_alphaTestFadePct, s_alphaTestRef/*, s_floraBatching*/ );

		const bool bDecimation = s_decimationFadePct < 100.0f;
		const float fDecimationDistance = s_alphaTestDistance *
			(s_decimationFadePct * 0.01f);
		const float fDecimationDistanceSq = fDecimationDistance *
			fDecimationDistance;
		const float fDecimationMulK = 1.0f / (s_alphaTestDistance *
			s_alphaTestDistance - fDecimationDistanceSq);

		for (size_t i = 0; i < numSortedBlocks; ++i)
		{
			const FloraBlockDistancePair & pair = sortedBlocks[i];
			const FloraBlock & block = *pair.block_;
			MF_ASSERT( !block.culled() && block.blockID() != -1 );
			MF_ASSERT( pair.distance_ < squareDistance );
			int numVertices = numVerticesPerBlock_;
			if (bDecimation)
			{
				const float fDecimationK = (pair.distance_ -
					fDecimationDistanceSq) * fDecimationMulK;
				if (fDecimationK > 1.f)
				{
					numVertices = 0;
				}
				else if (fDecimationK > 0.f)
				{
					numVertices = int(numVertices * (1.f - fDecimationK));
					if (numVertices % 3)
					{
						numVertices -= numVertices % 3;
					}
				}
			}

			if (numVertices)
			{
				pRenderer_->drawBlock( block.offset(), numVertices, block );
				s_numDrawPrimitiveCalls++;
				s_numTris += (numVertices)/3;
			}
		}
	}
}


void Flora::drawDebug()
{
	BW_GUARD;
	if ( s_debugStatus )
	{
		Moo::Material::setVertexColour();
		Moo::rc().setPixelShader( NULL );		
		Moo::rc().push();
		Moo::rc().world( Matrix::identity );

		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );		
		Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );		

		FloraBlock * center = &blocks_[centerBlockZ_][centerBlockX_];

		for ( int z=0; z<BLOCK_STRIDE; z++ )
		{
			for ( int x=0; x<BLOCK_STRIDE; x++ )
			{
				float scale = 2.f;
				FloraBlock * block = &blocks_[z][x];
				Vector2 pos( block->center() );
				pos.x -= Moo::rc().invView().applyToOrigin().x;
				pos.y -= Moo::rc().invView().applyToOrigin().z;				
				pos.x *= scale;
				pos.y *= scale;
				pos.x += Moo::rc().screenWidth() / 2;
				pos.y = Moo::rc().screenHeight() / 2 - pos.y;
				Vector2 pos2( pos );
				pos2 += Vector2(BLOCK_WIDTH*scale, BLOCK_WIDTH*scale);

				Moo::Colour c;
				if (block->culled())
				{
					c = Moo::Colour( block->needsRefill() ? 0x80800000 : 0x80008000 );
				}
				else
				{
					c = Moo::Colour( block->needsRefill() ? 0x80ff0000 : 0x8000ff00 );
				}
				Geometrics::drawRect( pos, pos2, c );
			}
		}

		Moo::rc().pop();

		floraTexture()->drawDebug();
	}
}

void Flora::drawPostDeferred()
{
	if ( s_debugBB )
	{
		Moo::Material::setVertexColour();
		Moo::rc().setPixelShader( NULL );
		Moo::rc().push();
		Moo::rc().world( Matrix::identity );

		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );

		FloraBlock * center = &blocks_[centerBlockZ_][centerBlockX_];

		for (size_t z = 0; z < BLOCK_STRIDE; ++z)
		{
			for (size_t x = 0; x < BLOCK_STRIDE; ++x)
			{
				const FloraBlock & block = blocks_[z][x];
				if ((!block.culled()) && (block.blockID() != -1))
				{
					if (&block != center)
					{
						Geometrics::wireBox( block.bounds(), 0xff0000ff );
					}
				}
			}
		}

		for ( int i=0; i<400; i++ )
		{
			bool fullyCulled = !!macroBB_[i].bb_.combinedOutcode();
			bool fullyInside = !macroBB_[i].bb_.outcode();
			uint32 col = ( fullyCulled ? 0xff000000 : (fullyInside?0xffffffff:0xff808080) );
			Geometrics::wireBox( macroBB_[i].bb_, col );
		}

		Geometrics::wireBox( center->bounds(), 0xff00ff00 );

		Moo::rc().pop();
	}
}

/**
 *	This method returns the ecotype for the given position.
 *	This method never returns NULL if there is no terrain block,
 *	or if there is no ecotype definition for the detailID
 *	in the Flora dataSection.  In these cases, the degenerate
 *	ecotype is returned ( which just creates degenerate triangles )
 */
Ecotype& Flora::ecotypeAt( const Vector2& p )
{
	BW_GUARD;
	if ( data_ )
	{
		Ecotype::ID id = ChunkFloraManager::instance().ecotypeAt(p);
		if (id == Ecotype::ID_AUTO)
		{
			id = this->generateEcotypeID(p);
		}

		// see if the ecotype exists / is valid
		if ( !ecotypes_[id] )
		{
			//TRACE_MSG( "Creating Ecotype %d\n", id );
			//create an ecotype.
			ecotypes_[id] = new Ecotype( this );				
		}

		// see if the ecotype needs loading
		if ( !ecotypes_[id]->isInited_ && !ecotypes_[id]->isLoading_ )
		{
			ecotypes_[id]->init( data_->openSection( "ecotypes" ), id );
		}		

		return *ecotypes_[id];
	}	

	//note degenerate ecotype always has its isLoading flag set to true.
	//thus whenever we return this ecotype, flora blocks will not fill
	//vertices using it.
	return degenerateEcotype_;
}

#ifdef EDITOR_ENABLED
bool Flora::highLight( BW::list<int>& indexPath, bool hightLight )
{
	BW_GUARD;
	int ecotype = indexPath.front();
	indexPath.pop_front();
	for (int i = 0; i< 256; ++i)
	{
		Ecotype* pEcotype = ecotypes_[i];
		if (pEcotype != NULL && pEcotype->id() == ecotype)
		{
			FloraVertexContainer* pVerts = pRenderer()->lock( 0, numVertices_ );
			pVerts->blocks_ = &blocks_[0][0];
			pEcotype->highLight( pVerts, indexPath, hightLight );
			pRenderer()->unlock( pVerts );
			break;
		}
	}
	return true;
}

#endif//EDITOR_ENABLED

/**
 *	This method automatically generates an EcotypeID if there is no
 *	explicit one set.
 */
Ecotype::ID Flora::generateEcotypeID( const Vector2& p )
{
	BW_GUARD;
	Vector3 pos( p.x, 0.f, p.y );
	Terrain::TerrainFinder::Details details = 
		Terrain::BaseTerrainBlock::findOutsideBlock( pos );

	if (details.pBlock_ && details.pBlock_->dominantTextureMap())
	{
		Terrain::BaseTerrainBlockPtr pBlock = details.pBlock_;
		Vector3 relPos = details.pInvMatrix_->applyPoint( pos );				
		Vector3 normal = pBlock->normalAt( relPos.x, relPos.z );

		if ( normal.y > cosMaxSlope_ )
		{
			const BW::string& textureName =
				pBlock->dominantTextureMap()->texture( relPos.x, relPos.z );	
			BW::StringRef texRoot = BWResource::removeExtension( textureName );

			StringEcotypeHashMap::const_iterator it = texToEcotype_.find( texRoot );

			if (it != texToEcotype_.end())
			{						
				return it->second;
			}
		}
	}

	return 0;
}


/**
 *	This method sets the center() value of every FloraBlock.
 */
void Flora::teleportCamera( const Vector2& camPos2 )
{
	BW_GUARD;
//	if ( !vb_ )
//		return;

	//TRACE_MSG( "Flora - teleport camera\n" );
	MF_ASSERT( numVerticesPerBlock_ != 0 );

	//move to top left, and snap camPos to 4 metre increments
	Vector2 topLeft = camPos2 + Vector2( -(float)BLOCK_STRIDE/2.f*BLOCK_WIDTH, (float)BLOCK_STRIDE/2.f*BLOCK_WIDTH );
	//find position of top left block
	float left = BLOCK_WIDTH * floorf( topLeft.x / BLOCK_WIDTH );
	float top = BLOCK_WIDTH * floorf( topLeft.y / BLOCK_WIDTH );

	int offset = 0;
	AABB spaceBounds =
		DeprecatedSpaceHelpers::cameraSpace()->bounds();

	//go through each block and give the position.
	Vector2 pos;
	pos.y = top - BLOCK_WIDTH/2.f;
	for ( int z=0; z<BLOCK_STRIDE; z++ )
	{
		pos.x = left + BLOCK_WIDTH/2.f;

		for ( int x=0; x<BLOCK_STRIDE; x++ )
		{
			FloraBlock & block = blocks_[z][x];
			block.init( pos, offset );
			offset += numVerticesPerBlock_;
			pos.x += BLOCK_WIDTH;
		}

		pos.y -= BLOCK_WIDTH;
	}

	cameraTeleport_ = false;

	//now fill blocks with correct vertices and stuff.
	movedBlocks_.clear();

	for (size_t z = 0; z < BLOCK_STRIDE; ++z)
	{
		for (size_t x = 0; x < BLOCK_STRIDE; ++x)
		{
			if (blocks_[z][x].withinBounds( spaceBounds ))
			{
				blocks_[z][x].fill( this, numVerticesPerBlock_ );
				if (blocks_[z][x].needsRefill())
				{
					movedBlocks_.insert( &blocks_[z][x] );
				}
			}
		}
	}

	centerBlockX_ = BLOCK_STRIDE/2;
	centerBlockZ_ = BLOCK_STRIDE/2;
	this->accumulateBoundingBoxes();
}


/**
 *	This method initialises the offsets_ array with
 *	vector2s inside a circle of BLOCKWIDTH/2 radius
 */
void Flora::initialiseOffsetTable( float blur /* = 2.f */ )
{
	BW_GUARD;
	srand(0); // Ensure that we will always created the same random tables

	float radius = BLOCK_WIDTH / 2.f;
	radius = sqrtf( 2.f * (radius * radius) );

	for ( int i=0; i<LUT_SIZE; i++ )
	{
		float t = ((float)rand() / (float)RAND_MAX) * MATH_2PI;
		float r = ((float)rand() / (float)RAND_MAX) * radius;
		offsets_[i].x = r * cosf(t);
		offsets_[i].y = r * sinf(t);
		randoms_[i] = (float)rand() / (float)RAND_MAX;
	}
}


uint32 Flora::maxVBSize() const
{
	return this->maxVBSize_;
}


void Flora::maxVBSize( uint32 bytes )
{
	BW_GUARD;
	maxVBSize_ = bytes;

	if (maxVBSize_ != 0)
	{
		//currently dimensions etc. are hard-coded.  they should
		//be put in the xml file.  still, we will assert that the
		//constants are correct.
		MF_ASSERT( MOVE_DIST == VISIBILITY * 2 );
		MF_ASSERT( BLOCK_STRIDE == (VISIBILITY * 2) / BLOCK_WIDTH );

		this->createUnmanagedObjects();
	}
	else
	{
		vbSize_ = 0;
	}
}


/**
 *	This method sets the index into the offsets_ array given
 *	a world position.  It is hoped that each world position
 *	gives a different seed.
 */
void Flora::seedOffsetTable( const Vector2& center )
{
	//some crazy deterministic function
	lutSeed_ = (int)( ( fabsf(center.x) * 5.f ) + ( fabsf(center.y) * 13.f ) );
}


/**
 *	This method returns the next Vector2 in the offsets_
 *	array.
 */
const Vector2& Flora::nextOffset()
{
	lutSeed_ = (lutSeed_+1)%LUT_SIZE;
	return offsets_[lutSeed_];
	/*static Vector2 s_zero2(0,0);
	return s_zero2;*/
}


float Flora::nextRotation()
{
	lutSeed_ = (lutSeed_+1)%LUT_SIZE;
	return fmodf( offsets_[lutSeed_].x, 6.283f );
}


float Flora::nextRandomFloat()
{
	lutSeed_ = (lutSeed_+1)%LUT_SIZE;
	return randoms_[lutSeed_];
}


/**
 *	This method finds a terrain block at the given position, and returns the relative
 *	position within that block.
 *
 *	TODO : cache the 4 terrain blocks used at any one time by flora.
 */
Terrain::BaseTerrainBlockPtr Flora::getTerrainBlock( const Vector3 & pos, Vector3 & relativePos, const Vector2 * refPt ) const
{
	BW_GUARD;
	if ( !refPt || *refPt != lastRefPt_ )
	{
		details_ = Terrain::BaseTerrainBlock::findOutsideBlock( pos );
		if ( refPt )
			lastRefPt_ = *refPt;
	}

	if (details_.pInvMatrix_ != NULL)
	{
		relativePos = details_.pInvMatrix_->applyPoint( pos );
	}

	return details_.pBlock_;
}


/**
 *	This method allocates a drawID.
 */
int Flora::getTerrainBlockID( const Matrix & terrainBlockTransform, const float terrainBlockSize ) const
{
	BW_GUARD;
	Vector3 pos = terrainBlockTransform.applyPoint( Vector3(terrainBlockSize/2.f,0,terrainBlockSize/2.f) );
	pos /= terrainBlockSize;
	int x = abs((int)(floorf(pos.x)));
	int z = abs((int)(floorf(pos.z)));
	return ( x%2 | (z%2 << 1) );
}


void Flora::resetBlockAt( float x, float z )
{
	BW_GUARD;
	//DEBUG_MSG( "------------------------------------------\n" );
	//DEBUG_MSG( "investigating %0.1f, %0.1f\n", x, z );

	FloraBlock * centerBlock = &blocks_[centerBlockZ_][centerBlockX_];
	Vector2 center = centerBlock->center();
	//DEBUG_MSG( "center block of idx x %d z %d, lies at %0.1f, %0.1f\n",
	//	centerBlockX_, centerBlockZ_, center.x, center.y );

	float dx = x - center.x;
	float dz = z - center.y;
	if ( fabsf(dx) >= VISIBILITY )
		return;
	if ( fabsf(dz) >= VISIBILITY )
		return;

	int dxIdx = dx > 0.f ? int(dx/BLOCK_WIDTH + 0.5f) : int(dx/BLOCK_WIDTH - 0.5f);
	int dzIdx = dz > 0.f ? int(dz/BLOCK_WIDTH + 0.5f) : int(dz/BLOCK_WIDTH - 0.5f);

	//DEBUG_MSG( "investigated point is x %d by z %d blocks away from center block\n", dxIdx, dzIdx );
	int xIdx = ((centerBlockX_ + dxIdx) + BLOCK_STRIDE) % BLOCK_STRIDE;
	int zIdx = ((centerBlockZ_ - dzIdx) + BLOCK_STRIDE) % BLOCK_STRIDE;
	//DEBUG_MSG( "block under investigation x %d, z %d\n", xIdx, zIdx );

	MF_ASSERT( xIdx >= 0 )
	MF_ASSERT( zIdx >= 0 )
	MF_ASSERT( xIdx < BLOCK_STRIDE )
	MF_ASSERT( zIdx < BLOCK_STRIDE )

	FloraBlock * block = &blocks_[zIdx][xIdx];
	AABB spaceBounds =
		DeprecatedSpaceHelpers::cameraSpace()->bounds();

	if (block->withinBounds( spaceBounds ))
	{
		if (!block->needsRefill())
		{
			block->invalidate();
			movedBlocks_.insert( block );
		}
	}
}


/*~ function BigWorld.floraReset
 *	@components{ client, tools }
 *
 *	This method will re-initialise the flora vertex buffer, i.e. repopulating
 *	all flora objects in the vicinity of the camera.  This needs to be called
 *	if for example you have changed the height map and need the flora to be
 *	re-seeded onto the new terrain mesh.
 */
/**
 *	This method resets the flora, recalaculating all flora objects.
 */
/*static*/ void Flora::floraReset()
{
	BW_GUARD;
	const BW::vector< Flora* > & floras = Flora::floras();
	for( unsigned i=0; i<floras.size(); i++)
	{
		floras[i]->cameraTeleport_ = true;
	}
}
PY_MODULE_STATIC_METHOD( Flora, floraReset, BigWorld )


/**
 *	This method sets the number of vertices used by the flora and
 *	reinitialises all dependencies.  Can be called at runtime to 
 *	change the flora detail level.
 */
void Flora::vbSize( uint32 numBytes )
{
	BW_GUARD;
	//no vbSize before init()
	if ( !data_ )
		return;

	if ( pRenderer_ )
	{
		pRenderer_->fini();
	}

	if ( numBytes == 0 )
	{
		numVertices_ = 0;
		numVerticesPerBlock_ = 0;
		vbSize_ = 0;
		return;
	}

	//can only call this function after the renderer has been created.
	if (!pRenderer_)
	{
		pRenderer_ = new FloraRenderer( this );
	}

	cameraTeleport_ = true;

	uint32 vSize = pRenderer_->vertexSize();	
	numVertices_ =  numBytes / vSize;

	//check graphics card caps here for maximum vertex buffer size.
	uint32 maxPrims = Moo::rc().deviceInfo(0).caps_.MaxPrimitiveCount;
	if (numVertices_ > maxPrims * 3)
	{	
		numVertices_ = maxPrims * 3;
		numBytes = numVertices_ * vSize;
		INFO_MSG( "Flora::createUnmanagedObjects - reduced memory usage to %d, due to VB size restriction\n", numBytes );
	}

	//Make a check - the number of vertices allocated to each flora block must be exactly
	//divisible by 3, or else the triangle lists won't work
	uint32 blockSize = (BLOCK_STRIDE*BLOCK_STRIDE);
	numVerticesPerBlock_  = std::max(numVertices_ / blockSize, (uint32)3U);
	if ( numVerticesPerBlock_%3 != 0 )
	{
		numVerticesPerBlock_ -= numVerticesPerBlock_ % 3;
		numVertices_ = numVerticesPerBlock_ * blockSize;
		numBytes = numVertices_ * vSize;
		INFO_MSG( "Flora::vbSize - reduced memory usage to %d, due to alignment requirements\n", numBytes );
	}
	numVertices_ = numVerticesPerBlock_ * blockSize;
	numBytes = numVertices_ * vSize;
	vbSize_ = numBytes;

	INFO_MSG( "Flora::vbSize - %d triangles per flora block\n", numVerticesPerBlock_ / 3 );
	INFO_MSG( "Flora::vbSize - %d KB used for vertex buffer\n", vbSize_ / 1024 );

	if (!pRenderer_->init( numBytes ))
	{
		bw_safe_delete(pRenderer_);
	}	

	//Must refill all blocks, since our vertex buffer etc. will have been recreated.
	cameraTeleport_ = true;
}

/*~ function BigWorld.floraVBSize
 *	@components{ client, tools }
 *
 *	This method sets the vertex buffer size for the Flora.
 *
 *	@param Number of bytes as an integer.
 */
/**
 *	This method sets the vertex buffer size of the flora.
 */
/*static*/ void Flora::floraVBSize( uint32 numBytes )
{
	BW_GUARD;
	const BW::vector< Flora* > & floras = Flora::floras();
	for( unsigned i=0; i<floras.size(); i++)
	{
		floras[i]->maxVBSize( numBytes );
	}
}

PY_MODULE_STATIC_METHOD( Flora, floraVBSize, BigWorld )

// -----------------------------------------------------------------------------
// Section: FloraSettings
// -----------------------------------------------------------------------------

/**
 *	Register the graphics settings controlled by 
 *	the envirominder. Namely, the FAR_PLANE setting.
 *
 *	The flora density options are defined in graphics_settings.xml (or 
 *	whatever xml file is set as the graphics settings file in resources.xml).
 *
 *	Below is an example of how to setup the options in flora.xml:
 *
 * \verbatim
 *	<flora>
 *		<vbSize>
 *			<option>
 *				<label>	HIGH	</label>
 *				<value>	1.0	</value>
 *			</option>
 *			<option>
 *				<label>	LOW	</label>
 *				<value>	0.5	</value>
 *			</option>
 *			<option>
 *				<label>	OFF	</label>
 *				<value>	0	</value>
 *			</option>
 *		</vbSize>
 *	</flora>
 * \endverbatim
 *
 *	For each \<option\> entry in \<vbSize\>, \<label\> will be used
 *	to identify the option and vb_size will be used to set the size of
 *	the vertex buffer used to render the flora. Zero has the special
 *	meaning of disabling flora rendering. See Flora::vbSize().
 *	
 */
void FloraSettings::init( DataSectionPtr resXML )
{
	BW_GUARD;

	//-- disable flora setting based on the available on chip video memory.
	bool supported = Moo::GraphicsSettingsAutoDetector::getTotalVideoMemory() > 320;

	// far plane settings
	this->floraSettings_ = 
		Moo::makeCallbackGraphicsSetting(
			getFloraSettingId(), "Flora Quality", *this, 
			&FloraSettings::setFloraOption, -1, 
			false, false);

	if (resXML.exists())
	{
		DataSectionIterator sectIt  = resXML->begin();
		DataSectionIterator sectEnd = resXML->end();
		while (sectIt != sectEnd)
		{
			static const float undefined = -1.0f;
			float value = (*sectIt)->readFloat("value", undefined);
			BW::string label = (*sectIt)->readString("label");
			if (!label.empty() && value != undefined)
			{
				//-- Note (b_sviglo): All options is advanced except OFF option.
				bool isSupported = value > 0 ? supported : true;
				bool isAdvanced  = value > 0 ? true : false;

				this->floraSettings_->addOption( label, label, isSupported, isAdvanced );
				this->floraOptions_.push_back( value );
			}
			++sectIt;
		}
	}
	else
	{
		this->floraSettings_->addOption( "MAX", "Max", supported, true );
		this->floraOptions_.push_back( 1.0f );

		this->floraSettings_->addOption( "OFF", "Off", true, false );
		this->floraOptions_.push_back( 0.0f );
	}
	Moo::GraphicsSetting::add(this->floraSettings_);
}


/**
 *	Sets the flora density. This value will be adjusted by the 
 *	global flora vb-size settings before it gets set into the camera.
 */
float FloraSettings::vbRatio() const
{
	BW_GUARD;
	// FloraSettings needs 
	// to be initialised first
	return this->floraSettings_.exists()
		? this->floraOptions_[this->floraSettings_->activeOption()]
		: 1.0f;
}


/**
 *	Returns true if settings have been initialised.
 */
bool FloraSettings::isInitialised() const
{
	BW_GUARD;
	return this->floraSettings_.exists();
}


/**
 *	Returns singleton FloraSettings instance.
 */
FloraSettings & FloraSettings::instance()
{
	static FloraSettings instance;
	return instance;
}


void FloraSettings::setFloraOption(int optionIndex)
{
	BW_GUARD;
	float ratio = this->floraOptions_[optionIndex];

	const BW::vector< Flora* > & floras = Flora::floras();
	for( unsigned i=0; i<floras.size(); i++)
	{
		floras[i]->vbSize(int32(floras[i]->maxVBSize() * this->vbRatio()));
	}

	Flora::enabled( ratio != 0.f ); // Disable flora if the ratio is zero
}


/*static */const char * FloraSettings::getFloraSettingId()
{
	return "FLORA_QUALITY";
}

BW_END_NAMESPACE

// flora.cpp
