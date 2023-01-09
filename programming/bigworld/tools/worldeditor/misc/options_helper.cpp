
#include "pch.hpp"
#include "options_helper.hpp"
#include "appmgr/options.hpp"

BW_BEGIN_NAMESPACE

#define OPTIONS_IMPLEMENT_VISIBLE( CLASS )	\
	/*static*/ bool CLASS::s_visible_ = false;	\
	/*static*/ bool CLASS::visible()			\
	{											\
		OptionsHelper::check();					\
		return s_visible_;						\
	}


#define OPTIONS_IMPLEMENT_VISIBLE_PREFIX( CLASS, PREFIX )	\
	/*static*/ bool CLASS::s_##PREFIX##Visible_ = false;	\
	/*static*/ bool CLASS::PREFIX##Visible()		\
	{												\
		OptionsHelper::check();						\
		return s_##PREFIX##Visible_;				\
	}


#define OPTIONS_IMPLEMENT_INT( CLASS, NAME )	\
	/*static*/ int CLASS::s_##NAME##_ = 0;		\
	/*static*/ int CLASS::##NAME##()			\
	{											\
		OptionsHelper::check();					\
		return s_##NAME##_;						\
	}


#define OPTIONS_IMPLEMENT_FLOAT( CLASS, NAME )	\
	/*static*/ float CLASS::s_##NAME##_ = 0;		\
	/*static*/ float CLASS::##NAME##()			\
	{											\
		OptionsHelper::check();					\
		return s_##NAME##_;						\
	}


#define OPTIONS_IMPLEMENT_STRING( CLASS, NAME )	\
	/*static*/ BW::string CLASS::s_##NAME##_;		\
	/*static*/ const BW::string & CLASS::##NAME##()			\
	{											\
		OptionsHelper::check();					\
		return s_##NAME##_;						\
	}

#define OPTIONS_IMPLEMENT_VECTOR3( CLASS, NAME )	\
	/*static*/ Vector3 CLASS::s_##NAME##_( 0.0f, 0.0f, 0.0f );		\
	/*static*/ const Vector3 & CLASS::##NAME##()			\
	{											\
		OptionsHelper::check();					\
		return s_##NAME##_;						\
	}


// -----------------------------------------------------------------------------
// Section: OptionsHelper
// -----------------------------------------------------------------------------

/*static*/ bool OptionsHelper::s_inited_ = false;


/*static*/ void OptionsHelper::tick()
{
	BW_GUARD;

	// Tick all helpers here.
	OptionsGameObjects::tick();
	OptionsEditorProxies::tick();
	OptionsLightProxies::tick();
	OptionsParticleProxies::tick();
	OptionsMisc::tick();
	OptionsTerrain::tick();
	OptionsScenery::tick();
	OptionsSnaps::tick();
}


/*static*/ void OptionsHelper::check()
{
	if (!s_inited_)
	{
		BW_GUARD;

		s_inited_ = true;
		OptionsHelper::tick();
	}
}


// -----------------------------------------------------------------------------
// Section: OptionsGameObjects
// -----------------------------------------------------------------------------

/*static*/ void OptionsGameObjects::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/gameObjects", 1 );
	s_entitiesVisible_ = s_visible_ && !!Options::getOptionInt( "render/gameObjects/drawEntities", 1 );
	s_udosVisible_ = s_visible_ && !!Options::getOptionInt( "render/gameObjects/drawUserDataObjects", 1 );
	s_metaDataVisible_ = s_visible_ &&
		Options::getOptionInt( "render/gameObjects/drawChunkMetaData" );
}

OPTIONS_IMPLEMENT_VISIBLE( OptionsGameObjects )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsGameObjects, entities )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsGameObjects, udos )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsGameObjects, metaData )


// -----------------------------------------------------------------------------
// Section: OptionsEditorProxies
// -----------------------------------------------------------------------------

/*static*/ void OptionsEditorProxies::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/proxys", 1 ) &&
				!!Options::getOptionInt( "render/misc/drawEditorProxies", 0 );
}


OPTIONS_IMPLEMENT_VISIBLE( OptionsEditorProxies )



// -----------------------------------------------------------------------------
// Section: OptionsLightProxies
// -----------------------------------------------------------------------------

/*static*/ void OptionsLightProxies::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/proxys", 1 ) &&
				!!Options::getOptionInt( "render/proxys/lightProxys", 1 );
	s_dynamicVisible_ = !!Options::getOptionInt( "render/proxys/dynamicLightProxys", 1 );
	s_dynamicLargeVisible_ = !!Options::getOptionInt( "render/proxys/dynamicLightProxyLarge", 1 );
	s_ambientVisible_ = !!Options::getOptionInt( "render/proxys/ambientLightProxys", 1 );
	s_ambientLargeVisible_ = !!Options::getOptionInt( "render/proxys/ambientLightProxyLarge", 1 );
	s_pulseVisible_ = !!Options::getOptionInt( "render/proxys/pulseLightProxys", 1 );
	s_pulseLargeVisible_ = !!Options::getOptionInt( "render/proxys/pulseLightProxyLarge", 1 );
	s_flareVisible_ = !!Options::getOptionInt( "render/proxys/flareProxys", 1 );
	s_flareLargeVisible_ = !!Options::getOptionInt( "render/proxys/flareProxyLarge", 1 );
	s_spotVisible_ = !!Options::getOptionInt( "render/proxys/spotLightProxys", 1 );
	s_spotLargeVisible_ = !!Options::getOptionInt( "render/proxys/spotLightProxyLarge", 1 );
}


OPTIONS_IMPLEMENT_VISIBLE( OptionsLightProxies )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, dynamic )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, dynamicLarge )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, ambient )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, ambientLarge )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, pulse )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, pulseLarge )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, flare )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, flareLarge )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, spot )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsLightProxies, spotLarge )


// -----------------------------------------------------------------------------
// Section: OptionsEditorProxies
// -----------------------------------------------------------------------------

/*static*/ void OptionsParticleProxies::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/proxys", 1 ) &&
				!!Options::getOptionInt( "render/proxys/particleProxys", 1 );
	s_particlesLargeVisible_ = !!Options::getOptionInt( "render/proxys/particleProxyLarge", 1 );
}


OPTIONS_IMPLEMENT_VISIBLE( OptionsParticleProxies )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsParticleProxies, particlesLarge )


// -----------------------------------------------------------------------------
// Section: OptionsMisc
// -----------------------------------------------------------------------------

/*static*/ void OptionsMisc::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/misc", 0 );
	s_readOnlyVisible_ = !!Options::getOptionInt( "render/misc/shadeReadOnlyAreas", 1 );
	s_frozenVisible_ = !!Options::getOptionInt( "render/misc/shadeFrozenObjects", 1 );
	s_lighting_ = Options::getOptionInt( "render/lighting", 0 );
}


OPTIONS_IMPLEMENT_VISIBLE( OptionsMisc )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsMisc, readOnly )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsMisc, frozen )

OPTIONS_IMPLEMENT_INT( OptionsMisc, lighting )


// -----------------------------------------------------------------------------
// Section: OptionsTerrain
// -----------------------------------------------------------------------------

/*static*/ void OptionsTerrain::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/terrain", 1 );
	s_numLayersWarning_ = Options::getOptionInt( "chunkTexture/numLayerWarning", 0 );
	s_numLayersWarningVisible_ = !!Options::getOptionInt( "chunkTexture/numLayerWarningShow", 0 );
	s_terrainOverlayMode_ = Options::getOptionInt( "render/terrain/terrainOverlayMode", 0 );
	s_lodLocksOverlayVisible_ = Options::getOptionInt( "render/terrain/LODLocks", 0 ) == 1;
}


void OptionsTerrain::terrainOverlayMode(uint mode)
{
	s_terrainOverlayMode_ = mode;
	Options::setOptionInt("render/terrain/terrainOverlayMode", mode);
}


OPTIONS_IMPLEMENT_VISIBLE( OptionsTerrain )

OPTIONS_IMPLEMENT_INT( OptionsTerrain, numLayersWarning )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsTerrain, numLayersWarning )

OPTIONS_IMPLEMENT_INT( OptionsTerrain, terrainOverlayMode )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsTerrain, lodLocksOverlay )


// -----------------------------------------------------------------------------
// Section: OptionsScenery
// -----------------------------------------------------------------------------

/*static*/ void OptionsScenery::tick()
{
	BW_GUARD;

	s_visible_ = !!Options::getOptionInt( "render/scenery", 1 );
	s_shellsVisible_ = s_visible_ && !!Options::getOptionInt( "render/scenery/shells", 1 );
	s_waterVisible_ = s_visible_ && !!Options::getOptionInt( "render/scenery/drawWater", 1 );
	s_particlesVisible_ = s_visible_ && !!Options::getOptionInt( "render/scenery/particle", 1 );
	s_flaresVisible_ = s_visible_ &&
		(Options::getOptionInt( "render/scenery/drawChunkFlares") != 0);
	s_lightsVisible_ = s_visible_ &&
		(Options::getOptionInt( "render/scenery/drawChunkLights") != 0);
}


OPTIONS_IMPLEMENT_VISIBLE( OptionsScenery )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsScenery, shells )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsScenery, water )

OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsScenery, particles )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsScenery, flares )
OPTIONS_IMPLEMENT_VISIBLE_PREFIX( OptionsScenery, lights )


// -----------------------------------------------------------------------------
// Section: OptionsSnaps
// -----------------------------------------------------------------------------

/*static*/ void OptionsSnaps::tick()
{
	BW_GUARD;

	s_snapsEnabled_ = Options::getOptionInt( "snaps/xyzEnabled", 0 );
	s_placementMode_ = Options::getOptionInt( "snaps/itemSnapMode", 0 );
	s_coordMode_ = Options::getOptionString( "tools/coordFilter", "World" );
	s_movementSnaps_ = Options::getOptionVector3( "snaps/movement", Vector3( 0.f, 0.f, 0.f ) );
	s_angleSnaps_ = Options::getOptionFloat( "snaps/angle", 0.f );
}


OPTIONS_IMPLEMENT_INT( OptionsSnaps, snapsEnabled )
OPTIONS_IMPLEMENT_INT( OptionsSnaps, placementMode )
OPTIONS_IMPLEMENT_STRING( OptionsSnaps, coordMode )
OPTIONS_IMPLEMENT_VECTOR3( OptionsSnaps, movementSnaps )
OPTIONS_IMPLEMENT_FLOAT( OptionsSnaps, angleSnaps )

BW_END_NAMESPACE

