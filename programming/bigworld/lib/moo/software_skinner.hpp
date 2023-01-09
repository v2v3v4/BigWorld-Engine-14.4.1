#ifndef SOFTWARE_SKINNER_HPP
#define SOFTWARE_SKINNER_HPP

#include "vertex_formats.hpp"
#include "node.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
/**
 * This function unpacks a compressed normal into a Vector3.
 */
inline Vector3 unpackIntNormal( uint32 packed )
{
	int32 z = int32(packed) >> 22;
	int32 y = int32( packed << 10 ) >> 21;
	int32 x = int32( packed << 21 ) >> 21;

	return Vector3( float( x ) / 1023.f, float( y ) / 1023.f, float( z ) / 511.f );
}

/**
 * This class represents a non-skinned vertex.
 */
class RigidSkinVertex
{
public:
	Vector3 position;
	Vector3 normal;
	Vector2 uv;
	int		index;
	typedef VertexXYZNUVI SecondaryType;
	typedef VertexXYZNUVIPC PrimaryType;

	RigidSkinVertex& operator = (const SecondaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = vert.normal_;
		this->uv = vert.uv_;
		this->index = int(vert.index_ / 3.f);

		return *this;
	}

	RigidSkinVertex& operator = (const PrimaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = vert.normal_;
		this->uv = vert.uv_;
		this->index = int(vert.index_ / 3.f);

		return *this;
	}

	void output( Vector3& pos, const NodePtrVector &nodes)
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		pos = wTrans.applyPoint( position );
	}
	void outputSimplified( Vector3& pos, const NodePtrVector &nodes)
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		pos = wTrans.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes )
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes )
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}

	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes,
		const Vector3& positionOverride)
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes,
		const Vector3& positionOverride)
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}
	void output( Vector3& pos, const Matrix* pTransforms)
	{
		const Matrix& wTrans = pTransforms[index];
		pos = wTrans.applyPoint( position );
	}
	void outputSimplified( Vector3& pos, const Matrix* pTransforms)
	{
		const Matrix& wTrans = pTransforms[index];
		pos = wTrans.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const Matrix* pTransforms )
	{
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms )
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}

	void output( VertexXYZNUV& vertex, const Matrix* pTransforms,
		const Vector3& positionOverride)
	{
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms,
		const Vector3& positionOverride)
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}
	void output( float& depth, const Vector4* partialWorldView )
	{
		const Vector4& wTrans = partialWorldView[index];
		depth = (((Vector3*)&wTrans)->dotProduct(position) + wTrans.w);
	}
	void output( float& depth, const Vector4* partialWorldView,
		const Vector3& positionOverride )
	{
		const Vector4& wTrans = partialWorldView[index];
		depth = (((Vector3*)&wTrans)->dotProduct(positionOverride) + wTrans.w);
	}
private:
};



/**
* This class represents a non-skinned vertex, for a normal-mapped surface.
*/
class RigidSkinBumpVertex
{
public:
	Vector3 position;
	Vector3 normal;
	Vector2 uv;
	int		index;
	Vector3 tangent;
	Vector3 binormal;
	typedef VertexXYZNUVITB SecondaryType;
	typedef VertexXYZNUVITBPC PrimaryType;

	RigidSkinBumpVertex& operator = (const SecondaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = unpackIntNormal(vert.normal_);
		this->uv = vert.uv_;
		this->index = int(vert.index_ / 3.f);
		this->tangent = unpackIntNormal( vert.tangent_ );
		this->binormal = unpackIntNormal( vert.binormal_ );

		return *this;
	}

	RigidSkinBumpVertex& operator = (const PrimaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = vert.normal_;
		this->uv = vert.uv_;
		this->index = int(vert.index_ / 3.f);
		this->tangent = vert.tangent_;
		this->binormal = vert.binormal_;

		return *this;
	}

	void output( Vector3& pos, const NodePtrVector& nodes )
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		pos = wTrans.applyPoint( position );
	}
	void outputSimplified( Vector3& pos, const NodePtrVector& nodes )
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		pos = wTrans.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes )
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes )
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans.applyVector( tangent );
		vertex.binormal_ = wTrans.applyVector( binormal );
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes,
		const Vector3& positionOverride)
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes,
		const Vector3& positionOverride)
	{
		const Matrix& wTrans = nodes[index]->worldTransform();
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans.applyVector( tangent );
		vertex.binormal_ = wTrans.applyVector( binormal );
	}
	void output( Vector3& pos, const Matrix* pTransforms )
	{
		const Matrix& wTrans = pTransforms[index];
		pos = wTrans.applyPoint( position );
	}
	void outputSimplified( Vector3& pos, const Matrix* pTransforms )
	{
		const Matrix& wTrans = pTransforms[index];
		pos = wTrans.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const Matrix* pTransforms )
	{
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms )
	{
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( position );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans.applyVector( tangent );
		vertex.binormal_ = wTrans.applyVector( binormal );
	}
	void output( VertexXYZNUV& vertex, const Matrix* pTransforms,
		const Vector3& positionOverride)
	{
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms,
		const Vector3& positionOverride)
	{
		const Matrix& wTrans = pTransforms[index];
		vertex.pos_ = wTrans.applyPoint( positionOverride );
		vertex.normal_ = wTrans.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans.applyVector( tangent );
		vertex.binormal_ = wTrans.applyVector( binormal );
	}
	void output( float& depth, const Vector4* partialWorldView )
	{
		const Vector4& wTrans = partialWorldView[index];
		depth = (((Vector3*)&wTrans)->dotProduct(position) + wTrans.w);
	}
	void output( float& depth, const Vector4* partialWorldView,
		const Vector3& positionOverride )
	{
		const Vector4& wTrans = partialWorldView[index];
		depth = (((Vector3*)&wTrans)->dotProduct(positionOverride) + wTrans.w);
	}
private:
};

/**
* This class represents a software skinned vertex with three weights.
*/
class SoftSkinVertex
{
public:
	Vector3 position;
	Vector3 normal;
	Vector2 uv;
	int		index1;
	int		index2;
	int		index3;
	float	weight1;
	float	weight2;
	float	weight3;
	typedef VertexXYZNUVIIIWW SecondaryType;
	typedef VertexXYZNUVIIIWWPC PrimaryType;

	SoftSkinVertex& operator = (const SecondaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = unpackIntNormal(vert.normal_);
		this->uv = vert.uv_;
		this->index1 = int(vert.index_ / 3);
		this->index2 = int(vert.index2_ / 3);
		this->index3 = int(vert.index3_ / 3);
		this->weight1 = float( vert.weight_ ) / 255.f;
		this->weight2 = float( vert.weight2_ ) / 255.f;
		this->weight3 = 1.f - this->weight1 - this->weight2;
		return *this;
	}

	SoftSkinVertex& operator = (const PrimaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = vert.normal_;
		this->uv = vert.uv_;
		this->index1 = int(vert.index_ / 3);
		this->index2 = int(vert.index2_ / 3);
		this->index3 = int(vert.index3_ / 3);
		this->weight1 = float( vert.weight_ ) / 255.f;
		this->weight2 = float( vert.weight2_ ) / 255.f;
		this->weight3 = 1.f - this->weight1 - this->weight2;
		return *this;
	}

	void output( Vector3& pos, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		pos = wTrans1.applyPoint( position ) * weight1 + 
				wTrans2.applyPoint( position ) * weight2 + 
				wTrans3.applyPoint( position ) * weight3;
	}
	void outputSimplified( Vector3& pos, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		pos = wTrans1.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes )
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes, 
		const Vector3& positionOverride )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes, 
		const Vector3& positionOverride )
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}
	void output( Vector3& pos, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		pos = wTrans1.applyPoint( position ) * weight1 + 
				wTrans2.applyPoint( position ) * weight2 + 
				wTrans3.applyPoint( position ) * weight3;
	}
	void outputSimplified( Vector3& pos, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		pos = wTrans1.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms )
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}
	void output( VertexXYZNUV& vertex, const Matrix* pTransforms, 
		const Vector3& positionOverride )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms, 
		const Vector3& positionOverride )
	{
		const Vector3 v3Zero(0,0,0);
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = v3Zero;
		vertex.binormal_ = v3Zero;
	}
	void output( float& depth, const Vector4* partialWorldView )
	{
		const Vector4& wTrans1 = partialWorldView[index1];
		const Vector4& wTrans2 = partialWorldView[index2];
		const Vector4& wTrans3 = partialWorldView[index3];
		depth = (((Vector3*)&wTrans1)->dotProduct(position) + wTrans1.w)  * weight1 + 
				(((Vector3*)&wTrans2)->dotProduct(position) + wTrans2.w)  * weight2 + 
				(((Vector3*)&wTrans3)->dotProduct(position) + wTrans3.w)  * weight3;
	}
	void output( float& depth, const Vector4* partialWorldView,
		const Vector3& positionOverride )
	{
		const Vector4& wTrans1 = partialWorldView[index1];
		const Vector4& wTrans2 = partialWorldView[index2];
		const Vector4& wTrans3 = partialWorldView[index3];
		depth = (((Vector3*)&wTrans1)->dotProduct(positionOverride) + wTrans1.w)  * weight1 + 
				(((Vector3*)&wTrans2)->dotProduct(positionOverride) + wTrans2.w)  * weight2 + 
				(((Vector3*)&wTrans3)->dotProduct(positionOverride) + wTrans3.w)  * weight3;
	}
private:
};

/**
 * This class represents a software skinned vertex with three weights, and 
 * tangent and binormal information (for a normal mapped surface).
 */
class SoftSkinBumpVertex
{
public:
	Vector3 position;
	Vector3 normal;
	Vector2 uv;
	int		index1;
	int		index2;
	int		index3;
	float	weight1;
	float	weight2;
	float	weight3;
	Vector3 tangent;
	Vector3 binormal;
	typedef VertexXYZNUVIIIWWTB SecondaryType;
	typedef VertexXYZNUVIIIWWTBPC PrimaryType;

	SoftSkinBumpVertex& operator = (const SecondaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = unpackIntNormal(vert.normal_);
		this->uv = vert.uv_;
		this->index1 = int(vert.index_ / 3);
		this->index2 = int(vert.index2_ / 3);
		this->index3 = int(vert.index3_ / 3);
		this->weight1 = float( vert.weight_ ) / 255.f;
		this->weight2 = float( vert.weight2_ ) / 255.f;
		this->weight3 = 1.f - this->weight1 - this->weight2;
		this->tangent = unpackIntNormal( vert.tangent_ );
		this->binormal = unpackIntNormal( vert.binormal_ );
		return *this;
	}

	SoftSkinBumpVertex& operator = (const PrimaryType& vert )
	{
		this->position = vert.pos_;
		this->normal = vert.normal_;
		this->uv = vert.uv_;
		this->index1 = int(vert.index_ / 3);
		this->index2 = int(vert.index2_ / 3);
		this->index3 = int(vert.index3_ / 3);
		this->weight1 = float( vert.weight_ ) / 255.f;
		this->weight2 = float( vert.weight2_ ) / 255.f;
		this->weight3 = 1.f - this->weight1 - this->weight2;
		this->tangent = vert.tangent_;
		this->binormal = vert.binormal_;
		return *this;
	}

	void output( Vector3& pos, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		pos = wTrans1.applyPoint( position ) * weight1 + 
				wTrans2.applyPoint( position ) * weight2 + 
				wTrans3.applyPoint( position ) * weight3;
	}
	void outputSimplified( Vector3& pos, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		pos = wTrans1.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans1.applyVector( tangent );
		vertex.binormal_ = wTrans1.applyVector( binormal );
	}
	void output( VertexXYZNUV& vertex, const NodePtrVector& nodes,
		const Vector3& positionOverride )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}
	void output( VertexXYZNUVTBPC& vertex, const NodePtrVector& nodes,
		const Vector3& positionOverride )
	{
		const Matrix& wTrans1 = nodes[index1]->worldTransform();
		const Matrix& wTrans2 = nodes[index2]->worldTransform();
		const Matrix& wTrans3 = nodes[index3]->worldTransform();
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans1.applyVector( tangent );
		vertex.binormal_ = wTrans1.applyVector( binormal );
	}

	void output( Vector3& pos, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		pos = wTrans1.applyPoint( position ) * weight1 + 
				wTrans2.applyPoint( position ) * weight2 + 
				wTrans3.applyPoint( position ) * weight3;
	}
	void outputSimplified( Vector3& pos, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		pos = wTrans1.applyPoint( position );
	}
	void output( VertexXYZNUV& vertex, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}

	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( position ) * weight1 + 
						wTrans2.applyPoint( position ) * weight2 + 
						wTrans3.applyPoint( position ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans1.applyVector( tangent );
		vertex.binormal_ = wTrans1.applyVector( binormal );
	}

	void output( VertexXYZNUV& vertex, const Matrix* pTransforms,
		const Vector3& positionOverride )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
	}

	void output( VertexXYZNUVTBPC& vertex, const Matrix* pTransforms,
		const Vector3& positionOverride )
	{
		const Matrix& wTrans1 = pTransforms[index1];
		const Matrix& wTrans2 = pTransforms[index2];
		const Matrix& wTrans3 = pTransforms[index3];
		vertex.pos_ = wTrans1.applyPoint( positionOverride ) * weight1 + 
						wTrans2.applyPoint( positionOverride ) * weight2 + 
						wTrans3.applyPoint( positionOverride ) * weight3;
		vertex.normal_ = wTrans1.applyVector( normal );
        vertex.uv_ = uv;
		vertex.tangent_ = wTrans1.applyVector( tangent );
		vertex.binormal_ = wTrans1.applyVector( binormal );
	}

	void output( float& depth, const Vector4* partialWorldView )
	{
		const Vector4& wTrans1 = partialWorldView[index1];
		const Vector4& wTrans2 = partialWorldView[index2];
		const Vector4& wTrans3 = partialWorldView[index3];
		depth = (((Vector3*)&wTrans1)->dotProduct(position) + wTrans1.w)  * weight1 + 
				(((Vector3*)&wTrans2)->dotProduct(position) + wTrans2.w)  * weight2 + 
				(((Vector3*)&wTrans3)->dotProduct(position) + wTrans3.w)  * weight3;
	}
	void output( float& depth, const Vector4* partialWorldView,
		const Vector3& positionOverride )
	{
		const Vector4& wTrans1 = partialWorldView[index1];
		const Vector4& wTrans2 = partialWorldView[index2];
		const Vector4& wTrans3 = partialWorldView[index3];
		depth = (((Vector3*)&wTrans1)->dotProduct(positionOverride) + wTrans1.w)  * weight1 + 
				(((Vector3*)&wTrans2)->dotProduct(positionOverride) + wTrans2.w)  * weight2 + 
				(((Vector3*)&wTrans3)->dotProduct(positionOverride) + wTrans3.w)  * weight3;
	}
private:
};

/**
 * This class is the interface for the software skinning.
 */
class BaseSoftwareSkinner : public ReferenceCount
{
public:
	virtual ~BaseSoftwareSkinner() {}
	virtual void transformVertices( VertexXYZNUV* pDestVertices, 
		int startVertex, int nVertices, const NodePtrVector& nodes, 
		const Vector3* pPositionOverrides = NULL ) = 0;
	virtual void transformVertices( VertexXYZNUVTBPC* pDestVertices, 
		int startVertex, int nVertices, const NodePtrVector& nodes, 
		const Vector3* pPositionOverrides = NULL ) = 0;
	virtual void transformVertices( VertexXYZNUV* pDestVertices, 
		int startVertex, int nVertices, const Matrix* pTransforms, 
		const Vector3* pPositionOverrides = NULL ) = 0;
	virtual void transformVertices( VertexXYZNUVTBPC* pDestVertices, 
		int startVertex, int nVertices, const Matrix* pTransforms, 
		const Vector3* pPositionOverrides = NULL ) = 0;
	virtual void outputDepths( float* pDestDepths,
		int startVertex, int nVertices, const Vector4* partialWVP,
		const Vector3* pPositionOverrides = NULL ) = 0;
	virtual void transformPositions( Vector3* pDestPos,
		int startVertex, int nVertices, const NodePtrVector& nodes,
		bool simplified = true, uint32 vertexSkip = 0 ) = 0;
	virtual void transformPositions( Vector3* pDestPos,
		int startVertex, int nVertices, const Matrix* pTransforms,
		bool simplified = true, uint32 vertexSkip = 0 ) = 0;

private:
};

typedef SmartPointer< BaseSoftwareSkinner > BaseSoftwareSkinnerPtr;

/**
 *	This class implements the software skinning interface templated
 *	over the vertex type. The vertex type stores the skinned vertex for
 *	easy software transform.
 *
 */
template <class VertexType>
class SoftwareSkinner : public BaseSoftwareSkinner
{
public:
	typedef BW::vector< VertexType > Vertices;
    typedef typename BW::vector< VertexType >::iterator VerticesIterator;

	SoftwareSkinner()
	{

	}

	~SoftwareSkinner()
	{

	}

	template<typename VT>
	void init( const VT * pVertices, int vertexCount )
	{
		vertices_.resize( vertexCount );
		VerticesIterator it = vertices_.begin();
		VerticesIterator end = vertices_.end();

		const VT* pVert = pVertices;

		while (it != end)
		{
			*(it++) = *(pVert++);
		}
	}

	void transformVertices( VertexXYZNUV* pDestVertices, int startVertex, 
		int nVertices, const NodePtrVector& nodes,
		const Vector3* pPositionOverrides )
	{
		IF_NOT_MF_ASSERT_DEV(unsigned(startVertex + nVertices) <= vertices_.size())
		{
			return;
		}
		static DogWatch s_skintimer( "swSkinning" );
		s_skintimer.start();
		VertexXYZNUV* pVert = pDestVertices;
		VerticesIterator it = vertices_.begin() + startVertex;
		VerticesIterator end = vertices_.begin() + startVertex + nVertices;
		if (pPositionOverrides)
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), nodes, *(pPositionOverrides++) );
			}
		}
		else
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), nodes );
			}
		}
		s_skintimer.stop();
	}

	void transformVertices( VertexXYZNUVTBPC* pDestVertices, int startVertex, 
		int nVertices, const NodePtrVector& nodes,
		const Vector3* pPositionOverrides )
	{
		IF_NOT_MF_ASSERT_DEV(unsigned(startVertex + nVertices) <= vertices_.size())
		{
			return;
		}
		static DogWatch s_skintimer( "swSkinning" );
		s_skintimer.start();
		VertexXYZNUVTBPC* pVert = pDestVertices;
		VerticesIterator it = vertices_.begin() + startVertex;
		VerticesIterator end = vertices_.begin() + startVertex + nVertices;
		if (pPositionOverrides)
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), nodes, *(pPositionOverrides++) );
			}
		}
		else
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), nodes );
			}
		}
		s_skintimer.stop();
	}

	void transformVertices( VertexXYZNUV* pDestVertices, int startVertex, 
		int nVertices, const Matrix* pTransforms,
		const Vector3* pPositionOverrides )
	{
		IF_NOT_MF_ASSERT_DEV(unsigned(startVertex + nVertices) <= vertices_.size())
		{
			return;
		}
		static DogWatch s_skintimer( "swSkinning" );
		s_skintimer.start();
		VertexXYZNUV* pVert = pDestVertices;
		VerticesIterator it = vertices_.begin() + startVertex;
		VerticesIterator end = vertices_.begin() + startVertex + nVertices;
		if (pPositionOverrides)
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), pTransforms, *(pPositionOverrides++) );
			}
		}
		else
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), pTransforms );
			}
		}
		s_skintimer.stop();
	}

	void transformVertices( VertexXYZNUVTBPC* pDestVertices, int startVertex, 
		int nVertices, const Matrix* pTransforms,
		const Vector3* pPositionOverrides )
	{
		IF_NOT_MF_ASSERT_DEV(unsigned(startVertex + nVertices) <= vertices_.size())
		{
			return;
		}
		static DogWatch s_skintimer( "swSkinning" );
		s_skintimer.start();
		VertexXYZNUVTBPC* pVert = pDestVertices;
		VerticesIterator it = vertices_.begin() + startVertex;
		VerticesIterator end = vertices_.begin() + startVertex + nVertices;
		if (pPositionOverrides)
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), pTransforms, *(pPositionOverrides++) );
			}
		}
		else
		{
			while (it != end)
			{
				(it++)->output( *(pVert++), pTransforms );
			}
		}
		s_skintimer.stop();
	}

	virtual void outputDepths( float* pDestDepths,
		int startVertex, int nVertices, const Vector4* partialWVP,
		const Vector3* pPositionOverrides = NULL )
	{
		float* pDepth = pDestDepths;
		VerticesIterator it = vertices_.begin() + startVertex;
		VerticesIterator end = vertices_.begin() + startVertex + nVertices;
		if (pPositionOverrides)
		{
			while (it != end)
			{
				(it++)->output( *(pDepth++), partialWVP, *(pPositionOverrides++) );
			}
		}
		else
		{
			while (it != end)
			{
				(it++)->output( *(pDepth++), partialWVP );
			}
		}
	}
	virtual void transformPositions( Vector3* pDestPos,
		int startVertex, int nVertices, const NodePtrVector& nodes,
		bool simplified = true, uint32 vertexSkip = 0 )
	{
		uint32 step = vertexSkip + 1;
		uint32 end = startVertex + nVertices;
		if (simplified)
		{
			for (uint32 i = startVertex; i < end; i += step)
			{
				vertices_[i].outputSimplified( *(pDestPos++), nodes );
			}
		}
		else
		{
			for (uint32 i = startVertex; i < end; i += step)
			{
				vertices_[i].output( *(pDestPos++), nodes );
			}
		}
	}

	virtual void transformPositions( Vector3* pDestPos,
		int startVertex, int nVertices, const Matrix* pTransforms,
		bool simplified = true, uint32 vertexSkip = 0 )
	{
		uint32 step = vertexSkip + 1;
		uint32 end = startVertex + nVertices;
		if (simplified)
		{
			for (uint32 i = startVertex; i < end; i += step)
			{
				vertices_[i].outputSimplified( *(pDestPos++), pTransforms );
			}
		}
		else
		{
			for (uint32 i = startVertex; i < end; i += step)
			{
				vertices_[i].output( *(pDestPos++), pTransforms );
			}
		}
	}

private:
	Vertices vertices_;
	SoftwareSkinner( const SoftwareSkinner& );
	SoftwareSkinner& operator=( const SoftwareSkinner& );
};

} // namespace Moo

BW_END_NAMESPACE

#endif // SOFTWARE_SKINNER_HPP
