#include "pch.hpp"

#include "scene/scene_type_system.hpp"

namespace BW
{

struct TypeA {};
struct TypeB {};
struct TypeC {};

TEST( Scene_TypeSystem_GUTID )
{
	// Test the GUTID system
	SceneTypeSystem::GlobalUniqueTypeID idA = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeA>();
	SceneTypeSystem::GlobalUniqueTypeID idB = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeB>();
	SceneTypeSystem::GlobalUniqueTypeID idC = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeC>();

	// Make sure all the ID's are different
	CHECK( idA != idB );
	CHECK( idA != idC );
	CHECK( idB != idC );

	// Make sure multiple calls to the same types are the same
	SceneTypeSystem::GlobalUniqueTypeID idA2 = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeA>();
	SceneTypeSystem::GlobalUniqueTypeID idB2 = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeB>();
	SceneTypeSystem::GlobalUniqueTypeID idC2 = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeC>();

	CHECK_EQUAL( idA, idA2 );
	CHECK_EQUAL( idB, idB2 );  
	CHECK_EQUAL( idC, idC2 );
}

TEST( Scene_TypeSystem_Context )
{
	SceneTypeSystem::GlobalUniqueTypeID idA = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeA>();
	SceneTypeSystem::GlobalUniqueTypeID idB = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeB>();
	SceneTypeSystem::GlobalUniqueTypeID idC = 
		SceneTypeSystem::getGloballyUniqueTypeID<TypeC>();

	// Test the contextual ID system
	SceneTypeSystem::TypeIDContext context;
	SceneTypeSystem::RuntimeTypeID localA = context.getLocalID( idA );
	SceneTypeSystem::RuntimeTypeID localC = context.getLocalID( idC );
	SceneTypeSystem::RuntimeTypeID localB = context.getLocalID( idB );

	// Make sure the values retreived are in order and consecutive.
	CHECK_EQUAL( 1, localA );
	CHECK_EQUAL( 2, localC );
	CHECK_EQUAL( 3, localB );
	
	// Test multiple calls 
	SceneTypeSystem::RuntimeTypeID localA2 = context.getLocalID( idA );
	SceneTypeSystem::RuntimeTypeID localC2 = context.getLocalID( idC );
	SceneTypeSystem::RuntimeTypeID localB2 = context.getLocalID( idB );
	
	CHECK_EQUAL( localA, localA2 );
	CHECK_EQUAL( localC, localC2 );
	CHECK_EQUAL( localB, localB2 );
}

} // namespace BW
