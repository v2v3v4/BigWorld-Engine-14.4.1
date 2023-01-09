#include "pch.hpp"

#include "chunk_particles.hpp"

#include "particle_system_manager.hpp"
#include "meta_particle_system.hpp"
#include "chunk/chunk.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "moo/geometrics.hpp"
#ifdef EDITOR_ENABLED
#include "appmgr/options.hpp"
#endif

#include "chunk/umbra_config.hpp"
#if UMBRA_ENABLE
#include "chunk/umbra_chunk_item.hpp"
#endif
#include "moo/draw_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkParticles
// -----------------------------------------------------------------------------
char * g_chunkParticlesLabel = "ChunkParticles";

int ChunkParticles_token = 0;
static uint32 s_staggerIdx = 0;
static BasicAutoConfig<int> s_staggerInterval( "environment/chunkParticleStagger", 50 );


class ReflectionVisibleStateBlock : public Moo::GlobalStateBlock
{
public:
	virtual	void	apply( Moo::ManagedEffect* effect, ApplyMode mode )
	{
		if (mode == APPLY_MODE)
		{
			// disable zwrite
			Moo::rc().pushRenderState( D3DRS_ZWRITEENABLE ); 
			Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE ); 
		}
		else if (mode == UNDO_MODE)
		{
			Moo::rc().popRenderState(); 
		}
	}
};

/**
 *	Constructor.
 */
ChunkParticles::ChunkParticles() :
	ChunkItem( (WantFlags)(WANTS_DRAW | WANTS_TICK) ),
	seedTime_( 0.1f ),	
	staggerIdx_( s_staggerIdx++ ),
	reflectionGlobalState_( NULL )
{
	ParticleSystemManager::init();
}


/**
 *	Destructor.
 */
ChunkParticles::~ChunkParticles()
{
	bw_safe_delete( reflectionGlobalState_ );
	ParticleSystemManager::fini();
}


/**
 *	This method ensures that particle systems that are chunk items
 *	get seeded once they are placed in the world.  This is done
 *	for off-screen particles, in a staggered fashion.
 */
void ChunkParticles::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkParticles_tick );

	if ( !system_ )
		return;

	if (seedTime_ > 0.f)
	{		
		if ((staggerIdx_++ % s_staggerInterval.value()) == 0)
		{		
			system_->setDoUpdate();
			seedTime_ -= dTime;
		}
	}	

	bool updated = system_->tick( dTime );

	if (pChunk_)
	{
		// Update the visibility bounds of our parent chunk
		pChunk_->space()->updateOutsideChunkInQuadTree( pChunk_ );
	}

#if UMBRA_ENABLE
	// Update umbra object
	if (!pUmbraDrawItem_)
	{
		// Set up our unit boundingbox for the attachment, we use a unit bounding box
		// and scale it using transforms since this is a dynamic object.
		UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
		pUmbraChunkItem->init( this, BoundingBox( Vector3(0,0,0), Vector3(1,1,1)), Matrix::identity, NULL );
		pUmbraDrawItem_ = pUmbraChunkItem;
		updated = true;
	}

	if (pChunk_ != NULL)
	{
		if ( updated )
		{
			// Get the object to cell transform
			Matrix m = Matrix::identity;
			BoundingBox bb( BoundingBox::s_insideOut_ );
			system_->worldVisibilityBoundingBox(bb);

			// Create the scale transform from the bounding box
			Vector3 scale(0,0,0);
			if (!bb.insideOut())
				scale = bb.maxBounds() - bb.minBounds();

			// Const number to see if the bounding box is too big for umbra.
			const float TOO_BIG_FOR_UMBRA = 100000.f;

			if (scale.x != 0.f && scale.y != 0.f && scale.z != 0.f &&
				scale.x < TOO_BIG_FOR_UMBRA && scale.y < TOO_BIG_FOR_UMBRA && scale.z < TOO_BIG_FOR_UMBRA)
			{
				// Set up our transform, the transform includes the scale of the bounding box
				Matrix m2;
				m2.setTranslate( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
				m.preMultiply( m2 );

				m2.setScale( scale.x, scale.y, scale.z );
				m.preMultiply( m2 );
				pUmbraDrawItem_->updateCell( pChunk_->getUmbraCell() );
				pUmbraDrawItem_->updateTransform( m );
			}
			else
			{			
				pUmbraDrawItem_->updateCell( NULL );
			}
		}
	}
	else
	{
		pUmbraDrawItem_->updateCell( NULL );
	}
#endif	
}


/**
 *	Draw the BoundingBox and VisibilityBox for the Particle System
 *	The parameters passed in are ignored, the details coming directly from the ChunkParticle.
 */
void ChunkParticles::drawBoundingBoxes( const BoundingBox &bb, const BoundingBox &vbb, const Matrix &spaceTrans ) const 
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	static uint32 s_settingsMark = -1;
	static bool s_useDebugBB = false;

	if (Moo::rc().frameTimestamp() != s_settingsMark)
	{
		s_useDebugBB = !!Options::getOptionInt("debugBB", 0 );
		s_settingsMark = Moo::rc().frameTimestamp();
	}

	if (s_useDebugBB)
	{
		BoundingBox drawBB = BoundingBox::s_insideOut_;
		system_->localBoundingBox( drawBB );
		drawBB.transformBy( worldTransform_ );
		Geometrics::addWireBoxWorld( drawBB, 0x00ffff00 );
		
		BoundingBox drawVBB = BoundingBox::s_insideOut_;
		system_->worldVisibilityBoundingBox( drawVBB );
		Geometrics::addWireBoxWorld( drawVBB, 0x000000ff );
	}
#endif
}

/**
 *	This return the visibility boundingbox of the system in world space
 *
 *	@param	bb		The resultant bounding box.
 */
void ChunkParticles::worldVisibilityBoundingBox( BoundingBox & bb, bool clipChunkSize ) const
{
	BW_GUARD;	

	if (system_)
	{
		system_->worldVisibilityBoundingBox( bb);
	}

	if (clipChunkSize)
	{
		const float gridSize = pChunk_->space()->gridSize();
		// clip the size with ( gridSize, 2*gridSize, gridSize )
		float scale = max( max( bb.width() / gridSize,
			bb.depth() / gridSize ),
			bb.height() / ( 2.f * gridSize ) );
		if (scale > 1.f)
		{
			Vector3 center = bb.centre();
			Vector3 min = center - ( center - bb.minBounds() ) / scale;
			Vector3 max = center + ( bb.maxBounds() - center ) / scale;
			bb = BoundingBox( min, max );
		}
	}
}


/**
 *	overridden lend method
 */
void ChunkParticles::lend( Chunk * pLender )
{
	BW_GUARD;

	BoundingBox bb;
	this->worldVisibilityBoundingBox( bb, true );
	this->lendByBoundingBox( pLender, bb );
}

/**
 *	Load method
 */
bool ChunkParticles::load( DataSectionPtr pSection )
{
	BW_GUARD;
	localTransform_ = pSection->readMatrix34( "transform", Matrix::identity );
	/*
	 * Make world transform identical to local transform, so clearBoundingBox
	 * won't be using full zero matrix introducing floating point errors
	*/
	worldTransform_ = localTransform_;

	BW::string resourceID = pSection->readString( "resource" );
	if (resourceID.empty())
	{
		return false;
	}

	// Particles used to not have a path, support that by defaulting to particles/
	if (resourceID.find( '/' ) == BW::string::npos)
	{
		resourceID = "particles/" + resourceID;
	}

	DataSectionPtr resourceDS = BWResource::openSection( resourceID );
	if (!resourceDS)
	{
		return false;
	}

	// ok, we're committed to loading now.
	seedTime_ = resourceDS->readFloat( "seedTime", seedTime_ );
	if (pSection->readBool( "reflectionVisible", false ))
	{
		reflectionGlobalState_ = new ReflectionVisibleStateBlock;
	}

	system_ = ParticleSystemManager::instance().loader().load( resourceID );
	system_->isStatic(true);
	system_->attach(this);

	return true;
}

bool ChunkParticles::setReflectionVis( const bool& reflVis )
{ 
	if (!reflectionGlobalState_)
	{
		reflectionGlobalState_ = new ReflectionVisibleStateBlock;
	}
	return true;
}

/*virtual*/ void ChunkParticles::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	if (Moo::rc().reflectionScene() && reflectionGlobalState_)
		return;

	if ( !system_ )
		return;

	if (reflectionGlobalState_)
	{
		drawContext.pushGlobalStateBlock( reflectionGlobalState_ );
	}

	float distance = 
		(worldTransform_.applyToOrigin() -
		Moo::rc().invView().applyToOrigin()).length();	

	system_->draw( drawContext, worldTransform_, distance );

	if (reflectionGlobalState_)
	{
		drawContext.popGlobalStateBlock( reflectionGlobalState_ );
	}
}


void ChunkParticles::load( const BW::StringRef & resourceName )
{
	BW_GUARD;
	if (resourceName.empty())
	{
		return;
	}

	// Particles used to not have a path, support that by defaulting to particles/
	BW::string resourceID;
	if (resourceName.find( '/' ) == BW::string::npos)
	{
		resourceID = "particles/" + resourceName;
	}
	else
	{
		resourceID.assign( resourceName.data(), resourceName.size() );
	}

	DataSectionPtr pSystemRoot = BWResource::openSection( resourceID );
	if (!pSystemRoot)
	{
		return;
	}

	seedTime_ = pSystemRoot->readFloat( "seedTime", seedTime_ );
	system_ = ParticleSystemManager::instance().loader().load( resourceID );
	system_->isStatic(true);
	system_->attach(this);
}


/** 
 *	Toss method
 */
void ChunkParticles::toss( Chunk * pChunk )
{
	BW_GUARD;
	Chunk * oldChunk = pChunk_;
	bool wasNowhere = pChunk_ == NULL;

	// call base class method
	this->ChunkItem::toss( pChunk );

	Matrix m = localTransform_;
	if (pChunk_ != NULL)
	{
		if (wasNowhere)
		{
			m.postMultiply( pChunk_->transform() );
			this->setMatrix(m);		
			if ( system_ )
				system_->enterWorld();
		}
		else
		{
			if ( system_ )
				system_->leaveWorld();

			m.postMultiply( pChunk_->transform() );
			this->setMatrix(m);
			if ( system_ )
				system_->enterWorld();
		}
	}
	else
	{
		if (!wasNowhere)
		{
			if ( system_ )
				system_->leaveWorld();
		}
	}	
}


/*
 *	label (to determine the type of chunk item)
 */
const char * ChunkParticles::label() const
{
	return g_chunkParticlesLabel; 
}


/**
 *	This static method creates a particle system from the input section and adds
 *	it to the given chunk.
 */
ChunkItemFactory::Result ChunkParticles::create( Chunk * pChunk, DataSectionPtr pSection )
{
	BW_GUARD;
	SmartPointer<ChunkParticles> pParticles = new ChunkParticles();

	if (!pParticles->load( pSection ))
	{

		BW::string err = LocaliseUTF8( L"PARTICLE/CHUNK_PARTICLES/FAILED_LOAD" ); 
		if ( pSection )
		{
			err += '\'' + pSection->readString( "resource" ) + '\'';
		}
		else
		{
			err += "(NULL)";
		}

		return ChunkItemFactory::Result( NULL, err );
	}
	else
	{ 
		pChunk->addStaticItem( &*pParticles );
		return ChunkItemFactory::Result( pParticles );
	}
}

bool ChunkParticles::addYBounds( BoundingBox& bb ) const
{
	if ( system_ )
	{
		BoundingBox me;
		system_->localVisibilityBoundingBox( me );
		me.transformBy( localTransform_ );
		bb.addYBounds( me.minBounds().y );
		bb.addYBounds( me.maxBounds().y );
		return true;
	}
	else 
	{
		WARNING_MSG("ChunkParticles::addYBounds but no system_ available");
		return false;
	}
}

/// Static factory initialiser
ChunkItemFactory ChunkParticles::factory_(
	"particles", 0, ChunkParticles::create );

BW_END_NAMESPACE

// chunk_particles.cpp
