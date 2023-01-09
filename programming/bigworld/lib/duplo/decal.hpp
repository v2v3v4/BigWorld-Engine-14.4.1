#ifndef DECAL_HPP
#define DECAL_HPP

#include "moo/forward_declarations.hpp"
#include "moo/moo_dx.hpp"
#include "math/mathdef.hpp"
#include "math/vector3.hpp"
#include "math/vector2.hpp"
#include "math/matrix.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "moo/device_callback.hpp"
#include "moo/vertex_buffer.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
	class BaseTexture;
};

class WorldTriangle;

/**
 * Decal - the geometry is very closer to underlying substrate and has its own texture.
 * Decal has goup's by live time.
 */
class Decal : public Moo::DeviceCallback
{
public:
	Decal();
	~Decal();

	void add( 
		const Vector3& start, 
		const Vector3& end, 
		const Vector2&	size,
		float			yaw,
		int				textureIndex,
		int				groupIndex,
		int				textureIndex2 = 0,
		float			textureBlendFactor = 0,
		bool			mirrorU = false);

	void addWithGetH( 
		const Vector3&	start,
		const Vector3&	end,
		const Vector2&	size,
		float			yaw,
		int				type,
		int				groupIndex,
		int				textureIndex2 = 0,
		float			textureBlendFactor = 0);


	// add new decal group.
	int addDecalGroup(
		const BW::string&	groupName,
		float				lifeTime,
		uint				trianglesCount );

	// clear all decals from rendering queue.
	void clear();

	void tick(float dTime);
	virtual void draw( float dTime );

	virtual void deleteUnmanagedObjects();
	virtual void createUnmanagedObjects();

	int validateGroupName(const BW::string& groupName);

	static int decalTextureIndex(const BW::string& name);

#ifdef EDITOR_ENABLED
	void setViewSpaceOffset(uint groupIndex, const Vector3 &offset);
	void useZeroYCollisionPlane(uint groupIndex, bool use);
#endif

private:
	void _addTriangles(
		const WorldTriangle * triangles, 
		uint                  numTriangles, 
		const Matrix        & transform,
		int                   type,
		int					  groupIndex,
		int					  textureIndex2 = 0,
		float				  textureBlendFactor = 0,
		Vector2				 * uvValues = NULL ); // directly assigned UVs instead of calculated from world->texture transform (for track traces)

	Decal( const Decal& );
	Decal& operator=( const Decal& );
private:
	struct DecalVertex
	{
		Vector3	pos_;
		Vector4	uv_;
		Vector2	time_;
		static DWORD fvf() { return D3DFVF_XYZ|D3DFVF_TEX2|D3DFVF_TEXCOORDSIZE4(0)|D3DFVF_TEXCOORDSIZE2(1); };
	};

	struct ClipVertex
	{
		Vector3 pos_;
		Vector2 uv_;
		Outcode oc_;
		void calcOutcode()
		{
			oc_ = 0;
			if (uv_.x < 0)
				oc_ |= OUTCODE_LEFT;
			else if (uv_.x > 1.f)
				oc_ |= OUTCODE_RIGHT;

			if (uv_.y < 0)
				oc_ |= OUTCODE_TOP;
			else if (uv_.y > 1.f)
				oc_ |= OUTCODE_BOTTOM;
		}
	};

	void _clip( BW::vector<ClipVertex>& cvs );

	// decal's object. Holds the creating time and vertices count.
	struct DecalDesc
	{
		DecalDesc()
			:	time(0.0f),
				verticesCount(0)
		{

		}

		float	time;
		uint32	verticesCount;
	};

	// array of decal's description.
	typedef BW::deque<DecalDesc> DecalDescArray;

	// Decal group it is a type of decals with own vertices buffer size,
	// the life time, and other internal information.
	struct DecalGroup
	{
		DecalGroup()
			:	trianglesCount(0),
				pVertexBuffer(NULL),
				firstVertex(0),
				lastVertex(0),
				lifeTime(0.0f)
		{
//			vertices.reserve(500);
#ifdef EDITOR_ENABLED
			viewSpaceOffset.setZero();
			useZeroYCollisionPlane = false;
#endif
		}
				
		BW::string name;
		uint trianglesCount;
		Moo::VertexBuffer* pVertexBuffer;
		DecalDescArray decalDescArray;
		uint firstVertex;
		uint lastVertex;
		float lifeTime;
//		BW::vector<DecalVertex> vertices;
#ifdef EDITOR_ENABLED
		Vector3 viewSpaceOffset;
		bool useZeroYCollisionPlane;
#endif
	};

	typedef BW::vector<DecalGroup> DecalGroupArray;

	Moo::EffectMaterialPtr		pMaterial_; // if it not loaded the draw operation not performed.
	DecalGroupArray				decalGroups_;

	// time after start drawing the decals.
	float elapsedTime_;
	
	// current update time and update tick interval.
	float tickUpdateInterval_;
	float curTickUpdateTime_;

	SmartPointer<Moo::BaseTexture> tileTex_;

	float watcherTickTime_;
	static bool watchersInitialized_;
	static uint renderedTrianglesCountPerFrame_;
	static uint collisonTrianglesCountPerSecond_;
	static uint buffersMemoryAllocated_;
	static uint buffersCount_;
};

BW_END_NAMESPACE

#endif // DECAL_HPP
