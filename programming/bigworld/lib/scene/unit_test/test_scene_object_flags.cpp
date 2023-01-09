#include "pch.hpp"
#include "test_polymesh.hpp"
#include "test_drawoperation.hpp"
#include "test_sceneview.hpp"
#include "test_staticsceneprovider.hpp"

#include "scene/forward_declarations.hpp"
#include "scene/scene_object_flags.hpp"

#include "cstdmf/guard.hpp"

namespace BW
{

TEST( Scene_Object_Flags_Construction )
{
	{
		SceneObjectFlags flags;
		CHECK_EQUAL( false, flags.isDynamic() );
		CHECK_EQUAL( false, flags.isReflectionVisible() );
		CHECK_EQUAL( false, flags.isCastingStaticShadow() );
		CHECK_EQUAL( false, flags.isCastingDynamicShadow() );
		CHECK_EQUAL( false, flags.isTerrain() );
	}
	
	{
		SceneObjectFlags flags( SceneObjectFlags::IS_DYNAMIC_OBJECT | 
			SceneObjectFlags::IS_CASTING_STATIC_SHADOW );
		CHECK_EQUAL( true, flags.isDynamic() );
		CHECK_EQUAL( false, flags.isReflectionVisible() );
		CHECK_EQUAL( true, flags.isCastingStaticShadow() );
		CHECK_EQUAL( false, flags.isCastingDynamicShadow() );
		CHECK_EQUAL( false, flags.isTerrain() );

		SceneObjectFlags copy( flags );
		CHECK_EQUAL( true, copy.isDynamic() );
		CHECK_EQUAL( false, copy.isReflectionVisible() );
		CHECK_EQUAL( true, copy.isCastingStaticShadow() );
		CHECK_EQUAL( false, copy.isCastingDynamicShadow() );
		CHECK_EQUAL( false, flags.isTerrain() );
	}
}

TEST( Scene_Object_Flags_GetSet )
{
	SceneObjectFlags flags;

	// Always Test twice to check setting/unsetting/resetting
	// isDynamic
	flags.isDynamic( false );
	CHECK_EQUAL( false, flags.isDynamic() );
	flags.isDynamic( true );
	CHECK_EQUAL( true, flags.isDynamic() );
	flags.isDynamic( false );
	CHECK_EQUAL( false, flags.isDynamic() );
	flags.isDynamic( true );
	CHECK_EQUAL( true, flags.isDynamic() );

	// isReflectionVisible
	flags.isReflectionVisible( false );
	CHECK_EQUAL( false, flags.isReflectionVisible() );
	flags.isReflectionVisible( true );
	CHECK_EQUAL( true, flags.isReflectionVisible() );
	flags.isReflectionVisible( false );
	CHECK_EQUAL( false, flags.isReflectionVisible() );
	flags.isReflectionVisible( true );
	CHECK_EQUAL( true, flags.isReflectionVisible() );
	
	// isCastingStaticShadow
	flags.isCastingStaticShadow( false );
	CHECK_EQUAL( false, flags.isCastingStaticShadow() );
	flags.isCastingStaticShadow( true );
	CHECK_EQUAL( true, flags.isCastingStaticShadow() );
	flags.isCastingStaticShadow( false );
	CHECK_EQUAL( false, flags.isCastingStaticShadow() );
	flags.isCastingStaticShadow( true );
	CHECK_EQUAL( true, flags.isCastingStaticShadow() );

	// isCastingDynamicShadow
	flags.isCastingDynamicShadow( false );
	CHECK_EQUAL( false, flags.isCastingDynamicShadow() );
	flags.isCastingDynamicShadow( true );
	CHECK_EQUAL( true, flags.isCastingDynamicShadow() );
	flags.isCastingDynamicShadow( false );
	CHECK_EQUAL( false, flags.isCastingDynamicShadow() );
	flags.isCastingDynamicShadow( true );
	CHECK_EQUAL( true, flags.isCastingDynamicShadow() );

	// isTerrain
	flags.isTerrain( false );
	CHECK_EQUAL( false, flags.isTerrain() );
	flags.isTerrain( true );
	CHECK_EQUAL( true, flags.isTerrain() );
	flags.isTerrain( false );
	CHECK_EQUAL( false, flags.isTerrain() );
	flags.isTerrain( true );
	CHECK_EQUAL( true, flags.isTerrain() );
}

} // namespace BW
