#ifndef PRIMITIVE_FILE_STRUCTS_HPP
#define PRIMITIVE_FILE_STRUCTS_HPP

#include "cstdmf/fourcc.hpp"

BW_BEGIN_NAMESPACE

namespace Moo 
{

/**
 *	Header structure for the vertex values stored in a file.
 */
struct VertexHeader
{
	char	vertexFormat_[ 64 ];
	int		nVertices_;
};

/**
 *	Header structure for processed vertex values stored in a file.
 */
struct ProcessedVertexHeader
{
	uint32	magic_;
	char	originalVertexFormat_[ 64 ];
	VertexHeader vertexHeader_;
};

/**
 * Constants for magic fourcc identifiers
 */
struct HeaderMagic
{
	enum Value
	{
		ProcessedVertices = DECLARE_FOURCC('B', 'P', 'V', 'T'),
		ProcessedVertexStream = DECLARE_FOURCC('B', 'P', 'V', 'S')
	};
};

/**
 *	Header structure for the index values stored in a file.
 */
struct IndexHeader
{
	char	indexFormat_[ 64 ];
	int		nIndices_;
	int		nTriangleGroups_;
};


/**
 *	The primitive group structure maps to a set of indices within the index
 *	buffer.
 */
struct PrimitiveGroup
{
	int		startIndex_;
	int		nPrimitives_;
	int		startVertex_;
	int		nVertices_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // PRIMITIVE_FILE_STRUCTS_HPP
