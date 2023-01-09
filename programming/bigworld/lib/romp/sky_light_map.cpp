#include "pch.hpp"
#include "sky_light_map.hpp"
#include "time_of_day.hpp"
#include "texture_feeds.hpp"
#include "cstdmf/debug.hpp"
#include "moo/effect_constant_value.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/draw_context.hpp"
#include "moo/managed_effect.hpp"
#include "moo/geometrics.hpp"
#include "resmgr/auto_config.hpp"
#include "enviro_minder.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

static float s_lightU = 1.6f;
static float s_lightV = 1.6f;

// the light map covers the viewable area, all the way to the far plane.
// addendum - set to 2000 because of constant in sky.cpp.  The clouds
// always extend to 2000 metres
const float CLOUD_EXTENT = 2000.f;
const float FADE_LOWER = 0.1f;
const float FADE_UPPER = 0.2f;

static AutoConfigString s_data( "environment/skyXML" );
static AutoConfigString s_nullBmp( "system/nullBmp" );
static const BW::string c_fxSettingKeyword = "skyLightmap";

struct SkyLightMapStatics
{
	SkyLightMapStatics()
#ifdef EDITOR_ENABLED
		: g_disableSkyLightMap( false )
#endif
	{
		REGISTER_SINGLETON( SkyLightMapStatics )
	}
#ifdef EDITOR_ENABLED
	bool g_disableSkyLightMap;
#endif

	static SkyLightMapStatics & instance();
};

static SkyLightMapStatics s_SkyLightMapStatics;

SkyLightMapStatics & SkyLightMapStatics::instance()
{
	SINGLETON_MANAGER_WRAPPER( SkyLightMapStatics )
	return s_SkyLightMapStatics;
}

#ifdef EDITOR_ENABLED
bool & getDisableSkyLightMapFlag()
{
	return SkyLightMapStatics::instance().g_disableSkyLightMap;
}
#endif

static float s_cloudMapDarkness = -0.65f;


/**
 *	This class exposes the SkyLightMap to the effect file engine.
 *	When a model is being rendered indoors, a null texture is
 *	used so that there is no cloud shadowing effect.
 */
class SkyLightMapSetter : public Moo::EffectConstantValue
{
public:
	SkyLightMapSetter( const BW::string& textureFeedName ) :
	  active_( true )
	{
		value_ = TextureFeeds::get( textureFeedName );
		null_ = Moo::TextureManager::instance()->get( s_nullBmp );
	}

	~SkyLightMapSetter()
	{
		value_ = NULL;
	}
	void reset()
	{ 
		value_ = NULL;
	}
	void active( bool newValue )
	{
		active_ = newValue;
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
#ifdef EDITOR_ENABLED
		if (getDisableSkyLightMapFlag())
		{
			pEffect->SetTexture(constantHandle, null_->pTexture());
			return true;
		}
#endif
		if (Moo::rc().effectVisualContext().isOutside() && active_ && value_)
			pEffect->SetTexture(constantHandle, value_->pTexture());
		else
			pEffect->SetTexture(constantHandle, null_->pTexture());
		return true;
	}

private:
	Moo::BaseTexturePtr value_;
	Moo::BaseTexturePtr null_;
	bool active_;
};


/**
 *	This class exposes a 2x4 matrix to the effect file engine, which
 *	transforms from world position to texture coordinate space.
 */
class SkyLightMapTransformSetter : public Moo::EffectConstantValue
{
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{				
		pEffect->SetVectorArray(constantHandle, transform_, 2);
		return true;
	}

	void calculate()
	{
		float farPlane = 1.f / (2.f * Moo::rc().camera().farPlane());		
		transform_[0].set( farPlane, 0.f, 0.f, 0.5f - texCoordOffset_.x + sunAngleOffset_ );
		transform_[1].set( 0.f, 0.f, -farPlane, 0.5f + texCoordOffset_.y );
	}

public:
	SkyLightMapTransformSetter():
		texCoordOffset_(0.f,0.f),
		sunAngleOffset_(0.f)
	{
		this->calculate();
	}

	void texCoordOffset( const Vector2& offset )
	{
		texCoordOffset_ = offset;
		calculate();
	}

	void sunAngleOffset( float a )
	{
		sunAngleOffset_ = a;
		calculate();
	}

private:
	Vector4 transform_[2];
	Vector2 texCoordOffset_;
	float sunAngleOffset_;
};



/**
 *	Constructor.
 */
static IncrementalNameGenerator s_skyLightMapNames( "SkyLightMap" );

SkyLightMap::SkyLightMap():
	EffectLightMap( s_skyLightMapNames.nextName() ),
	inited_( false ),	
	needsUpdate_( true ),
	texCoordOffset_( 0.f, 0.f ),
	sunAngleOffset_()
{
	BW_GUARD;
	if (s_cloudMapDarkness < 0.f)
	{
		s_cloudMapDarkness = -s_cloudMapDarkness;
		MF_WATCH( "Client Settings/Clouds/max sky light map darkness",
			s_cloudMapDarkness,
			Watcher::WT_READ_WRITE,
			"Maximum sky light map darkness.  Sky light map values are clamped to "
			"this alpha value." );
	}
	this->createUnmanagedObjects();	
}


/**
 *	Destructor.
 */
SkyLightMap::~SkyLightMap()
{
	BW_GUARD;
}


void SkyLightMap::createUnmanagedObjects()
{
	BW_GUARD;
	//we will have to recreate the light map
	needsUpdate_ = true;
	LightMap::createUnmanagedObjects();
}


void SkyLightMap::deleteUnmanagedObjects()
{
	BW_GUARD;
	if (active_)
	{
		if (lightMapSetter_)
		{
			SkyLightMapSetter* setter =
				reinterpret_cast<SkyLightMapSetter*>( lightMapSetter_.getObject() );
			setter->reset();
		}
	}
	LightMap::deleteUnmanagedObjects();
}


/**
 *	This method is called by the sky when it is
 *	activated, i.e. when the camera enters this
 *	space.
 */
void SkyLightMap::activate()
{
	BW_GUARD;
	if ( !inited_ )
	{
		this->initInternal();		
	}

	//Note : there may be cases where there are two or more
	//sky light maps, so when activated we must expose our own
	//effect constant setters to effects.
	LightMap::activate();
}


/**
 *	This method is called by the sky when it is
 *	deactivated, i.e. when the camera leaves this
 *	space.
 */
void SkyLightMap::deactivate()
{
	BW_GUARD;
	LightMap::deactivate();
}


void SkyLightMap::initInternal()
{
	BW_GUARD;
	DataSectionPtr pSection = BWResource::openSection( s_data );
	DataSectionPtr pRelevant;
	if (pSection)
	{
		pRelevant = pSection->openSection("light_map");
		if (pRelevant)
		{
			inited_ = this->init(pRelevant);			
		}
		else
		{
			ASSET_MSG( "SkyLightMap::initInternal - no light_map section.\n" );			
		}
	}
	if ( !inited_ )
	{			
		ASSET_MSG( "SkyLightMap::initInternal - init failed\n" );
		return;
	}	
}


/**
 *	This method updates the sky light map, which either recalculates
 *	the map given the current cloud data, or shifts the texture transform
 *	by an amount equal to the wind speed.
 */
void SkyLightMap::update( float sunAngle, const Vector2& strataMovement )
{
	BW_GUARD;
	if (!inited_)
		return;

	Vector3 center = Moo::rc().invView().applyToOrigin();	

	// incorporate the sun angle into the texture offset	
	//note cloud strata height is in clouds texture map space	
	sunAngle -= MATH_PI;
	const float cloudStrataHeight = 100.f / Moo::rc().camera().farPlane();
	float xOffset = Math::clamp(-FADE_UPPER, cloudStrataHeight * tanf(sunAngle), FADE_UPPER);
	this->sunAngleOffset( xOffset );

	// set the new tex coord offset
	float x = texCoordOffset().x;
	float y = texCoordOffset().y;
	// NOTE : setting texCoordOffset here implicitly recalculates the light map transform
	// every frame.  Even if the texCoordOffset doesn't change, we may as well
	// calculate the transform per-frame just in case the far plane has changed too.  That
	// way we don't need to explicitly keep track of camera far plane changes.
	this->texCoordOffset(Vector2(x + strataMovement.x/CLOUD_EXTENT, y + strataMovement.y/CLOUD_EXTENT));

	// when the sun goes low in the sky, begin fading out the sky light map
	float absx = fabsf(xOffset);
	bool needsLightingUpdate = (absx > FADE_LOWER && absx <= FADE_UPPER);

	needsUpdate_ |= (y>-0.25f);	
	for (uint32 i=0; i<contributors_.size(); i++)
	{
		needsUpdate_ |= contributors_[i]->needsUpdate();
	}	

	if (absx > FADE_UPPER)
		needsUpdate_ = false;

	//early out		
	if (!needsUpdate_ && !needsLightingUpdate)
	{		
		return;
	}

	if (!material_.get() || !material_->pEffect() || !material_->pEffect()->pEffect())
		return;		

	if ( this->beginRender() )
	{
		if (SkyLightMapSettings::isEnabled())
		{
			//we are going to recreate the light map, so reset the tex coord offset thing
			this->texCoordOffset( Vector2(0.f,0.f) );
			for (uint32 i=0; i<contributors_.size(); i++)
			{
				contributors_[i]->render( this, material_, sunAngle);
			}
			needsUpdate_ = false;
			this->lighten(xOffset);
		}			
		this->endRender();
	}
}


void SkyLightMap::addContributor( IContributor& c )
{
	BW_GUARD;
	contributors_.push_back( &c );
}


void SkyLightMap::delContributor( IContributor& c )
{
	BW_GUARD;
	Contributors::iterator it = contributors_.begin();

	while (it != contributors_.end())
	{
		IContributor* b = *it;
		if (b != &c)
		{
			it++;
		}
		else
		{
			//remove all references to c
			contributors_.erase( it );
			it = contributors_.begin();
		}			
	}		
}


/**
 *	This private method is used internally by the SkyLightMap class.
 */
bool SkyLightMap::beginRender()
{
	BW_GUARD;
	uint32 nTextures = Moo::rc().deviceInfo( Moo::rc().deviceIndex() ).caps_.MaxSimultaneousTextures;
	for (uint32 i = 0; i < nTextures; i++)
	{
		Moo::rc().setTexture( i, NULL );
	}

	bool ret = true;
	if ( !pRT_ || !pRT_->push() )
	{
		ret = false;
	}
	else
	{
		Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x00000000, 1, 0 );
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA );
	}

	if ( lightMapSetter_ )
	{
		// Tell the light map setter about the state of the render target
		// It's safe to cast since we create it in 'createLightMapSetter'
		SkyLightMapSetter* setter =
			reinterpret_cast<SkyLightMapSetter*>( lightMapSetter_.getObject() );
		setter->active( ret );
	}

	return ret;
}


/**
 *	This private method is used internally by the SkyLightMap class.
 */
void SkyLightMap::endRender()
{
	BW_GUARD;
	pRT_->pop();	

	// restore colorwriteenable states.
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_RED | 
		D3DCOLORWRITEENABLE_BLUE | 
		D3DCOLORWRITEENABLE_GREEN );
}


/**
 *	This private method is used internally by the SkyLightMap class.
 *
 *	The xOffset variable is passed in from the calling function.  It
 *	represents how far across the texture map is shifted to account
 *	for the sun going down.  When the sun is low, we fade out the sky
 *	light map.
 */
void SkyLightMap::lighten(float xOffset)
{
	BW_GUARD;
	float maxDarkness = s_cloudMapDarkness / 2.f;
	if (fabsf(xOffset)>FADE_LOWER)
	{
		float fadeValue = (fabsf(xOffset) - FADE_LOWER) / (FADE_UPPER-FADE_LOWER);
		fadeValue = 1.f - Math::clamp(0.f,fadeValue,1.f);
		maxDarkness *= fadeValue;
	}
	//Clamp the values in the sky light map to a maximum.
	Moo::Colour colour(1.f-maxDarkness,1.f-maxDarkness,1.f-maxDarkness,maxDarkness);
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA );
	static CustomMesh<Moo::VertexTL> mesh( D3DPT_TRIANGLESTRIP );
	Geometrics::setVertexColourMod2(colour);
	Geometrics::createRectMesh(
		Vector2(0,0),
		Vector2( Moo::rc().screenWidth(), Moo::rc().screenHeight() ), 
		colour,
		mesh );
	mesh.draw();
	mesh.clear();
}


/**
 *	This private method is used internally by the SkyLightMap class.
 */
const Vector2& SkyLightMap::texCoordOffset() const
{
	return texCoordOffset_;
}


/**
 *	This method overrides EffectFile::init, and reads in the maximum darkness
 *	value for the light map.
 */
bool SkyLightMap::init(const DataSectionPtr pSection)
{
	BW_GUARD;
	if (pSection)
	{
		s_cloudMapDarkness = 
			pSection->readFloat( "maxDarkness", s_cloudMapDarkness );
		DEBUG_MSG( "cloud map darkness %0.2f\n", s_cloudMapDarkness );
	}

	bool result = EffectLightMap::init( pSection );	
	return result;
}


/**
 *	This method is overriden from the LightMap base class, and
 *	creates a SkyLightMapSetter instead of a plain LightMapSetter
 */
void SkyLightMap::createLightMapSetter( const BW::string& textureFeedName )
{
	BW_GUARD;
	lightMapSetter_ = new SkyLightMapSetter( textureFeedName );
}


/**
 *	This method implements the LightMap base class interface and
 *	creates a SkyLightMapTransformSetter.
 */
void SkyLightMap::createTransformSetter()
{
	BW_GUARD;
	transformSetter_ = new SkyLightMapTransformSetter;	
}


/**
 *	This private method is used internally by the SkyLightMap class.
 */
void SkyLightMap::texCoordOffset( const Vector2& value )
{
	BW_GUARD;
	texCoordOffset_ = value;

	Moo::EffectConstantValue* ecv = &*transformSetter_;
	SkyLightMapTransformSetter* slmts = static_cast<SkyLightMapTransformSetter*>(ecv);
	slmts->texCoordOffset( texCoordOffset_ );
}


/**
 *	This private method is used internally by the SkyLightMap class.
 */
void SkyLightMap::sunAngleOffset( float a )
{
	BW_GUARD;
	sunAngleOffset_ = a;

	Moo::EffectConstantValue* ecv = &*transformSetter_;
	SkyLightMapTransformSetter* slmts = static_cast<SkyLightMapTransformSetter*>(ecv);
	slmts->sunAngleOffset( sunAngleOffset_ );
}



//-----------------------------------------------------------------------------
//Section - Sky Light Map Graphics Settings
//-----------------------------------------------------------------------------
static SkyLightMapSettings s_slmSettings;


/**
 *	Adjusts the effect settings related to sky lightmapping. Tests 
 *	hardware capabilities and forces disable lightmapping is not supported. 
 *	Also, makes sure the setting is present, otherwise fx compilation will fail.
 */
void SkyLightMapSettings::configureKeywordSetting(Moo::EffectMacroSetting & setting)
{	
	BW_GUARD;
	REGISTER_SINGLETON_FUNC( SkyLightMapSettings, &getSettings );
	bool supported = (Moo::rc().psVersion() >= 0x200);		
	setting.addOption("ON", "On", supported, "1");
	setting.addOption("OFF", "Off", true, "0");
	Moo::EffectManager::instance().addListener( &s_slmSettings );
}


/**
 *	This method finalises the sky light map settings.  It should only be
 *	called once per application execution.
 */
void SkyLightMapSettings::fini()
{
	BW_GUARD;
	Moo::EffectManager::instance().delListener( &s_slmSettings );
	getSettings() = NULL;
}


Moo::EffectMacroSetting::EffectMacroSettingPtr SkyLightMapSettings::s_skyLightMapSetting;

void SkyLightMapSettings::onSelectPSVersionCap(int psVerCap)
{	
	BW_GUARD;
}


bool SkyLightMapSettings::isEnabled()
{
	BW_GUARD;
	return false;
	//return getSettings()->activeOption() == 0;
}

int SkyLightMapSetting_token = 0;

BW_END_NAMESPACE

// sky_light_map.cpp
