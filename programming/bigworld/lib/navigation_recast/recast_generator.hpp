#ifndef RECAST_GENERATOR_HPP
#define RECAST_GENERATOR_HPP


#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"

#include "math/vector3.hpp"
#include "math/boundbox.hpp"
#include "chunk/collision_scene_provider.hpp"

struct rcHeightfield;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcPolyMesh;
struct rcPolyMeshDetail;
class rcContext;

BW_BEGIN_NAMESPACE

class Chunk;


class RecastConfig
{
	BoundingBox chunkBB_;
	float maxWalkableSlopeInAngle_;
	float maxAgentClimb_;
	float agentHeight_;
	float agentRadius_;
	bool scaleNavigation_;
	float gridSize_;

public:
	RecastConfig( const BoundingBox& chunkBB, float maxWalkableSlopeInAngle,
		float maxAgentClimb, float agentHeight, float agentRadius,
		bool scaleNavigation, float gridSize )
		: chunkBB_( chunkBB ), maxWalkableSlopeInAngle_( maxWalkableSlopeInAngle ),
		maxAgentClimb_( maxAgentClimb ), agentHeight_( agentHeight ),
		agentRadius_( agentRadius ), scaleNavigation_( scaleNavigation ),
		gridSize_ ( gridSize )
	{}

	const BoundingBox& chunkBB() const		{ return chunkBB_; }
	float maxWalkableSlopeInAngle() const	{ return maxWalkableSlopeInAngle_; }
	float maxAgentClimb() const				{ return maxAgentClimb_; }
	float agentHeight() const				{ return agentHeight_; }
	float agentRadius() const				{ return agentRadius_; }
	bool scaleNavigation() const			{ return scaleNavigation_; }
	float gridSize() const					{ return gridSize_; }
};


class RecastGenerator
{
	template<typename T>
	class AutoPtr
	{
		typedef T* (*Alloc)();
		typedef void (*Free)( T* );

		Alloc alloc_;
		Free free_;
		T* ptr_;
	public:
		AutoPtr( Alloc alloc, Free free )
			: alloc_( alloc ), free_( free ), ptr_( NULL )
		{}
		~AutoPtr()
		{
			free_( ptr_ );
		}
		operator T*()
		{
			return ptr_;
		}
		operator T*() const
		{
			return ptr_;
		}
		T* operator->()
		{
			return ptr_;
		}
		const T* operator->() const
		{
			return ptr_;
		}
		void destroy()
		{
			free_( ptr_ );
			ptr_ = NULL;
		}
		void recreate()
		{
			if( ptr_ )
				free_( ptr_ );
			ptr_ = alloc_();
		}
	};

	BW::vector<unsigned char> navmesh_;
	AutoPtr<rcHeightfield> heightField_;
	AutoPtr<rcCompactHeightfield> compactHeightField_;
	AutoPtr<rcContourSet> contourSet_;
	AutoPtr<rcPolyMesh> polyMesh_;
	AutoPtr<rcPolyMeshDetail> polyMeshDetail_;
	AutoPtr<rcContext> context_;

public:
	RecastGenerator();

	bool generate( const CollisionSceneProviders& provider,
		const RecastConfig& config, int gridX, int gridZ, bool useMono );
	void convertRecastMeshToNavgenMesh( float girth, Chunk& chunk, BW::vector<unsigned char>& navgenMesh );
	void stop();

	const BW::vector<unsigned char>& navmesh() const { return navmesh_; }

	static BW::set<Chunk*> collectChunks( Chunk* primary );
	static CollisionSceneProviders collectTriangles( Chunk* primary );
private:
	struct MeshVertex
	{
		// The position of the vert
		Vector3 position_;
		// Neighbouring polygon index (-1 for no link, 65535 for polygon edge
		//  which is a portal that links to another polygon)
		int connection_;
	};


	struct Mesh
	{
		// The mesh verts
		BW::vector<MeshVertex> vertices_;
		// The lowest point in the mesh
		float minHeight_;
		// The heighest point in the mesh
		float maxHeight_;
		// The set which the mesh belongs to
		int set_;
	};

	static bool isPolygonInChunk( const Mesh& mesh, Chunk& chunk );

	static void convertMeshSet( const BW::vector<Mesh>& meshes,
		float girth, BW::vector<unsigned char>& result );
	static void buildMeshSet( BW::vector<Mesh>& meshes );
};

BW_END_NAMESPACE

#endif //RECAST_GENERATOR_HPP
