#ifndef _OPTIONS_HELPER_HPP_
#define _OPTIONS_HELPER_HPP_


// These classes are used to avoid asking for the same options several times
// per frame, which was taking up to 6% of the total frame rate in some cases.
// The worst offenders are dealt with here, usually visibility queries.
// They also might encapsulate some visibility logic, for example, returning
// 'true' in 'entitiesVisible' if both entities and gameObjects are ticked.

BW_BEGIN_NAMESPACE

/**
 *	This class is used to keep all optimised options in sync.
 */
class OptionsHelper
{
public:
	static void tick();
	static void check();

private:
	static bool s_inited_;
};


// Helper macros
#define OPTIONS_DECLARE_VISIBLE()	\
	private:						\
		static bool s_visible_;		\
	public:							\
		static bool visible();


#define OPTIONS_DECLARE_VISIBLE_PREFIX( PREFIX )	\
	private:										\
		static bool s_##PREFIX##Visible_;			\
	public:											\
		static bool PREFIX##Visible();


#define	OPTIONS_DECLARE_INT( NAME )	\
	private:						\
		static int s_##NAME##_;	\
	public:							\
		static int NAME();


#define	OPTIONS_DECLARE_FLOAT( NAME )	\
	private:						\
		static float s_##NAME##_;	\
	public:							\
		static float NAME();


#define	OPTIONS_DECLARE_STRING( NAME )	\
	private:						\
	static BW::string s_##NAME##_;	\
	public:							\
		static const BW::string & NAME();


#define	OPTIONS_DECLARE_VECTOR3( NAME )	\
	private:						\
	static Vector3 s_##NAME##_;	\
	public:							\
		static const Vector3 & NAME();


/**
 *	This class is used optimise entity and UDO related options
 */
class OptionsGameObjects
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()
	OPTIONS_DECLARE_VISIBLE_PREFIX( entities )
	OPTIONS_DECLARE_VISIBLE_PREFIX( udos )
	OPTIONS_DECLARE_VISIBLE_PREFIX( metaData )
};


/**
 *	This class is used optimise editor proxies related options
 */
class OptionsEditorProxies
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()
};


/**
 *	This class is used optimise light proxies related options
 */
class OptionsLightProxies
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()

	OPTIONS_DECLARE_VISIBLE_PREFIX( dynamic )
	OPTIONS_DECLARE_VISIBLE_PREFIX( dynamicLarge )
	OPTIONS_DECLARE_VISIBLE_PREFIX( ambient )
	OPTIONS_DECLARE_VISIBLE_PREFIX( ambientLarge )
	OPTIONS_DECLARE_VISIBLE_PREFIX( pulse )
	OPTIONS_DECLARE_VISIBLE_PREFIX( pulseLarge )
	OPTIONS_DECLARE_VISIBLE_PREFIX( flare )
	OPTIONS_DECLARE_VISIBLE_PREFIX( flareLarge )
	OPTIONS_DECLARE_VISIBLE_PREFIX( spot )
	OPTIONS_DECLARE_VISIBLE_PREFIX( spotLarge )
};


/**
 *	This class is used optimise particle proxies related options
 */
class OptionsParticleProxies
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()

	OPTIONS_DECLARE_VISIBLE_PREFIX( particlesLarge )
};


/**
 *	This class is used optimise shading-related options
 */
class OptionsMisc
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()

	OPTIONS_DECLARE_VISIBLE_PREFIX( readOnly )
	OPTIONS_DECLARE_VISIBLE_PREFIX( frozen )
	OPTIONS_DECLARE_INT( lighting )
};


// possible values of terrainOverlayMode (TerrainOverlayController.py support)
enum
{
	TERRAIN_OVERLAY_MODE_DEFAULT = 0, // no overlay
	TERRAIN_OVERLAY_MODE_COLORIZE_GROUND_STRENGTH = 1, // visualize ground strength
	TERRAIN_OVERLAY_MODE_TEXTURE_OVERLAY = 2, // blend terrain with texture overlay
	TERRAIN_OVERLAY_MODE_GROUND_TYPES_MAP = 3 // blend terrain with ground types texture
};

/**
 *	This class is used optimise terrain related options
 */
class OptionsTerrain
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()

	OPTIONS_DECLARE_VISIBLE_PREFIX( numLayersWarning )
	OPTIONS_DECLARE_INT( numLayersWarning )
	OPTIONS_DECLARE_INT( terrainOverlayMode )

	static void terrainOverlayMode(uint mode);

	OPTIONS_DECLARE_VISIBLE_PREFIX( lodLocksOverlay )
};


/**
 *	This class is used optimise scenery related options
 */
class OptionsScenery
{
public:
	static void tick();

	OPTIONS_DECLARE_VISIBLE()

	OPTIONS_DECLARE_VISIBLE_PREFIX( shells )
	OPTIONS_DECLARE_VISIBLE_PREFIX( water )
	OPTIONS_DECLARE_VISIBLE_PREFIX( particles )
	OPTIONS_DECLARE_VISIBLE_PREFIX( flares )
	OPTIONS_DECLARE_VISIBLE_PREFIX( lights )
};


/**
 *	This class is used optimise scenery related options
 */
class OptionsSnaps
{
public:
	static void tick();

	OPTIONS_DECLARE_INT( snapsEnabled )
	OPTIONS_DECLARE_INT( placementMode )
	OPTIONS_DECLARE_STRING( coordMode )
	OPTIONS_DECLARE_VECTOR3( movementSnaps )
	OPTIONS_DECLARE_FLOAT( angleSnaps )
};

BW_END_NAMESPACE

#endif // _OPTIONS_HELPER_HPP_

