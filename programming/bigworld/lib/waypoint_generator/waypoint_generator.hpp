#ifndef WAYPOINT_GENERATOR_HEADER
#define WAYPOINT_GENERATOR_HEADER

#include "waypoint_flood.hpp"
#include "waypoint_view.hpp"

#include "cstdmf/stdmf.hpp"

#include "math/planeeq.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

#include "resmgr/datasection.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


#undef DEBUG_UNRECIPROCATED_ADJACENCY


BW_BEGIN_NAMESPACE

class WaypointGenerator : public IWaypointView
{
public:
	WaypointGenerator();
	virtual ~WaypointGenerator();

	void	clear();
	bool	readTGA(const char* filename);
	bool	init( int width, int height,
		const Vector3 & gridMin, float gridResolution );

	uint32	gridX() const 					{ return gridX_; }
	uint32 	gridZ() const 					{ return gridZ_; }
	uint8 	gridMask(int x, int z) const;// { return grid_[x + gridX_ * z]; }
	Vector3	gridMin() const;
	Vector3 gridMax() const;
	float	gridResolution() const			{ return gridResolution_; }

	int		gridPollCount() const			{ return gridX_*gridZ_*WaypointFlood::MAX_HEIGHTS; }
	bool	gridPoll( int cursor, Vector3 & pt, Vector4 & ahgts ) const;

	AdjGridElt *const*	adjGrids() const	{ return adjGrids_; }
	float *const*		hgtGrids() const	{ return hgtGrids_; }

	void	generate( );
	//void	determineOneWayAdjacencies( );
	void	determineEdgesAdjacentToOtherChunks( Chunk * pChunk, IPhysics* physics ); 
	void	extendThroughUnboundPortals( Chunk * pChunk );
	void	streamline();
	void	calculateSetMembership( const BW::vector<Vector3> & frontierPts,
									const BW::string & chunkName );

	int		getBSPNodeCount() const;
	int		getBSPNode( int cursor, int depth, BSPNodeInfo & bni ) const;

	int		getPolygonCount() const;
	float	getGirth( int set ) const		{ return 0.f; }
	int		getVertexCount(int polygon) const;
	float	getMinHeight(int polygon) const;
	float	getMaxHeight(int polygon) const;
	int		getSet(int polygon) const;
	int		getSets() const;
	void	getVertex(int polygon, int vertex, Vector2& v, int& adjacency,
		bool & adjToAnotherChunk) const;
	void	setAdjacency( int polygon, int vertex, int newAdj );

	BW::vector<int>	findWaypoints( const Vector3& v, float girth ) const;

	struct IProgress
	{
		virtual void onProgress(const wchar_t* phase, int current) = 0;
	};

	virtual const BW::string& identifier() const	{ return identifier_; }

	void	setProgressMonitor(IProgress* pProgress) { pProgress_ = pProgress; }

private:
	
	struct BSPNode
	{
		float	borderOffset[8];
		float	splitOffset;
		int		splitNormal;
		int		parent;
		int		front;
		int		back;
		bool	waypoint;
		int		waypointIndex;
		Vector2	centre;
		float	minHeight;
		float	maxHeight;

		mutable int		baseX;
		mutable int		baseZ;
		mutable int		width;
		mutable BW::vector<BOOL> pointInNodeFlags;

		void	updatePointInNodeFlags() const;
		BOOL	pointInNode( int x, int z ) const;

		void	calcCentre();
		bool	pointInNode(const Vector2&) const;
	};

	struct SplitDef
	{
		static const int VERTICAL_SPLIT = 8;
		int 	normal;
		float 	value;
		union 
		{
			float position[2];
			float heights[2];
		};
	};

	struct PointDef
	{
		int		angle;
		float	offset;
		float	t;
		float	minHeight;
		float	maxHeight;

		bool operator<(const PointDef& v) const;
		bool operator==(const PointDef& v) const;
		bool nearlySame(const PointDef& v) const;
	};

	struct VertexDef
	{
		VertexDef() :
			pos( Vector2::ZERO ),
			adjNavPoly( 0 ),
			adjToAnotherChunk(),
			angles( 0 )
		{}

		Vector2		pos;
		int			adjNavPoly;
		bool		adjToAnotherChunk;
		int			angles;
	};
	
	struct PolygonDef
	{
		PolygonDef() :
			minHeight( 0.f ), maxHeight( 0.f ), set( 0 ) { }

		BW::vector<VertexDef>		vertices;
		float						minHeight;
		float						maxHeight;
		int							set;

		bool ptNearEnough( const Vector3 & pt ) const;

#ifdef DEBUG_UNRECIPROCATED_ADJACENCY
		// this is for debugging purposes only
		int							originalId;
#endif
	};
	
	struct EdgeDef
	{
		Vector2	from;
		Vector2 to;
		int		id;

		bool operator<(const EdgeDef& v) const;
		bool operator==(const EdgeDef& v) const;
	};
	
	struct ChunkDef
	{
		BW::string				id;
		BW::vector<PlaneEq>	planes;
	};

	AdjGridElt *			adjGrids_[WaypointFlood::MAX_HEIGHTS];
	float *					hgtGrids_[WaypointFlood::MAX_HEIGHTS];
	unsigned int			gridX_;
	unsigned int			gridZ_;
	Vector3					gridMin_;
	float					gridResolution_;
	IProgress*				pProgress_;

	BW::vector<BSPNode>	bsp_;
	BW::vector<PolygonDef>	polygons_;
	BW::set<PointDef>		points_;
	BW::set<EdgeDef>		edges_;
	int						sets_;
	BW::string				identifier_;
	
	Vector3 toVector3( const Vector2 & v, const PolygonDef & polygon,
					 Chunk * pChunk ) const;

	bool	nodesSame( const int index1, const int index2 );
	void 	initBSP();
	void	processNode( BW::vector<int> & indexStack );
	bool	findDispoints( const BSPNode & frontNode );
	void 	calcSplitValue( const BSPNode &, SplitDef & );
	void	refineNodeEdges( BSPNode* node ) const;
	void	reduceNodeHeight( BSPNode & node );
	std::pair<float,int> averageHeight(const BSPNode &);
	void	doBestHorizontalSplit( int index, BW::vector<int> & indexStack );
	bool	splitNode(int, const SplitDef&, bool replace = false);
	bool	calcEdge(int, int, PointDef&, PointDef&) const;
	void	generatePoints();
	void	generatePolygons();
	void	generateAdjacencies();
	void	invertPoint(PointDef&) const;
	void	pointToVertex(const PointDef&, Vector2&) const;
	bool	isConvexJoint(int a1, int a2, int b) const;
	int		findPolygonVertex(int, const Vector2&) const;
	bool	joinPolygon(int);
	void	joinPolygons();
	void	fixupAdjacencies(int, int, int);

};

BW_END_NAMESPACE

#endif

