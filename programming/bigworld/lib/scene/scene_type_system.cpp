#include "pch.hpp"

#include "scene_type_system.hpp"
#include "cstdmf/lookup_table.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/lookup_table.hpp"
#include "cstdmf/bw_hash.hpp"

namespace BW
{
namespace SceneTypeSystem
{

namespace
{
	SimpleMutex s_typeIDGenerationMutex;
}

LookUpTable< const char * > s_typeNameLookup;
TypeIDContext s_sceneGlobalTypeContext;
TypeIDContext s_viewTypeContext;
TypeIDContext s_objectTypeContext;
TypeIDContext s_objectOperationTypeContext;

GlobalUniqueTypeID generateSceneTypeID( const char * typeName )
{
	GlobalUniqueTypeID hashId = 
		static_cast<GlobalUniqueTypeID>( hash_string( typeName ) );
	
	// NOTE: This extra layer of typeID's is setup so that
	// debug tools can automatically look up the typename based on the
	// contextual ID's using lookup tables. Type ID generation is thread-safe.
	TypeIDContext::TypeLocalUniqueID sceneId = 
		s_sceneGlobalTypeContext.getLocalID( hashId );

	SimpleMutexHolder smh( s_typeIDGenerationMutex );
	s_typeNameLookup[sceneId] = typeName;

	return static_cast<GlobalUniqueTypeID>(sceneId);
}

const char * fetchTypeName( GlobalUniqueTypeID id )
{
	SimpleMutexHolder smh( s_typeIDGenerationMutex );
	return s_typeNameLookup[ static_cast<size_t>(id) ];
}

TypeIDContext& sceneViewTypeIDContext()
{
	return s_viewTypeContext;
}

TypeIDContext& sceneObjectTypeIDContext()
{
	return s_objectTypeContext;
}

TypeIDContext& sceneObjectOperationTypeIDContext()
{
	return s_objectOperationTypeContext;
}

TypeIDContext::TypeLocalUniqueID TypeIDContext::getLocalID( 
	TypeGlobalUniqueID globalUniqueID )
{
	SimpleMutexHolder smh( s_typeIDGenerationMutex );

	InternalMapping::iterator findResult = 
		internalMapping_.find( globalUniqueID );
	if (findResult != internalMapping_.end())
	{
		return findResult->second;
	}

	// We need to create a new ID then
	externalMapping_.push_back( globalUniqueID );
	TypeLocalUniqueID result = 
		static_cast<TypeLocalUniqueID>(externalMapping_.size());
	internalMapping_[ globalUniqueID ] = result;

	// If the result is 0 then we've overrun the storage available in 
	// the uniqueID type. Increase its size.
	MF_ASSERT( result != UNKNOWN );

	return result;
}

TypeIDContext::TypeGlobalUniqueID TypeIDContext::getGlobalID( 
	TypeLocalUniqueID localUniqueID )
{
	SimpleMutexHolder smh( s_typeIDGenerationMutex );

	if (static_cast<size_t>(localUniqueID - 1) >= externalMapping_.size())
	{
		return UNKNOWN;
	}
	else
	{
		return externalMapping_[localUniqueID - 1];
	}
}

} // namespace SceneTypeSystem
} // namespace BW

