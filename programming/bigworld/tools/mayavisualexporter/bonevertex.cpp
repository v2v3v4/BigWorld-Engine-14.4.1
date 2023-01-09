#include "pch.hpp"

#include "bonevertex.hpp"

BW_BEGIN_NAMESPACE

BoneVertex::BoneVertex( const Point3& position, int index1, int index2, int index3, float weight1, float weight2, float weight3 ) :
	position( position ), index1( index1 ), index2( index2 ), index3( index3 ), weight1( weight1 ), weight2( weight2 ), weight3( weight3 )
{
}

BW_END_NAMESPACE

