#include "recast.h"
#include "RecastDump.h"
#include "recast_generator.hpp"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMesh.h"

#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/geometry_mapping.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

// Uncomment to dump polymesh and detailed polymesh from recast
// these files will be stored in folders poly and detail in the
// current working directory, in OBJ format.
//#define DUMP_POLYMESH

// Uncomment to dump obj of triangles sent to recast from collision scene
//#define DUMP_COLLISION_SCENE

#ifdef DUMP_POLYMESH
class FileIO : public duFileIO
{
public:
	FileIO() :
		fp_(0),
		mode_(-1)
	{
	}

	virtual ~FileIO()
	{
		if (fp_)
		{
			fclose(fp_);
		}
	}

	bool openForWrite(const char* path)
	{
		if (fp_)
		{
			return false;
		}

		fp_ = fopen(path, "wb");
		if (!fp_)
		{
			return false;
		}

		mode_ = 1;
		return true;
	}

	bool openForRead(const char* path)
	{
		if (fp_)
		{
			return false;
		}

		fp_ = fopen(path, "rb");
		if (!fp_)
		{
			return false;
		}

		mode_ = 2;
		return true;
	}

	virtual bool isWriting() const
	{
		return mode_ == 1;
	}

	virtual bool isReading() const
	{
		return mode_ == 2;
	}

	virtual bool write(const void* ptr, const size_t size)
	{
		if ( !fp_ || mode_ != 1)
		{
			return false;
		}
		fwrite(ptr, size, 1, fp_);
		return true;
	}

	virtual bool read(void* ptr, const size_t size)
	{
		if ( !fp_ || mode_ != 2)
		{
			return false;
		}
		fread(ptr, size, 1, fp_);
		return true;
	}
private:
	FILE* fp_;
	int mode_;
};
#endif

class RecastConfigInternal : public rcConfig, public RecastConfig
{
public:
	RecastConfigInternal( const RecastConfig& config );
};

// Recast collision scene consumer
// This class will consume triangles and add them to a recast height field
class RecastCollisionSceneConsumer : public CollisionSceneConsumer
{
	static const int MAX_TRIANGLES = 512;
	Vector3 vertices_[ MAX_TRIANGLES * 3 ];
	unsigned char areas_[ MAX_TRIANGLES ];
	int verticesNum_;

	Vector3 portalVertices_[ MAX_TRIANGLES * 3 ];
	unsigned char portalAreas_[ MAX_TRIANGLES ];
	int portalVerticesNum_;

	rcContext* context_;
	rcHeightfield* heightField_;
	float walkableSlopeAngle_;
	int walkableClimb_;

public:
	RecastCollisionSceneConsumer( rcContext* context, 
		rcHeightfield* heightField, float walkableSlopeAngle,
		int walkableClimb ) 
		: verticesNum_( 0 ),
		portalVerticesNum_( 0 ),
		context_( context ),
		heightField_( heightField ),
		walkableSlopeAngle_ ( walkableSlopeAngle ),
		walkableClimb_( walkableClimb )
	{
		memset(areas_, RC_NULL_AREA, sizeof(areas_));
		memset(portalAreas_, RC_NULL_AREA, sizeof(portalAreas_));
	}

	virtual void consume( const Vector3& v )
	{
		vertices_[ verticesNum_ ] = v;
		++verticesNum_;

		// If our buffer is full then do a periodic flush
		if (verticesNum_ == MAX_TRIANGLES * 3)
		{
			flush();
		}
	}

	
	virtual void consumePortal( const Vector3& v )
	{
		portalVertices_[ portalVerticesNum_ ] = v;
		++portalVerticesNum_;

		// If our buffer is full then do a periodic flush
		if (portalVerticesNum_ == MAX_TRIANGLES * 3)
		{
			flush();
		}
	}

	void rasterizeTris ( float* vertices, unsigned char* areas, int vertCount )
	{
		// Check that we actually have a triangle
		MF_ASSERT( vertCount % 3 == 0 );

		// Set RC_WALKABLE_AREA for every triangle with a slope below  max 
		// walkable slope angle
		rcMarkWalkableTriangles( context_, walkableSlopeAngle_,
			vertices, vertCount / 3, areas );

		// Rasterize triangle mesh into the heightfield spans
		rcRasterizeTriangles( context_, vertices, areas,
			vertCount / 3, *heightField_, walkableClimb_ );
	}

	virtual void flush()
	{
		if (verticesNum_ != 0)
		{
			this->rasterizeTris( (float*)vertices_, areas_, verticesNum_ );
		}

		if (portalVerticesNum_ != 0)
		{
			this->rasterizeTris( (float*)portalVertices_, portalAreas_,
				portalVerticesNum_ );
		}

		verticesNum_ = 0;
		portalVerticesNum_ = 0;

		memset(areas_, RC_NULL_AREA, sizeof(areas_));
		memset(portalAreas_, RC_NULL_AREA, sizeof(portalAreas_));
	}

	virtual bool stopped() const
	{
		return context_->stopped();
	}
};


RecastConfigInternal::RecastConfigInternal( const RecastConfig& config )
	: RecastConfig( config )
{
	static const float SCALE_BASE_GRID_SIZE = 100.0f;

	static float CELL_SIZE = 0.23f;
	static float CELL_HEIGHT = 0.2f;

	// Any areas smaller then this will be removed from the nav mesh
	static int REGION_MIN_AREA = 0;

	// Any areas smaller then this will possibly be merged with other meshes
	static int REGION_MAX_AREA = 400;

	static int MAX_VERTS_PER_POLY = 6; // This should never exceed 6

	// Detail mesh sampling distance to use when generating the detail mesh
	static float DETAIL_SAMPLE_DIST = 6.f; // (0 or >= 0.9)

	// The max distance a simplied contour's border edge should deviate the
	// original raw contour
	static float EDGE_MAX_ERROR = 1.3f;

	// The max distance the detail mesh should deviate from the heightfield data
	static float SAMPLE_MAX_ERROR = 1.f;

	// The maximum allowed length of a contour edge along the border of the mesh.
	static float EDGE_MAX_LENGTH = 12.f;


	memset((rcConfig*)this, 0, sizeof(rcConfig));

	// Set cell size and height
	cs = CELL_SIZE;
	ch = CELL_HEIGHT;

	if (config.scaleNavigation())
	{
		cs *= config.gridSize() / SCALE_BASE_GRID_SIZE;
		ch *= config.gridSize() / SCALE_BASE_GRID_SIZE;
	}
	
	// Walkable slope (in degrees)
	walkableSlopeAngle = maxWalkableSlopeInAngle();

	// Walk height, climb, radius (in voxel units)
	walkableHeight = (int)ceilf( agentHeight() / ch );
	walkableClimb = (int)floorf( maxAgentClimb() / ch );
	walkableRadius = (int)ceilf( agentRadius() / cs );

	// Max edge length (in voxel units)
	maxEdgeLen = (int)( EDGE_MAX_LENGTH / cs );

	// Max error for simplfied contour's border edges
	maxSimplificationError = EDGE_MAX_ERROR;

	// The minmum number of cells allowed to form isolated island areas
	minRegionArea = REGION_MIN_AREA;

	// Any region with a span smllaer then this value will (if possible) be 
	// merged with larger regions
	mergeRegionArea = REGION_MAX_AREA;

	maxVertsPerPoly = MAX_VERTS_PER_POLY;

	// Get edge length
	float edgeLength = std::max<float>(
		chunkBB().maxBounds().x - chunkBB().minBounds().x,
		chunkBB().maxBounds().z - chunkBB().minBounds().z );

	// The width/height size of tiles (in voxel units)
	// each tile will be represent a chunk
	tileSize = (int)ceilf( edgeLength / cs );

	// The size of the non-walkable area around height field
	//   in this case the walk radius + 3  (in voxel units)
	borderSize = walkableRadius + 3;

	width = tileSize + borderSize * 2; 
	height = tileSize + borderSize * 2;

	// The sampling distance when generating the detailed mesh 
	detailSampleDist = DETAIL_SAMPLE_DIST;

	// The maximum distance the detail mesh surface should deviate from the 
	// height field.
	detailSampleMaxError = ch * SAMPLE_MAX_ERROR;

	// Set bounds of the fields AABB
	bmin[0] = chunkBB().minBounds().x;
	bmin[1] = chunkBB().minBounds().y;
	bmin[2] = chunkBB().minBounds().z;

	bmax[0] = chunkBB().maxBounds().x;
	bmax[1] = chunkBB().maxBounds().y;
	bmax[2] = chunkBB().maxBounds().z;

	// A navmesh that is connected to another chunk has to be slightly larger 
	// than the chunks.The border size parameter cannot be used in this case as
	// the border only stores the intermediate result and any mesh inside the
	// border area will be culled later.So the bounding box must be increased
	// and the border size reduced, to ensure it can get 2 grids outside the
	// chunk.
	bmin[0] -= borderSize * cs;
	bmin[1] -= borderSize * cs;
	bmin[2] -= borderSize * cs;
	bmax[0] += borderSize * cs;
	bmax[1] += borderSize * cs;
	bmax[2] += borderSize * cs;

	borderSize -= 2;
}


namespace
{
	class RcContextLog : public rcContext
	{
	public:
		inline RcContextLog(bool state = true) : rcContext(state) { }
		virtual ~RcContextLog() { }
	protected:
		virtual void doLog(const rcLogCategory category, const char* msg, 
			const int len)
		{
			if (category == RC_LOG_ERROR)
				ERROR_MSG("%s\n", msg);
			else if (category == RC_LOG_WARNING)
				WARNING_MSG("%s\n", msg);
			else
				INFO_MSG("%s\n", msg);
		}
	};

	rcContext* rcAllocContext()
	{
#ifdef _DEBUG
		return new RcContextLog;
#else
		return new rcContext;
#endif
	}


	void rcFreeContext( rcContext* ctx )
	{
		bw_safe_delete(ctx);
	}

} // anonymous namespace


RecastGenerator::RecastGenerator()
	: heightField_( rcAllocHeightfield, rcFreeHeightField ),
	compactHeightField_( rcAllocCompactHeightfield, rcFreeCompactHeightfield ),
	contourSet_( rcAllocContourSet, rcFreeContourSet ),
	polyMesh_( rcAllocPolyMesh, rcFreePolyMesh ),
	polyMeshDetail_( rcAllocPolyMeshDetail, rcFreePolyMeshDetail ),
	context_( rcAllocContext, rcFreeContext )
{
	context_.recreate();
}

bool RecastGenerator::generate( const CollisionSceneProviders& provider,
					const RecastConfig& cfg, int gridX, int gridZ,bool useMono )
{
	// Create data structures
	heightField_.recreate();
	compactHeightField_.recreate();
	contourSet_.recreate();
	polyMesh_.recreate();
	polyMeshDetail_.recreate();

	RecastConfigInternal config( cfg );

	// Create and initalize new height field.
	if (!rcCreateHeightfield( context_, *heightField_, config.width,
		config.height, config.bmin, config.bmax, config.cs, config.ch ))
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not create solid heightfield.\n");
		return false;
	}
	
	// Mark walkable areas
	RecastCollisionSceneConsumer consumer( context_, heightField_,
		config.walkableSlopeAngle, config.walkableClimb );

	if (!provider.feed( &consumer ))
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not mark walkable areas.\n");
		return false;
	}

	// Mark non-walkable low obstacles as walkable if they are close than
	// walkableClimb from the walkable surface so they can step over low hanging
	// obstacles.
	rcFilterLowHangingWalkableObstacles( context_, config.walkableClimb,
		*heightField_ );

	if (context_->stopped())
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not remove low hanging obstacles.\n");
		return false;
	}

	// Remove WALKABLE flag from all spans that are at ledges, this filtering 
	// removes possible over estimation of the conservative voxelation so that
	// the resulting mesh will not have regions hanging in the air over ledges.
	rcFilterLedgeSpans( context_, config.walkableHeight, config.walkableClimb,
		*heightField_ );

	if (context_->stopped())
	{
		WARNING_MSG("RecastGenerator::generate(): Could not filter ledges.\n");
		return false;
	}

	// Removes WALKABLE flag from all spans which have a smaller height then
	// walkable Height
	rcFilterWalkableLowHeightSpans (context_, config.walkableHeight, 
		*heightField_ );

	if (context_->stopped())
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not filter low spans.\n");
		return false;
	}

	if (!provider.feedPortals( &consumer ))
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not mark walkable areas.\n");
		return false;
	}

	// Builds the compact representation of the heightfield
	if (!rcBuildCompactHeightfield( context_, config.walkableHeight, 
		config.walkableClimb, *heightField_, *compactHeightField_ ))
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not build compact height field.\n");
		return false;
	}

	// Remove non-compact height field
	heightField_.destroy();

	// Erode the edges of waypoints around walls by the walkable radius to
	// ensure the AI does not get too close to walls.
	if (!rcErodeWalkableArea( context_, config.walkableRadius,
		*compactHeightField_ ))
	{
		WARNING_MSG("RecastGenerator::generate(): Could not erode.\n");
		return false;
	}

	if (useMono)
	{
		if (!rcBuildRegionsMonotone( context_, *compactHeightField_,
			config.borderSize, config.minRegionArea, config.mergeRegionArea ))
		{
			WARNING_MSG("RecastGenerator::generate(): "
				"Could not build regions (monotone)\n");
			return false;
		}
	}
	else
	{
		// Prepare for region partitioning, by calculating distance field along
		// the walkable surface
		if (!rcBuildDistanceField( context_, *compactHeightField_ ))
		{
			WARNING_MSG("RecastGenerator::generate(): "
				"Could not build distance field.\n");
			return false;
		}

		// Partition the walkable surface into simple regions without holes
		if (!rcBuildRegions( context_, *compactHeightField_, config.borderSize,
			config.minRegionArea, config.mergeRegionArea ))
		{
			WARNING_MSG("RecastGenerator::generate(): "
				"Could not build regions.\n");
			return false;
		}
	}


	// Build simplified contours from the regions outlines
	if (!rcBuildContours( context_, *compactHeightField_,
		config.maxSimplificationError, config.maxEdgeLen, *contourSet_ ))
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not create contours.\n");
		return false;
	}

	// Build connected convex polygon mesh from the contour polygons
	if (!rcBuildPolyMesh( context_, *contourSet_, config.maxVertsPerPoly,
		*polyMesh_ ))
	{
		WARNING_MSG("RecastGenerator::generate(): "
			"Could not triangulate contours.\n");
		return false;
	}

	// Build detail triangle mesh for each polygon in the polygon mesh
	if (!rcBuildPolyMeshDetail( context_, *polyMesh_, *compactHeightField_,
		config.detailSampleDist, config.detailSampleMaxError, 
		*polyMeshDetail_ ))
	{
		WARNING_MSG("RecastGenerator::generate():"
			"Could build polymesh detail.\n");
		return false;
	}

	// Cleanup memory
	compactHeightField_.destroy();
	contourSet_.destroy();

	unsigned char* navData = 0;
	int navDataSize = 0;

	dtNavMeshCreateParams params = { 0 };

	params.verts = polyMesh_->verts;
	params.vertCount = polyMesh_->nverts;
	params.polys = polyMesh_->polys;
	params.polyAreas = polyMesh_->areas;
	params.polyFlags = polyMesh_->flags;
	params.polyCount = polyMesh_->npolys;
	params.nvp = polyMesh_->nvp;
	params.detailMeshes = polyMeshDetail_->meshes;
	params.detailVerts = polyMeshDetail_->verts;
	params.detailVertsCount = polyMeshDetail_->nverts;
	params.detailTris = polyMeshDetail_->tris;
	params.detailTriCount = polyMeshDetail_->ntris;
	params.offMeshConVerts = 0;
	params.offMeshConRad = 0;
	params.offMeshConDir = 0;
	params.offMeshConAreas = 0;
	params.offMeshConFlags = 0;
	params.offMeshConUserID = 0;
	params.offMeshConCount = 0;
	params.walkableHeight = config.agentHeight();
	params.walkableRadius = config.agentRadius();
	params.walkableClimb = config.maxAgentClimb();
	params.tileX = gridX;
	params.tileY = gridZ;
	params.tileLayer = 0;
	rcVcopy( params.bmin, polyMesh_->bmin );
	rcVcopy( params.bmax, polyMesh_->bmax );
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = false; // Bounding volume is not needed

	// Create nav mesh from poly mesh and detail poly mesh
	if (dtCreateNavMeshData( &params, &navData, &navDataSize ))
	{
		navmesh_.assign( navData, navData + navDataSize );

		dtFree( navData );
	}

	return true;
}


void RecastGenerator::stop()
{
	context_->stop();
}

/* static */void RecastGenerator::buildMeshSet( BW::vector<Mesh>& meshes )
{
	int curSet = 0;
	typedef BW::vector<Mesh>::size_type Index;

	for (Index i = 0; i < meshes.size(); ++i)
	{
		// If mesh is not a member of a set yet
		if (meshes[ i ].set_ == -1)
		{
			BW::set<Index> visit;

			// Add mesh to visited set
			visit.insert( i );

			while (!visit.empty())
			{
				// Get top mesh from visit set
				Mesh& mesh = meshes[ *visit.begin() ];
				visit.erase( visit.begin() );

				// If this mesh is not a member of a set yet
				if (mesh.set_ == -1)
				{
					// This mesh is not part of a set, so add it to one
					mesh.set_ = curSet;

					// Foreach vert within this mesh
					for (Index vi = 0; vi < mesh.vertices_.size(); ++vi)
					{
						int conn = mesh.vertices_[ vi ].connection_;

						if (conn != -1 && conn != 65535)
						{
							// Add connection to visit set
							visit.insert( conn );
						}
					}
				}
			}

			++curSet;
		}
	}

	BW::vector<Mesh> temp( meshes );
	// Index map between old mesh indexes (pre set grouping) and new mesh
	//  indexes (post set grouping)
	BW::map<int, int> indexMap;
	Index meshIndex = 0;


	// Reorder the mesh indexes again so sets are next to each other
	for (int set = 0; set < curSet; ++set)
	{
		for (Index mi = 0; mi < temp.size(); ++mi)
		{
			if (temp[ mi ].set_ == set)
			{
				meshes[ meshIndex ] = temp[ mi ];
				indexMap[ static_cast<int>(mi) ] = static_cast<int>(meshIndex);

				++meshIndex;
			}
		}
	}

	indexMap[ -1 ] = -1;
	indexMap[ 65535 ] = 65535;


	for (Index i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[ i ];

		for (Index j = 0; j < mesh.vertices_.size(); ++j)
		{
			// Update connections to point to new mesh index
			//  (after grouping sets)
			mesh.vertices_[ j ].connection_ =
				indexMap[ mesh.vertices_[ j ].connection_ ];
		}
	}
}

template <typename T>
void push( BW::vector<unsigned char>& buffer, const T& t)
{
	BW::vector<unsigned char>::size_type offset = buffer.size();

	buffer.resize( offset + sizeof( t ) );
	*(T*)&buffer[offset] = t;
}


/* static */void RecastGenerator::convertMeshSet(
	const BW::vector<Mesh>& meshes, float girth,
	BW::vector<unsigned char>& result )
{
	// Adjustment for min height to better include the original mesh
	// recast normally puts everything above the voxels so we must adjust the
	// the min height, to include the mesh below it.
	const float SINK_MESH_OFFSET  = -1.0f;

	typedef BW::vector<Mesh>::size_type Index;

	// Foreach set (even though this is using meshes size)
	for (Index i = 0; i < meshes.size();)
	{
		int set = meshes[ i ].set_;
		// The total number of meshes in this set
		int meshNum = 0;
		// The total number of edges within this set
		int edgeNum = 0;

		for (Index mi = i; mi < meshes.size(); ++mi)
		{
			const Mesh& mesh = meshes[ mi ];

			if (mesh.set_ == set)
			{
				++meshNum;
				edgeNum += (int)mesh.vertices_.size();
			}
			else
			{
				// We can break now, as the meshes are already grouped by set
				break;
			}
		}


		// Create navmesh data storage, reading is located in
		//    ..\waypoint_generator\chunk_view.cpp
		//			method ChunkView::load()
		// and ..\..\tools\worldeditor\misc\navmesh_view.cpp
		//			method NavmeshView::populateGirth()
		// and ..\waypoint\chunk_waypoint_set_data.cpp
		//			method ChunkWaypointSetData::navmeshFactory()

		// Version
		push<int>( result, 0 );

		push<float>( result, girth );
		push<int>( result, meshNum );
		push<int>( result, edgeNum );

		for (Index j = i; j < i + meshNum; ++j)
		{
			push<float>( result, meshes[ j ].minHeight_ + SINK_MESH_OFFSET);
			push<float>( result, meshes[ j ].maxHeight_ );
			push<int>( result, (int)meshes[ j ].vertices_.size() );
		}

		for (Index j = i; j < i + meshNum; ++j)
		{
			const Mesh& mesh = meshes[ j ];

			for (Index vi = 0; vi < mesh.vertices_.size(); ++vi)
			{
				push<float>( result, mesh.vertices_[ vi ].position_.x );
				push<float>( result, mesh.vertices_[ vi ].position_.z );

				if (mesh.vertices_[ vi ].connection_ == -1 ||
					mesh.vertices_[ vi ].connection_ == 65535)
				{
					push<int>( result, mesh.vertices_[ vi ].connection_ );
				}
				else
				{
					push<int>( result, mesh.vertices_[ vi ].connection_ -
						static_cast<int>(i) );
				}
			}
		}

		// Move to next set
		i += (Index)meshNum;
	}
}

/* static */bool RecastGenerator::isPolygonInChunk( const Mesh& mesh,
												Chunk& chunk )
{
	BW::vector<MeshVertex>::size_type size = mesh.vertices_.size();
	const float CHECK_DISTANCE = 0.25f;

	for (BW::vector<MeshVertex>::size_type i = 0; i < size; ++i)
	{
		const MeshVertex& cur = mesh.vertices_[ i ];
		const MeshVertex& next = mesh.vertices_[ ( i + 1 ) % size ];

		float distance = (next.position_ - cur.position_).length();

		Vector3 normal = (next.position_ - cur.position_);
		normal.normalise();

		// Increment along edge, checking if owned by chunk
		for (float f = 0.f; f < distance; ++f)
		{
			Vector3 v = cur.position_ + normal * f;

			if (chunk.owns( v ))
			{
				return true;
			}
		}
	}

	return false;
}


void RecastGenerator::convertRecastMeshToNavgenMesh( float girth, Chunk& chunk,
										BW::vector<unsigned char>& navgenMesh )
{
	static const float BORDER_WIDTH = 0.5f;
	dtNavMesh* navmesh = dtAllocNavMesh();
	BW::vector<Mesh> meshes;

#ifdef DUMP_POLYMESH
	if ((chunk.identifier() == "0005fffdo" || // outside
		chunk.identifier() == "3a37b2e1i" || // door way
		chunk.identifier() == "4fc12c09i" || // bottom stairs
		chunk.identifier() == "29250b5ei") // second stairs
		&& girth == 0.5f )
	{
		FileIO polyone;
		char filename[512];
		sprintf(filename, "dump\\%s_poly.obj", chunk.identifier().c_str());
		if (polyone.openForWrite(filename))
		{
			duDumpPolyMeshToObj(*polyMesh_, &polyone);
		}

		FileIO detailone;
		sprintf(filename, "dump\\%s_detail.obj", chunk.identifier().c_str());
		if (detailone.openForWrite(filename))
		{
			duDumpPolyMeshDetailToObj(*polyMeshDetail_, &detailone);
		}
	}
#endif 

	dtStatus initResult = navmesh->init( &navmesh_[0],
		(int)navmesh_.size(), (dtTileFlags)0 );

	if (dtStatusFailed( initResult ))
	{
		// failed to init detour navmesh, exit
		dtFreeNavMesh( navmesh );
		return;	
	}

	// Loop through tiles and find the first valid tile
	dtMeshTile* tile = NULL;

	for (int i = 0; i < navmesh->getMaxTiles(); ++i)
	{
		// for some reason non-const version of 
		// getTile() is private, have to cast
		tile = const_cast<dtMeshTile*>(
			((const dtNavMesh*)navmesh)->getTile( i ) );
		// make sure tile is valid
		if (tile && tile->header && tile->dataSize)
		{
			// found a valid tile, break the loop
			break;
		}
	}

	if (!tile)
	{
		// no valid tiles, exit
		dtFreeNavMesh( navmesh );
		return;	
	}

	// create mesh structure, removing meshes 
	// not within this chunk
	int deletedMeshes = 0;

	// Map between old and new indexes
	//   key = mesh index prior to deleting mesh
	//   value = mesh index after deleting mesh
	BW::map<int, int> meshIndexMap;

	// Reserve enough room for each poly
	meshes.reserve( tile->header->polyCount );

	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		// Create new mesh for polygon in meshes
		meshes.resize( meshes.size() + 1 );
		Mesh& mesh = meshes.back();

		dtPoly& poly = tile->polys[ i ];

		mesh.set_ = -1; // Mesh is not a member of a set yet
		mesh.maxHeight_ = -FLT_MAX;
		mesh.minHeight_ = FLT_MAX;

		bool foundConnections = false;
		MF_ASSERT(poly.vertCount <= DT_VERTS_PER_POLYGON );
		int vertCount = poly.vertCount;

		for (int v = 0; v < vertCount; ++v)
		{
			MeshVertex mv;

			unsigned short vertIndex = poly.verts[ v ];
			MF_ASSERT( vertIndex < tile->header->vertCount );

			float* vertPos = tile->verts + vertIndex * 3;
			mv.position_.x = vertPos[0];
			mv.position_.y = vertPos[1];
			mv.position_.z = vertPos[2];

			mesh.maxHeight_ = std::max( mesh.maxHeight_, mv.position_.y );
			mesh.minHeight_ = std::min( mesh.minHeight_, mv.position_.y );

			if (poly.neis[ v ] & DT_EXT_LINK)
			{
				// Polygon edge is a portal that links to another
				//  polygon
				mv.connection_ = 65535;
				foundConnections = true;
			}
			else if (poly.neis[ v ] == 0)
			{
				// Polygon edge has no link
				mv.connection_ = -1;
			}
			else
			{
				// Neighbouring polygon
				mv.connection_ = poly.neis[ v ] - 1;
				foundConnections = true;
			}

			mesh.vertices_.push_back( mv );
		}

		// update vert count
		vertCount = static_cast<int>(mesh.vertices_.size());

		// Include the heights in the detail poly mesh
		const dtPolyDetail* pd = &tile->detailMeshes[i];
		for (int triangleIndex = 0;	triangleIndex < pd->triCount;
				++triangleIndex)
		{
			// Get the triangle indexes from the detail triangles
			const unsigned char* triangle =
				&tile->detailTris[(pd->triBase+triangleIndex)*4];

			for (int triangleVertex = 0; triangleVertex < 3;
					++triangleVertex)
			{
				// If this is a vert already included in non-detail
				// polymesh then we can ignore it, as the min/max
				// are already checked.
				if (triangle[triangleVertex] < poly.vertCount)
					continue;

				// Get the vertex from the detail poly array
				float* vertex =
					&tile->detailVerts[
						((pd->vertBase+triangle[triangleVertex]
								-poly.vertCount)*3)];

				// Adjust the min/max to include the vertex
				mesh.maxHeight_ = std::max( mesh.maxHeight_, vertex[1]);
				mesh.minHeight_ = std::min( mesh.minHeight_, vertex[1]);
			}
		}

		ChunkBoundaries& bounds = chunk.bounds();

		bool badMesh = false;

		// now check the actual boundary
		for (ChunkBoundaries::const_iterator it = bounds.begin();
			it != bounds.end() && !badMesh; it++)
		{
			BW::vector<MeshVertex> newMesh;
			const PlaneEq &curplane = (*it)->plane();
			// move plane out to allow for overlapping
			PlaneEq plane(curplane.normal(),
				curplane.d() - BORDER_WIDTH);
			newMesh.reserve( mesh.vertices_.size() );

			for (int v =0; v < vertCount; ++v)
			{
				MeshVertex& cur = mesh.vertices_[ v ];
				MeshVertex& next = mesh.vertices_[ ( v + 1 ) % vertCount ];

				// bring the point into our own space
				Vector3 curLocal = chunk.transformInverse()
							.applyPoint( cur.position_ );
				Vector3 nextLocal = chunk.transformInverse()
							.applyPoint( next.position_ );

				float nextPlaneDist = plane.distanceTo( nextLocal );
				float curPlaneDist = plane.distanceTo( curLocal );

				// Skip bad edges
				if ( nextPlaneDist < 0 && curPlaneDist < 0 )
					continue;

				// If we're in a good place then add ourselves
				if (curPlaneDist >= 0)
				{
					// keep vert
					newMesh.push_back( cur );
				}

				// Leaving good place, entering bad point, then add
				//  new point
				if ( nextPlaneDist < 0 && curPlaneDist >= 0 )
				{
					// add vert for this edge
					Vector3 dir = curLocal- nextLocal;
					dir.normalise();
					Vector3 newPos = plane.intersectRay( curLocal,
						dir );
					MeshVertex newVert;
					newVert.connection_ = next.connection_;
					newVert.position_ = chunk.transform()
											.applyPoint( newPos );

					newMesh.push_back( newVert );
				}
				else if ( nextPlaneDist >= 0 && curPlaneDist < 0 )
				{
					// add vert for this edge
					Vector3 dir = nextLocal - curLocal;
					dir.normalise();
					Vector3 newPos = plane.intersectRay( nextLocal,	dir );
					MeshVertex newVert;
					newVert.connection_ = cur.connection_;
					newVert.position_ = chunk.transform()
											.applyPoint( newPos );

					newMesh.push_back( newVert );
				}
			}

			if ( newMesh.size() < 3 )
			{
				badMesh = true;
			}
			else
			{
				mesh.vertices_ = newMesh;
				vertCount = static_cast<int>(newMesh.size());
			}
		}
		// update vert count
		// from now our mesh might contain more vertices than recast mesh
		vertCount = static_cast<int>(mesh.vertices_.size());

		// If polygon is in the chunk, we need to build index map 
		//  and mark portals as needed otherwise we must delete the
		//  polygon, as it is not part of our chunk
		if (!badMesh && isPolygonInChunk( mesh, chunk ) &&
				foundConnections)
		{
			// Update map to link between old (pre-deleted meshes)
			//  and new indexes
			meshIndexMap[ i ] = i - deletedMeshes;

			for (int i = 0; i < vertCount; ++i)
			{
				MeshVertex& cur = mesh.vertices_[ i ];
				MeshVertex& next = 
					mesh.vertices_[ ( i + 1 ) % vertCount ];

				if (!chunk.owns( cur.position_ ) &&
					!chunk.owns( next.position_ ))
				{
					Chunk* curChunk = chunk.space()->
						findChunkFromPointExact( cur.position_ );
					Chunk* nextChunk = chunk.space()->
						findChunkFromPointExact( next.position_ );

					// Current vert and next vert are not in the 
					//  current chunk and both are in the same chunk
					//  so this must be a portal.
					if (curChunk && curChunk == nextChunk)
						cur.connection_ = 65535;
				}
			}
		}
		else
		{
			// Polygon is not part of this chunk, so should be able
			//  to remove
			meshes.pop_back();
			++deletedMeshes;
		}
	}

	typedef BW::vector<Mesh>::size_type Index;

	meshIndexMap[ -1 ] = -1;
	meshIndexMap[ 65535 ] = 65535;

	for (Index i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[ i ];

		for (Index vi = 0; vi < mesh.vertices_.size(); ++vi)
		{
			if (meshIndexMap.find( 
					mesh.vertices_[ vi ].connection_ ) !=
				meshIndexMap.end())
			{
				// Update connection to point to new mesh index. 
				//  (After deleting polygons not in chunk)
				mesh.vertices_[ vi ].connection_ =
					meshIndexMap[ mesh.vertices_[ vi ].connection_ ];
			}
			else
			{
				// If no index is found to next polygon, mark vert 
				//  as no connection
				mesh.vertices_[ vi ].connection_ = -1;
			}
		}
	}

	dtFreeNavMesh( navmesh );

	buildMeshSet( meshes );

	return convertMeshSet( meshes, girth, navgenMesh );
}


/**
 *	@see ChunkProcessorManager
 */
BW::set<Chunk*> RecastGenerator::collectChunks( Chunk* primary )
{
	BW::set<Chunk*> result;

	if (primary)
	{
		result.insert( primary );

		if (primary->isOutsideChunk())
		{
			int16 gridX, gridZ;

			MF_VERIFY(primary->mapping()->gridFromChunkName(
				primary->identifier(),
				gridX,
				gridZ ));

			// Foreach neighbouring chunk, if exist, then add to result list
			for (int16 z = gridZ - 1; z <= gridZ + 1; ++z)
			{
				for (int16 x = gridX - 1; x <= gridX + 1; ++x)
				{
					BW::string chunkName = primary->mapping()->
						outsideChunkIdentifier( x, z );

					// Chunk name is empty if outside of bounds
					if (chunkName.empty())
					{
						continue;
					}

					Chunk* chunk = ChunkManager::instance()
						.findChunkByName( chunkName,
										primary->mapping() );

					// Add neighbouring chunk to result set
					result.insert( chunk );

				}
			}

			BW::set<Chunk*> copy( result );

			BW::set<Chunk*>::iterator iter;
			for (iter = copy.begin(); iter != copy.end(); ++iter)
			{
				Chunk* pChunk = *iter;

				const ChunkOverlappers::Overlappers& overlappers = 
					ChunkOverlappers::instance( *pChunk ).overlappers();

				ChunkOverlappers::Overlappers::const_iterator oIter;
				for (oIter = overlappers.begin();
					oIter != overlappers.end();
					++oIter)
				{
					ChunkOverlapperPtr pChunkOverlapper = *oIter;

					// Add Overlapper to result set
					result.insert(
						pChunkOverlapper->pOverlappingChunk() );
				}
			}
		}
		else
		{
			Chunk* overlapped[4];

			primary->collectOverlappedOutsideChunksForShell( overlapped );

			for (int i = 0; i < 4; ++i)
			{
				if (Chunk* chunk = overlapped[i])
				{
					// Continue collecting connected chunks
					BW::set<Chunk*> chunks = collectChunks( chunk );

					// Add collected chunks to final result
					result.insert( chunks.begin(), chunks.end() );
				}
			}
		}
	}

	return result;
}


CollisionSceneProviders RecastGenerator::collectTriangles( Chunk* primary )
{
	CollisionSceneProviders csps;

	// Collect all interconnected chunks
	BW::set<Chunk*> chunks = collectChunks( primary );

	for (BW::set<Chunk*>::iterator iter = chunks.begin();
		iter != chunks.end(); ++iter)
	{
		Chunk* c = *iter;

		if (!c->isBound())
			continue;

		MF_ASSERT( c->isBound() );

		BW::vector<ChunkItemPtr> items = c->staticItems();

		for (BW::vector<ChunkItemPtr>::iterator iter = items.begin();
			iter != items.end(); ++iter)
		{
			// Add get collision provider
			csps.append( (*iter)->getCollisionSceneProvider( primary ) );
		}
	}

#ifdef DUMP_COLLISION_SCENE
	if (primary->identifier() == "0005fffdo" || //outside
		primary->identifier() == "3a37b2e1i" || // door way
		primary->identifier() == "4fc12c09i" || // bottom stairs
		primary->identifier() == "29250b5ei") //
	{
		csps.dumpOBJ( "dump\\" + primary->identifier() + "_scene.obj" );
	}
#endif

	return csps;
}
BW_END_NAMESPACE
