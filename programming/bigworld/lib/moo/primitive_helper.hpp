#ifndef PRIMITIVE_HELPER_HPP
#define PRIMITIVE_HELPER_HPP

#include "index_buffer.hpp"
#include "moo_dx.hpp"
#include "primitive_file_structs.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

namespace PrimitiveHelper
{

/**
 * This enum represents the primitive topology type of a buffer
 */
enum PrimitiveType {
	POINT_LIST,
	LINE_LIST,
	LINE_STRIP,
	TRIANGLE_LIST,
	TRIANGLE_STRIP,
	TRIANGLE_FAN
};

/**
 * This method converts a device specific enumeration of primitive type
 * to a version that primitive helper understands.
 *
 *	@param	value	The device specific primitive type enumeration.
 *
 *	@return	PrimitiveHelper::PrimitiveType that corresponds to the device 
 *		value.
 */
PrimitiveType primitiveTypeFromDeviceEnum( D3DPRIMITIVETYPE value );

/**
 * This method fills the provided index buffer iteration parameters based on 
 * the provided primitive type. NULL params will be ignored.
 *
 *	@param	primitiveType	The primitive type representation of the buffer data
 *
 *	@param	pIndexMultiplier		Output pointer. The base index mutiplier.
 *	@param	pIndexBasedOrdering		Output pointer.
 *									Either 0 or 1. 1 indicates flipping index 
 *									order on relevant triangles e.g. strips
 *	@param pTotalCountNegativeOffset	Output pointer. An offset from end
 *										used to calculate triangle counts.
 *										(see numTriangles).
 *	@param	complainOnUnsupportedType	If true, warn if unsupported type.
 *
 *	@return	False if unsupported primitive type.
 */
bool getTriangleIterationParams( PrimitiveType primitiveType,
	uint32 * pIndexMultiplier,
	uint32 * pIndexBasedOrdering,
	uint32 * pTotalCountNegativeOffset,
	bool complainOnUnsupportedType = true );

/**
 * This method validates that the triangle count does not go out of bounds
 * of the index buffer for the given primitive type
 * 
 *	@param	indexBufferSize	The total size (count) of the index buffer
 *	@param	startIndex		The starting index offset, 0 if none.
 *	@param	triangleCount	The total number of triangles
 *	@param	primitiveType	The primitive type representation of the buffer data
 *	@param	complainOnInvalidRange	If true, warn if invalid range.
 *
 *	@return	True if params respect the bounds of the index buffer.
 */
bool isValidIndexBufferRange( uint32 indexBufferSize, 
	uint32 startIndex, uint32 triangleCount, PrimitiveType primitiveType,
	bool complainOnInvalidRange = true);

/**
 * This method calculates the total number of triangles that will be
 * iterated for an index buffer and primitive type.
 * 
 *	@param	primitiveType	The primitive type representation of the buffer data
 *	@param	indexBufferSize	The total size (count) of the index buffer
 *	@param	startIndex		The starting index offset, 0 if none
 * 
 *	@return	The number of triangles that are represented by the indexBuffer. 
 */
uint32 numTriangles( PrimitiveType primitiveType, uint32 indexBufferSize, 
	uint32 startIndex );

/**
 * This class simply provides an indexing operator[](i) that, given an index X
 * will return the same index value X. This provides uniformity to 
 * generateTrianglesFromIndices() when we are not working with an index buffer.
 */
template < class RetType, class IndexType >
class IndexGenerator
{
public:
	RetType operator[]( IndexType index ) const
	{
		return static_cast< RetType >( index );
	}
};

// Default value for disabling validity checks
static const uint32 UNSPECIFIED_BUFFER_SIZE = uint32(-1);

/**
 * This function generically iterates over triangle primitives based on the 
 * primitive type in order to generate triplets of indexes which are then 
 * passed as params to a user provided "per triangle" function. 
 *
 * This allows writing functors that operate on indexed triangles without 
 * worrying about the details of the triangle's indexing format or 
 * primitive/topology type (e.g. triangle lists, triangle strips, etc)
 *
 *	@param	indices	The index data holder that stores the indices. This can 
 *			be any type that supports indexing
 *
 *	@param	startIndex		The index to start at.
 *	@param	triangleCount	The total number of triangles to iterate over.
 *	@param	primitiveType	The primitive type represented by the buffer data.
 *	@param	perTriangleFunc	A function or functor of the form: 
 *								RetType func(uint32 a, uint32 b, uint32 c)
 *							if value of RetType is non-true, stop iterating.
 *	@param indexBufferSize	Optional. The total number of items of the index 
 *							buffer (not byte size). If this is
 *							provided, will initially assert that potential
 *							accesses stay within the buffer's bounds.
 *		
 *	@return	The total number of triangles iterated
 */
template < class IndexCollection, class PerTriangleFunc >
static uint32 generateTrianglesFromIndices( const IndexCollection & indices, 
	uint32 startIndex,
	uint32 triangleCount,
	PrimitiveType primitiveType,
	PerTriangleFunc & perTriangleFunc,
	uint32 indexBufferSize = UNSPECIFIED_BUFFER_SIZE )
{
	// Validate params if applicable
	if (indexBufferSize != UNSPECIFIED_BUFFER_SIZE &&
		!isValidIndexBufferRange( indexBufferSize, startIndex, 
			triangleCount, primitiveType ))
	{
		return 0;
	}

	// Get iteration params
	uint32 indexMultiplier;
	uint32 indexBasedOrdering;

	if (!getTriangleIterationParams( primitiveType, 
			&indexMultiplier, &indexBasedOrdering, NULL ))
	{
		return 0;
	}

	// Iterate over all triangles 
	uint32 currentTri;
	for (currentTri = 0; currentTri < triangleCount; ++currentTri)
	{
		// Calculate the base index for this triangle
		uint32 baseIndexID = startIndex + (currentTri * indexMultiplier);

		// Get triangle indices based on winding order if sensible for type.
		// E.g. Every odd triangle in a strip is out of order.
		// 1 if it is odd, and index order is meaningful for this primitive type
		uint32 oneIfOddBaseIndex = (baseIndexID & 1) * indexBasedOrdering;

		uint32 index1 = indices[baseIndexID];
		uint32 index2 = indices[baseIndexID + 1 + oneIfOddBaseIndex];
		uint32 index3 = indices[baseIndexID + 2 - oneIfOddBaseIndex];

		// Call user provided functor with indices
		if (!perTriangleFunc( index1, index2, index3 ))
		{
			break;
		}
	}
	return currentTri;
}


/// Specialised convenience version for calls without an index buffer.
template < class PerTriangleFunc >
static uint32 generateTrianglesFromIndices(
	uint32 startIndex,
	uint32 triangleCount,
	PrimitiveType primitiveType,
	PerTriangleFunc & perTriangleFunc, 
	uint32 indexBufferSize = UNSPECIFIED_BUFFER_SIZE)
{
	// build a dummy indexer to call real function with
	typedef IndexGenerator< uint32, uint32 > DefaultIndexGenerator;
	DefaultIndexGenerator tempIndexer;

	return generateTrianglesFromIndices( 
		tempIndexer, 
		startIndex, 
		triangleCount, 
		primitiveType, 
		perTriangleFunc,
		indexBufferSize);
}


} // namespace PrimitiveHelper

} // namespace Moo


BW_END_NAMESPACE

#endif
// PRIMITIVE_HELPER_HPP
