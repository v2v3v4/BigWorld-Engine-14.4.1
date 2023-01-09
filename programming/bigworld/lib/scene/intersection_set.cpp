#include "pch.hpp"

#include "intersection_set.hpp"
#include "scene_object.hpp"


namespace BW
{

IntersectionSet::IntersectionSet()
{

}


IntersectionSet::IntersectionSet( SceneObjectType typeMax ) :
	buckets_( typeMax )
{

}


IntersectionSet::~IntersectionSet()
{

}

void IntersectionSet::insert( const SceneObject& obj )
{
	buckets_[obj.type()].push_back( obj );
}

IntersectionSet::SceneObjectList&
	IntersectionSet::objectsOfType( SceneObjectType typeID )
{
	return buckets_[typeID];
}

const IntersectionSet::SceneObjectList&
	IntersectionSet::objectsOfType( SceneObjectType  typeID ) const
{
	return buckets_[typeID];
}


void IntersectionSet::clear()
{
	for(SceneObjectType i = 0; i < buckets_.size(); ++i)
	{
		buckets_[i].resize( 0 );
	}
}

size_t IntersectionSet::numObjects() const
{
	size_t result = 0;
	for(SceneObjectType i = 0; i < buckets_.size(); ++i)
	{
		result += buckets_[i].size();
	}
	return result;
}

IntersectionSet::SceneObjectType IntersectionSet::maxType() const
{
	return static_cast<IntersectionSet::SceneObjectType>(buckets_.size());
}

} // namespace BW
