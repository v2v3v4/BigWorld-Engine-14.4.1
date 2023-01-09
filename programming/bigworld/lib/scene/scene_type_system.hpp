#ifndef SCENE_TYPE_SYSTEM_HPP
#define SCENE_TYPE_SYSTEM_HPP

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/stdmf.hpp"

#include "forward_declarations.hpp"
#include "scene_dll.hpp"

namespace BW
{
namespace SceneTypeSystem
{

typedef uint8 RuntimeTypeID;
typedef uint64 GlobalUniqueTypeID;
class TypeIDContext;

GlobalUniqueTypeID SCENE_API generateSceneTypeID( const char * typeName );
const char * SCENE_API fetchTypeName( GlobalUniqueTypeID id );

TypeIDContext& SCENE_API sceneViewTypeIDContext();
TypeIDContext& SCENE_API sceneObjectTypeIDContext();
TypeIDContext& SCENE_API sceneObjectOperationTypeIDContext();

/**
 * This class manages some local type ID's and remaps them into a local 
 * set of contiguous ID's that can be stored in lookup tables
 */
class TypeIDContext
{
public:
	typedef uint64 TypeGlobalUniqueID;
	typedef uint8 TypeLocalUniqueID;
	
	static const TypeLocalUniqueID UNKNOWN = 0;

	TypeLocalUniqueID getLocalID( TypeGlobalUniqueID globalUniqueID );	
	TypeGlobalUniqueID getGlobalID( TypeLocalUniqueID localUniqueID );
		
private:	

	typedef BW::vector< TypeGlobalUniqueID > ExternalMapping;
	typedef BW::map< TypeGlobalUniqueID, TypeLocalUniqueID> InternalMapping;
	
	InternalMapping internalMapping_;
	ExternalMapping externalMapping_;
};

template <typename T>
struct GloballyUniqueTypeID
{
	static GlobalUniqueTypeID getID()
	{
		// TODO: This is not a good way to support type ID's
		// This code is intended to be replaced by a reflection system
		// in the near future.
		T * tempTypeVar = NULL;
		const char * name = typeid( tempTypeVar ).name();
		static GlobalUniqueTypeID id = generateSceneTypeID( name );
		return id;
	}
};

template <typename T>
struct GloballyUniqueTypeID<T const>
{
	static GlobalUniqueTypeID getID()
	{
		return GloballyUniqueTypeID<T>::getID();
	}
};

template <typename T>
GlobalUniqueTypeID getGloballyUniqueTypeID()
{
	return GloballyUniqueTypeID<T>::getID();
}

template <typename Type>
RuntimeTypeID generateSceneViewRuntimeID()
{
	GlobalUniqueTypeID typeID = getGloballyUniqueTypeID<Type>();
	return sceneViewTypeIDContext().getLocalID( typeID );
}

template <typename Type>
RuntimeTypeID generateObjectRuntimeID()
{
	GlobalUniqueTypeID typeID = getGloballyUniqueTypeID<Type>();
	return sceneObjectTypeIDContext().getLocalID( typeID );
}

template <typename Type>
RuntimeTypeID generateObjectOperationRuntimeID()
{
	GlobalUniqueTypeID typeID = getGloballyUniqueTypeID<Type>();
	return sceneObjectOperationTypeIDContext().getLocalID( typeID );
}

template <typename Type>
RuntimeTypeID getSceneViewRuntimeID()
{
	static RuntimeTypeID cachedID = generateSceneViewRuntimeID<Type>();
	return cachedID;
}

template <typename Type>
RuntimeTypeID getObjectRuntimeID()
{
	static RuntimeTypeID cachedID = generateObjectRuntimeID<Type>();
	return cachedID;
}

template <typename Type>
RuntimeTypeID getObjectOperationRuntimeID()
{
	static RuntimeTypeID cachedID = generateObjectOperationRuntimeID<Type>();
	return cachedID;
}

} // namespace SceneTypeSystem
} // namespace BW

#endif
