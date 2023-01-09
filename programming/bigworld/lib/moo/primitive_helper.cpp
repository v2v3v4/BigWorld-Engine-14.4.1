#include "pch.hpp"
#include "primitive_helper.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

PrimitiveHelper::PrimitiveType PrimitiveHelper::primitiveTypeFromDeviceEnum(
	D3DPRIMITIVETYPE value )
{
	switch ( value )
	{
	case D3DPT_POINTLIST:
		return POINT_LIST;
	case D3DPT_LINELIST:
		return LINE_LIST;
	case D3DPT_LINESTRIP:
		return LINE_STRIP;
	case D3DPT_TRIANGLELIST:
		return TRIANGLE_LIST;
	case D3DPT_TRIANGLESTRIP:
		return TRIANGLE_STRIP;
	case D3DPT_TRIANGLEFAN:
		return TRIANGLE_FAN;
	default:
		MF_ASSERT( false && "Unsupported primitive type");
		return TRIANGLE_LIST;
	}
}

bool PrimitiveHelper::getTriangleIterationParams( PrimitiveType primitiveType,
	uint32* pIndexMultiplier,
	uint32* pIndexBasedOrdering,
	uint32* pTotalCountNegativeOffset,
	bool complainOnUnsupportedType )
{
	uint32 indexMultiplier;
	uint32 indexBasedOrdering;
	uint32 totalCountNegativeOffset;

	if (primitiveType == PrimitiveType::TRIANGLE_LIST)
	{
		indexMultiplier = 3;
		indexBasedOrdering = 0;
		totalCountNegativeOffset = 0;
	}
	else if (primitiveType == PrimitiveType::TRIANGLE_STRIP)
	{
		indexMultiplier = 1;
		indexBasedOrdering = 1;
		totalCountNegativeOffset = 2;
	}
	else
	{
		// unsupported primitive type
		if (complainOnUnsupportedType)
		{
			CRITICAL_MSG("PrimitiveHelper::getPrimitiveIterationParams: unsupported primitive type: %d\n", static_cast<unsigned int>(primitiveType));
		}
		return false;
	}

	if (pIndexMultiplier)
	{
		*pIndexMultiplier = indexMultiplier;
	}

	if (pIndexBasedOrdering)
	{
		*pIndexBasedOrdering = indexBasedOrdering;
	}

	if (pTotalCountNegativeOffset)
	{
		*pTotalCountNegativeOffset = totalCountNegativeOffset;
	}

	return true;
}


bool PrimitiveHelper::isValidIndexBufferRange( uint32 indexBufferSize,
	uint32 startIndex, uint32 triangleCount, PrimitiveType primitiveType,
	bool complainOnInvalidRange )
{
	// No index buffer access required for no prims
	if (triangleCount == 0)
	{
		return true;
	}

	uint32 indexMultiplier;
	if (!getTriangleIterationParams( primitiveType, &indexMultiplier, NULL, NULL ) ||
		indexBufferSize == 0)
	{
		return false;
	}

	// Check if in bounds
	uint32 lastTriangleIndex = startIndex + (triangleCount - 1) * indexMultiplier;
	if (lastTriangleIndex < (indexBufferSize + 3))
	{
		return true;
	}
	else if (complainOnInvalidRange)
	{
		// Out of buffer bounds, warn
		CRITICAL_MSG("PrimitiveHelper::isValidIndexBufferRange: out of " 
			"index buffer (size=%d) bounds. tris=%d, start=%d, primType=%d\n", 
			indexBufferSize, triangleCount,  startIndex,
			static_cast<unsigned int>(primitiveType));
	}

	return false;
}

uint32 PrimitiveHelper::numTriangles( PrimitiveType primitiveType, 
	uint32 indexBufferSize, 
	uint32 startIndex )
{
	uint32 indexMultiplier;
	uint32 totalCountNegativeOffset;

	if (!getTriangleIterationParams( primitiveType, 
		&indexMultiplier, NULL, &totalCountNegativeOffset ))
	{
		return 0;
	}

	return ((indexBufferSize - startIndex) / indexMultiplier) - totalCountNegativeOffset;
}

} // namespace Moo

BW_END_NAMESPACE

