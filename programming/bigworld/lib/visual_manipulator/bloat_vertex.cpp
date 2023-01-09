#include "pch.hpp"

#include "bloat_vertex.hpp"


BW_BEGIN_NAMESPACE

using namespace VisualManipulator;

BloatVertex::BloatVertex() :
	pos(0,0,0),
	normal(0,0,0),
	binormal(0,0,0),
	tangent(0,0,0),
	colour(0,0,0,0),
	uv(0,0),
	uv2(0,0),
	meshIndex(0),
	vertexIndex(0),
	smoothingGroup(0),
	weights(0,0,0,0)
{
	indices[0] = 0; indices[1] = 0; indices[2] = 0; indices[3] = 0;
}


void BloatVertex::output( Moo::VertexXYZNUV& v ) const
{
	v.pos_ = pos;
	v.normal_ = normal;
	v.uv_ = uv;
}


void BloatVertex::output( Moo::VertexXYZNUVIIIWW& v ) const
{
	v.pos_ = pos;
	v.normal_ = Moo::packNormal(normal);
	v.uv_ = uv;
	v.index_ = indices[0] * 3;
	v.index2_ = indices[1] * 3;
	v.index3_ = indices[2] * 3;

	float sum = weights.x + weights.y + weights.z;
	float weightMul = 255.f / sum;
	v.weight_ = uint8(weights.x * weightMul);
	v.weight2_ = uint8(weights.y * weightMul);
}


void BloatVertex::output( Moo::VertexXYZNUVTB& v ) const
{
	v.pos_ = pos;
	v.normal_ = Moo::packNormal(normal);
	v.uv_ = uv;
	v.tangent_ = Moo::packNormal(tangent);
	v.binormal_ = Moo::packNormal(binormal);
}


void BloatVertex::output( Moo::VertexXYZNUVIIIWWTB& v ) const
{
	v.pos_ = pos;
	v.normal_ = Moo::packNormal(normal);
	v.uv_ = uv;
	v.index_ = indices[0] * 3;
	v.index2_ = indices[1] * 3;
	v.index3_ = indices[2] * 3;
	v.tangent_ = Moo::packNormal(tangent);
	v.binormal_ = Moo::packNormal(binormal);

	float sum = weights.x + weights.y + weights.z;
	float weightMul = 255.f / sum;
	v.weight_ = uint8(weights.x * weightMul);
	v.weight2_ = uint8(weights.y * weightMul);
}


bool BloatVertex::operator == ( const BloatVertex& bv ) const 
{ 
	return  this->pos == bv.pos &&
	this->normal == bv.normal &&
	this->uv == bv.uv &&
	this->vertexIndex == bv.vertexIndex &&
	this->colour == bv.colour &&
	this->uv2 == bv.uv2 &&
	this->smoothingGroup == bv.smoothingGroup &&
	this->binormal == bv.binormal &&
	this->tangent == bv.tangent &&
	this->meshIndex == bv.meshIndex &&
	weights == bv.weights &&
	indices[0] == bv.indices[0] &&
	indices[1] == bv.indices[1] &&
	indices[2] == bv.indices[2] &&
	indices[3] == bv.indices[3];

};

bool BloatVertex::operator < ( const BloatVertex& bv ) const 
{
	if( pos < bv.pos )	return true;
	if( pos > bv.pos )	return false;

	if( normal < bv.normal )	return true;
	if( normal > bv.normal )	return false;

	if( uv < bv.uv )	return true;
	if( uv > bv.uv )	return false;

	if( vertexIndex < bv.vertexIndex )	return true;
	if( vertexIndex > bv.vertexIndex )	return false;

	if( colour < bv.colour )	return true;
	if( colour > bv.colour )	return false;

	if( uv2 < bv.uv2 )	return true;
	if( uv2 > bv.uv2 )	return false;

	if( binormal < bv.binormal )	return true;
	if( binormal > bv.binormal )	return false;

	if( tangent < bv.tangent )	return true;
	if( tangent > bv.tangent )	return false;

	if( meshIndex < bv.meshIndex )	return true;
	if( meshIndex > bv.meshIndex )	return false;

	if( weights < bv.weights )	return true;
	if( weights > bv.weights )	return false;

	for (uint32 i = 0; i < 4; i++)
	{
		if( indices[i] < bv.indices[i] )	return true;
		if( indices[i] > bv.indices[i] )	return false;
	}

	return false; // enough
};

BW_END_NAMESPACE
