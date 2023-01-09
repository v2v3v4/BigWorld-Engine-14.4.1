#include "pch.hpp"

#include "py_physics2.hpp"

#include "closest_triangle.hpp"
#include "closest_chunk_item.hpp"

// This file contains code that uses both the pyscript and physics2 libraries
// and is common to many processes. This is not big enough to be its own
// library.

// BW Tech Headers
#include "physics2/material_kinds.hpp"

#include "pyscript/py_data_section.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

int PyPhysics2_token = 0;


namespace // anonymous
{

/*~ function BigWorld.getMaterialKinds
 *	@components{ client, cell }
 *
 *	This method returns a list of id and section tuples of the
 *	material kind list, in the order in which they appear in the XML file
 *
 *	@return		The list of (id, DataSection) tuples of material kinds
 */
/**
 *	This method returns a list of id and section tuples of material kinds.
 */
PyObject* getMaterialKinds()
{
	const MaterialKinds::Map& kinds = MaterialKinds::instance().materialKinds();

	Py_ssize_t size = kinds.size();
	PyObject* pList = PyList_New( size );

	MaterialKinds::Map::const_iterator iter = kinds.begin();
	int i = 0;

	while (iter != kinds.end())
	{
		PyObject * pTuple = PyTuple_New( 2 );

		PyTuple_SetItem( pTuple, 0, PyInt_FromLong( iter->first ) );
		PyTuple_SetItem( pTuple, 1, new PyDataSection( iter->second ) );

		PyList_SetItem( pList, i, pTuple );

		i++;
		++iter;
	}

	return pList;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getMaterialKinds, END, BigWorld )


/*~ function BigWorld.collide
 *	@components{ client, cell, bots }
 *
 *	This function performs a collision test along a line segment from one
 *	point to another through the collision scene. If the line intersects nothing
 *	then the function returns None. Otherwise, it returns a tuple
 *	consisting of the point the first collision occurred at, the triangle that
 *	was hit (specified by its three points, as Vector3s), and the material
 *	type of the poly that was hit (an integer between 0 and 255).  This
 *	material type corresponds to the material or object Kind specified in
 *	ModelViewer (see Object->Properties and Material->Material Properties panes).
 *
 *	For example:
 *	@{
 *		>>> BigWorld.collide( spaceID, (0, 10, 0), (0,-10,0) )
 *		((0.0000, 0.0000, 0.0000), ( (0.0000, 0.0000, 0.0000),
 *		(4.0000, 0.0000, 0.0000), (4.0000, 0.0000, 4.0000)), 0)
 *	@}
 *	In this example the line goes from 10 units above the origin to 10 units
 *	below it. It intersects with a triangle at the origin, where the triangles
 *	three points are (0,0,0), (4,0,0) and (4,0,4). The material is of type 0.
 *
 *	@param spaceID			The space in which to perform the collision.
 *	@param src				The point to project from.
 *	@param dst				The point to project to.
 *	@param triangleFlags	Optional triangle filter. Only collisions with these
 *							triangles are tested, e.g TRIANGLE_WATER for water
 *							triangles, and TRIANGLE_TERRAIN for terrain
 *							triangles.
 *
 *	@return A 3-tuple of the hit point, the triangle it hit, and the material
 *			type or None if nothing was hit.
 */
/**
 *	Collides the given line with the collision scene
 *
 *	@return If detailedResults is true, a 3-tuple of the hit point, the triangle
 *			it hit, and the material type or None if nothing was hit.
 *			If detailedResults is false, the hit point is returned or None if
 *			nothing was hit.
 */
static PyObject * collide( SpaceID spaceID,
		const Vector3 & src, const Vector3 & dst,
		WorldTriangle::Flags triangleFlags = 0 )
{
	switch (triangleFlags)
	{
		case 0:
			return py_collide< ClosestTriangle >( spaceID, src, dst );

		case TRIANGLE_TERRAIN:
			return py_collide< ClosestTerrain >( spaceID, src, dst );

		case TRIANGLE_WATER:
			return py_collide< ClosestWater >( spaceID, src, dst );

		default:
			PyErr_Format( PyExc_ValueError,
					"invalid triangle flags '%u'",
					triangleFlags );
			return NULL;
	}
}
PY_AUTO_MODULE_FUNCTION( RETOWN, collide, ARG( SpaceID,
	ARG( Vector3, ARG( Vector3, OPTARG( WorldTriangle::Flags, 0, END ) ) ) ), BigWorld )


} // namespace (anonymous)

BW_END_NAMESPACE

// py_physics2.cpp
