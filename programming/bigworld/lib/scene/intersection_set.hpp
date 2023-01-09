#ifndef SCENE_INTERSECTION_SET_HPP
#define SCENE_INTERSECTION_SET_HPP

#include "cstdmf/lookup_table.hpp"
#include "scene_type_system.hpp"

namespace BW
{
	class IntersectionSet
	{
	public:
		typedef BW::vector< SceneObject > SceneObjectList;
		typedef SceneTypeSystem::RuntimeTypeID SceneObjectType;

	public:
		IntersectionSet();
		IntersectionSet( SceneObjectType typeMax );
		~IntersectionSet();

		void insert( const SceneObject& obj );
		SceneObjectList& objectsOfType( SceneObjectType typeID );
		const SceneObjectList& objectsOfType( SceneObjectType typeID ) const;
		
		void clear();

		size_t numObjects() const;
		SceneObjectType maxType() const;

	private:
		LookUpTable< SceneObjectList > buckets_;
	};
}


#endif // SCENE_INTERSECTION_SET_HPP
