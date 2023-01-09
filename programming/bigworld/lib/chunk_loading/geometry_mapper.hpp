#ifndef GEOMETRY_MAPPER_HPP
#define GEOMETRY_MAPPER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

#include "math/vector3.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is an interface for users of GeometryMappings.
 */
class GeometryMapper
{
public:
	/**
	 *	This method fills in the initial point for loading the geometry mapping.
	 *	The initial point is specified in space coordinates.
	 *
	 *	@arg	result Set to the initial point.
	 *
	 *	@return	True if able to fill the origin, false otherwise.
	 */
	virtual bool initialPoint( Vector3 & result ) const { return false; }

	/**
	*	This method is called when a GeometryMapping has been fully loaded.
	*/
	virtual void onSpaceGeometryLoaded(
			SpaceID spaceID, const BW::string & mappingName ) = 0;

	virtual ~GeometryMapper() {};
};

BW_END_NAMESPACE

#endif // GEOMETRY_MAPPER_HPP

// geometry_mapper.hpp
