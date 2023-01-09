#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_flare.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "appmgr/options.hpp"
#include "model/super_model.hpp"
#include "moo/geometrics.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/umbra_chunk_item.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/resource_cache.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorChunkFlare
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkFlare::EditorChunkFlare()
{
	BW_GUARD;

#if UMBRA_ENABLE
	this->wantFlags_ = WantFlags(
		this->wantFlags_ | WANTS_TICK | WANTS_UPDATE | WANTS_DRAW );
#else
	this->wantFlags_ = WantFlags(
		this->wantFlags_ | WANTS_UPDATE | WANTS_DRAW );
#endif

	flareModel_ = Model::get( "resources/models/flare.model" );
	flareModelSmall_ = Model::get( "resources/models/flare_small.model" );
	ResourceCache::instance().addResource( flareModel_ );
	ResourceCache::instance().addResource( flareModelSmall_ );
}


/**
 *	Destructor.
 */
EditorChunkFlare::~EditorChunkFlare()
{
}


/**
 *	This method saves the data section pointer before calling its
 *	base class's load method
 */
bool EditorChunkFlare::load( DataSectionPtr pSection, Chunk* pChunk, BW::string* errorString )
{
	BW_GUARD;

	bool ok = this->EditorChunkSubstance<ChunkFlare>::load( pSection, pChunk );
	if (ok)
	{
		flareRes_ = pSection->readString( "resource" );
	}
	else if ( errorString )
	{
		*errorString = "Failed to load flare '" + pSection->readString( "resource" ) + "'";
	}
	return ok;
}



/**
 *	Save any property changes to this data section
 */
bool EditorChunkFlare::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!edCommonSave( pSection ))
		return false;

	pSection->writeString( "resource", flareRes_ );
	pSection->writeVector3( "position", position_ );

	if (!lensEffects_.empty())
	{
		LensEffect& le = *lensEffects_.begin();
		pSection->writeFloat( "maxDistance", le.maxDistance() );
		pSection->writeFloat( "area", le.area() );
		pSection->writeFloat( "fadeSpeed", le.fadeSpeed() );
	}

	pSection->delChild( "colour" );
	if (colourApplied_) pSection->writeVector3( "colour", colour_ );

	return true;
}


/**
 *	Get the current transform
 */
const Matrix & EditorChunkFlare::edTransform()
{
	transform_.setIdentity();
	transform_.translation( position_ );

	return transform_;
}


/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkFlare::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyToOrigin() );
	if (pNewChunk == NULL) return false;

	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		transform_ = m;
		position_ = transform_.applyToOrigin();
		this->syncInit();
		return true;
	}

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;

	// ok, accept the transform change then
	transform_.multiply( m, pOldChunk->transform() );
	transform_.postMultiply( pNewChunk->transformInverse() );
	position_ = transform_.applyToOrigin();

	// note that both affected chunks have seen changes
	WorldManager::instance().changedChunk( pOldChunk );
	if (pNewChunk != pOldChunk )
	{
		WorldManager::instance().changedChunk( pNewChunk );
	}

	edMove( pOldChunk, pNewChunk );

	this->syncInit();
	return true;
}




/**
 *	This class checkes whether or not a data section is a suitable flare
 *	resource.
 */
class FlareResourceChecker : public ResourceProperty::Checker
{
public:
	virtual bool check( DataSectionPtr pRoot ) const
	{
		BW_GUARD;

		return !!pRoot->openSection( "Flare" );
	}

	static FlareResourceChecker instance;
};

FlareResourceChecker FlareResourceChecker::instance;


/**
 *	This helper class wraps up a flare's colour property
 */
class FlareColourWrapper : public UndoableDataProxy<ColourProxy>
{
public:
	explicit FlareColourWrapper( EditorChunkFlarePtr pItem ) :
		pItem_( pItem )
	{
	}

	virtual Moo::Colour get() const
	{
		BW_GUARD;

		return pItem_->colour();
	}

	virtual void setTransient( Moo::Colour v )
	{
		BW_GUARD;

		pItem_->colour( v );
	}

	virtual bool setPermanent( Moo::Colour v )
	{
		BW_GUARD;

		// make it valid
		if (v.r < 0.f) v.r = 0.f;
		if (v.r > 1.f) v.r = 1.f;
		if (v.g < 0.f) v.g = 0.f;
		if (v.g > 1.f) v.g = 1.f;
		if (v.b < 0.f) v.b = 0.f;
		if (v.b > 1.f) v.b = 1.f;
		v.a = 1.f;

		// set it
		this->setTransient( v );

		// flag the chunk as having changed
		WorldManager::instance().changedChunk( pItem_->chunk() );
		pItem_->edPostModify();

		// update its data section
		pItem_->edSave( pItem_->pOwnSect() );

		return true;
	}

	virtual BW::string opName()
	{
		BW_GUARD;

		return LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/SET_COLOUR", pItem_->edDescription().c_str() );
	}

private:
	EditorChunkFlarePtr	pItem_;
};


/**
 *	Add the properties of this flare to the given editor
 */
bool EditorChunkFlare::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	STATIC_LOCALISE_NAME( s_flare, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/FLARE" );
	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/POSITION" );
	STATIC_LOCALISE_NAME( s_colour, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/COLOUR" );
	STATIC_LOCALISE_NAME( s_maxDistance, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/MAXDISTANCE" );
	STATIC_LOCALISE_NAME( s_area, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/AREA" );
	STATIC_LOCALISE_NAME( s_fadeSpeed, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_FLARE/FADESPEED" );

	editor.addProperty( new ResourceProperty( s_flare,
		new SlowPropReloadingProxy<EditorChunkFlare,StringProxy>(
			this, "flare resource", 
			&EditorChunkFlare::flareResGet, 
			&EditorChunkFlare::flareResSet ),
		".xml",
		FlareResourceChecker::instance ) );

	MatrixProxy * pMP = new ChunkItemMatrix( this );
	editor.addProperty( new GenPositionProperty( s_position, pMP ) );

	editor.addProperty( new ColourProperty( s_colour, new FlareColourWrapper( this ) ) );

	if (!lensEffects_.empty())
	{
		editor.addProperty( new GenFloatProperty( s_maxDistance,
			new AccessorDataProxy<EditorChunkFlare,FloatProxy>(
				this, "maxDistance", 
				&EditorChunkFlare::getMaxDistance, 
				&EditorChunkFlare::setMaxDistance ) ) );

		editor.addProperty( new GenFloatProperty( s_area,
			new AccessorDataProxy<EditorChunkFlare,FloatProxy>(
				this, "area", 
				&EditorChunkFlare::getArea, 
				&EditorChunkFlare::setArea) ) );

		editor.addProperty( new GenFloatProperty( s_fadeSpeed,
			new AccessorDataProxy<EditorChunkFlare,FloatProxy>(
				this, "fadeSpeed", 
				&EditorChunkFlare::getFadeSpeed, 
				&EditorChunkFlare::setFadeSpeed ) ) );
	}

	return true;
}



void EditorChunkFlare::tick( float /*dTime*/ )
{
	BW_GUARD;

	ModelPtr model = reprModel();
	if (currentModel_ != model)
	{
		currentModel_ = model;
#if UMBRA_ENABLE
		this->syncInit();
#endif
		updateReprModel();
	}

}



void EditorChunkFlare::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (!edShouldDraw())
		return;

	ModelPtr model = reprModel();
	
	if (WorldManager::instance().drawSelection() && model)
	{
		// draw a some points near the centre of the reprModel, so the system
		// can be selected from the distance where the repr model might be
		// smaller than a pixel and fail to draw.
		Moo::rc().push();
		Moo::rc().world( chunk()->transform() );
		Moo::rc().preMultiply( edTransform() );
		// bias of half the size of the representation model's bounding box in
		// the vertical axis, because the object might be snapped to terrain
		// or another object, so the centre might be below something else.
		float bias = model->boundingBox().width() / 2.0f;
		Vector3 points[3];
		points[0] = Vector3( 0.0f, -bias, 0.0f );
		points[1] = Vector3( 0.0f, 0.0f, 0.0f );
		points[2] = Vector3( 0.0f, bias, 0.0f );
		union
		{
			DWORD argb;
			void * ptr;
		} cast;
		cast.ptr = this;
		Geometrics::drawPoints( points, 3, 3.0f, cast.argb );
		Moo::rc().pop();
	}

	if (!edIsTooDistant() || WorldManager::instance().drawSelection())
	{
		// Render both the proxy and the flare
		EditorChunkSubstance<ChunkFlare>::draw( drawContext );
	}
	else
	{
		// Too far away for the proxy, just render the flare.
		ChunkFlare::draw( drawContext );
	}
}


/**
 *	This method gets our colour as a moo colour
 */
Moo::Colour EditorChunkFlare::colour() const
{
	Vector4 v4col = colourApplied_ ?
		Vector4( colour_ / 255.f, 1.f ) :
		Vector4( 1.f, 1.f, 1.f, 1.f );

	return Moo::Colour( v4col );
}

/**
 *	This method sets our colour (and colourApplied flag) from a moo colour
 */
void EditorChunkFlare::colour( const Moo::Colour & c )
{
	colour_ = Vector3( c.r, c.g, c.b ) * 255.f;
	colourApplied_ = !(c.r == 1.f && c.g == 1.f && c.b == 1.f);
}


/**
 *	This returns the max distance
 */
float EditorChunkFlare::getMaxDistance() const
{
	BW_GUARD;

	return (*lensEffects_.begin()).maxDistance();
}


/**
 *	This sets max distance for all lens effects
 */
bool EditorChunkFlare::setMaxDistance( const float& maxDistance )
{
	BW_GUARD;

	for (LensEffects::iterator iter = lensEffects_.begin();
		iter != lensEffects_.end(); ++iter)
	{
		iter->maxDistance( maxDistance );
	}

	return true;
}


/**
 *	This returns the area
 */
float EditorChunkFlare::getArea() const
{
	BW_GUARD;

	return (*lensEffects_.begin()).area();
}


/**
 *	This sets area for all lens effects
 */
bool EditorChunkFlare::setArea( const float& area )
{
	BW_GUARD;

	for (LensEffects::iterator iter = lensEffects_.begin();
		iter != lensEffects_.end(); ++iter)
	{
		iter->area( area );
	}

	return true;
}


/**
 *	This returns the fade speed
 */
float EditorChunkFlare::getFadeSpeed() const
{
	BW_GUARD;

	return (*lensEffects_.begin()).fadeSpeed();
}


/**
 *	This sets fade speed for all lens effects
 */
bool EditorChunkFlare::setFadeSpeed( const float& fadeSpeed )
{
	BW_GUARD;

	for (LensEffects::iterator iter = lensEffects_.begin();
		iter != lensEffects_.end(); ++iter)
	{
		iter->fadeSpeed( fadeSpeed );
	}

	return true;
}


/**
 *	Return a modelptr that is the representation of this chunk item
 */
ModelPtr EditorChunkFlare::reprModel() const
{
	BW_GUARD;

	int renderLightProxy = OptionsLightProxies::visible();
	int renderFlareProxy = OptionsLightProxies::flareVisible();
	int renderLargeProxy = OptionsLightProxies::flareLargeVisible();

	if (renderLargeProxy && renderFlareProxy && renderLightProxy) 
	{
		return flareModel_;
	}
	else if (renderFlareProxy && renderLightProxy)
	{
		return flareModelSmall_;
	}

	return NULL;
}


void EditorChunkFlare::syncInit()
{
	// Grab the visibility bounding box
	#if UMBRA_ENABLE
	BW_GUARD;

	bw_safe_delete(pUmbraDrawItem_);

	if (currentModel_ == NULL)
	{
		EditorChunkSubstance<ChunkFlare>::syncInit();
		return;
	}	
	
	BoundingBox bb = BoundingBox::s_insideOut_;
	bb = currentModel_->boundingBox();
	
	// Set up object transforms
	Matrix m = pChunk_->transform();
	m.preMultiply( transform_ );

	// Create the umbra chunk item
	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem();
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell());
	pUmbraDrawItem_ = pUmbraChunkItem;

	this->updateUmbraLenders();
	#endif
}
/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkFlare, flare, 1 )


BW_END_NAMESPACE
// editor_chunk_flare.cpp
