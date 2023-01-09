#include "pch.hpp"

#include "scene_object_flags.hpp"

namespace BW
{

SceneObjectFlags::SceneObjectFlags() :
	flags_( 0 )
{

}

SceneObjectFlags::SceneObjectFlags( const SceneObjectFlags& other ) :
	flags_( other.flags_ ) 
{

}

SceneObjectFlags::SceneObjectFlags( StorageType flags ) :
	flags_( flags )
{
}

bool SceneObjectFlags::isDynamic() const
{
	return (flags_ & IS_DYNAMIC_OBJECT) != 0;
}

void SceneObjectFlags::isDynamic( bool value )
{
	flags_ &= ~IS_DYNAMIC_OBJECT;
	flags_ |= (value) ? IS_DYNAMIC_OBJECT : 0;
}

bool SceneObjectFlags::isReflectionVisible() const
{
	return (flags_ & IS_REFLECTION_VISIBLE) != 0;
}

void SceneObjectFlags::isReflectionVisible( bool value )
{
	flags_ &= ~IS_REFLECTION_VISIBLE;
	flags_ |= (value) ? IS_REFLECTION_VISIBLE : 0;
}

bool SceneObjectFlags::isCastingStaticShadow() const
{
	return (flags_ & IS_CASTING_STATIC_SHADOW) != 0;
}

void SceneObjectFlags::isCastingStaticShadow( bool value )
{
	flags_ &= ~IS_CASTING_STATIC_SHADOW;
	flags_ |= (value) ? IS_CASTING_STATIC_SHADOW : 0;
}

bool SceneObjectFlags::isCastingDynamicShadow() const
{
	return (flags_ & IS_CASTING_DYNAMIC_SHADOW) != 0;
}

void SceneObjectFlags::isCastingDynamicShadow( bool value )
{
	flags_ &= ~IS_CASTING_DYNAMIC_SHADOW;
	flags_ |= (value) ? IS_CASTING_DYNAMIC_SHADOW : 0;
}

bool SceneObjectFlags::isTerrain() const
{
	return (flags_ & IS_TERRAIN_OBJECT) != 0;
}

void SceneObjectFlags::isTerrain( bool value )
{
	flags_ &= ~IS_TERRAIN_OBJECT;
	flags_ |= (value) ? IS_TERRAIN_OBJECT : 0;
}

}
