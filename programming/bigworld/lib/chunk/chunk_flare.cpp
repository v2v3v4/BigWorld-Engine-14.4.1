#include "pch.hpp"
#include "chunk_flare.hpp"

#include "resmgr/bwresource.hpp"
#include "math/colour.hpp"
#include "romp/lens_effect_manager.hpp"

#if UMBRA_ENABLE
#include "umbra_chunk_item.hpp"
#endif

#include "chunk.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkFlare
// -----------------------------------------------------------------------------

int ChunkFlare_token = 0;

/**
 *	Constructor.
 */
ChunkFlare::ChunkFlare() :
	ChunkItem( WANTS_DRAW ),
	position_( Vector3::ZERO ),
	colour_( 255.f, 255.f, 255.f ),
	colourApplied_( false ),
	type2_()
{
}


/**
 *	Destructor.
 */
ChunkFlare::~ChunkFlare()
{
}


/**
 *	Load method
 */
bool ChunkFlare::load( DataSectionPtr pSection, Chunk* pChunk )
{
	BW_GUARD;
	BW::string	resourceID = pSection->readString( "resource" );
	if (resourceID.empty()) return false;

	DataSectionPtr pFlareRoot = BWResource::openSection( resourceID );
	if (!pFlareRoot) return false;

	// ok, we're committed to loading now.
	// since we support reloading, remove old effects first
	for (uint i = 0; i < lensEffects_.size(); i++)
	{
		LensEffectManager::instance().forget( i + (size_t)this );
	}
	lensEffects_.clear();

	LensEffect le;
	if (le.load( pFlareRoot ))
	{
		le.maxDistance( pSection->readFloat( "maxDistance", le.maxDistance() ) );
		le.area( pSection->readFloat( "area", le.area() ) );
		le.fadeSpeed( pSection->readFloat( "fadeSpeed", le.fadeSpeed() ) );
		lensEffects_.push_back( le );
	}

	position_ = pSection->readVector3( "position" );
	DataSectionPtr pColourSect = pSection->openSection( "colour" );
	if (pColourSect)
	{
		colour_ = pColourSect->asVector3();
		colourApplied_ = true;
	}
	else
	{
		colour_.setZero();
		colourApplied_ = false;
	}

	return true;
}


void ChunkFlare::syncInit()
{
	BW_GUARD;	
#if UMBRA_ENABLE
	bw_safe_delete(pUmbraDrawItem_);
	BoundingBox bb( Vector3(-0.5f,-0.5f,-0.5f), Vector3(0.5f,0.5f,0.5f) );

	// Set up object transforms
	Matrix m = this->pChunk_->transform();
	Matrix tr;
	tr.setTranslate( position_ );
	m.preMultiply( tr );

	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell() );
	pUmbraDrawItem_ = pUmbraChunkItem;
	this->updateUmbraLenders();
#endif
}

/**
 *	The draw function ... add our lens effects to the list
 */
void ChunkFlare::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	//during some rendering passes we ignore all lens flares...
	if ( s_ignore )
		return;

	static DogWatch drawWatch( "ChunkFlare" );
	ScopedDogWatch watcher( drawWatch );

	size_t leid = (size_t)this;

	Vector4 tintColour( colour_ / 255.f, 1.f );

	LensEffects::iterator it = lensEffects_.begin();
	LensEffects::iterator end = lensEffects_.end();
	while( it != end )
	{
		LensEffect & le = *it++;

		uint32 oldColour = 0;

		if (colourApplied_)
		{
			// Multiply our tinting colour with the flare's base colour
			oldColour = le.colour();
			Vector4 flareColour( Colour::getVector4Normalised( oldColour ) );
			flareColour.w *= tintColour.w;
			flareColour.x *= tintColour.x;
			flareColour.y *= tintColour.y;
			flareColour.z *= tintColour.z;
			le.colour( Colour::getUint32FromNormalised( flareColour ) );
		}

		LensEffectManager::instance().add(
			leid++, Moo::rc().world().applyPoint( position_ ), le );

		if (colourApplied_)
		{
			le.colour( oldColour );
		}
	}
}


/**
 *	This static method creates a flare from the input section and adds
 *	it to the given chunk.
 */
ChunkItemFactory::Result ChunkFlare::create( Chunk * pChunk, DataSectionPtr pSection )
{
	BW_GUARD;
	ChunkFlare * pFlare = new ChunkFlare();

	if (!pFlare->load( pSection, pChunk ))
	{
		bw_safe_delete(pFlare);
		return ChunkItemFactory::Result( NULL,
			"Failed to load flare " + pSection->readString( "resource" ) );
	}
	else
	{
		pChunk->addStaticItem( pFlare );
		return ChunkItemFactory::Result( pFlare );
	}
}


/// Static factory initialiser
ChunkItemFactory ChunkFlare::factory_( "flare", 0, ChunkFlare::create );


void ChunkFlare::ignore( bool state )
{
	ChunkFlare::s_ignore = state;
}

bool ChunkFlare::s_ignore = false;

BW_END_NAMESPACE

// chunk_flare.cpp
