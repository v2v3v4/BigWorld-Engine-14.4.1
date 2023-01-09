#ifndef _CHUNK_VIEW_HEADER
#define _CHUNK_VIEW_HEADER

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"

#include "waypoint_view.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class ChunkView : public IWaypointView
{
	static const int navPolySetEltSize = 16;
	static const int navPolyEltSize = 12;
	static const int navPolyEdgeEltSize = 12;

public:
	ChunkView();
	~ChunkView();

	void clear();
	bool load( Chunk * pChunk );

	uint32	gridX() const;
	uint32 	gridZ() const;
	uint8 	gridMask(int x, int z) const;
	Vector3	gridMin() const;
	Vector3 gridMax() const;
	float	gridResolution() const;

	int		getPolygonCount() const;
	int		getBSPNodeCount() const;
	float	getGirth( int set ) const;
	int		getVertexCount(int polygon) const;
	float	getMinHeight( int polygon ) const;
	float	getMaxHeight( int polygon ) const;
	int		getSet( int polygon ) const;
	int		getSets() const;
	void	getVertex( int polygon, int vertex, 
		Vector2& v, int& adjacency, bool & adjToAnotherChunk ) const;
	void	setAdjacency( int polygon, int vertex, int newAdj );

	BW::vector<int>	findWaypoints( const Vector3&, float girth ) const;

	virtual const BW::string& identifier() const { return identifier_; }

private:
	Vector3		gridMin_;
	Vector3		gridMax_;

	struct VertexDef
	{
		VertexDef() :
			adjacentID(),
			adjToAnotherChunk()
		{
		}

		Vector2 position;
		int		adjacentID;
		bool	adjToAnotherChunk;
	};

	struct PolygonDef
	{
		PolygonDef() :
			minHeight(),
			maxHeight(),
			set()
		{
		}

		float minHeight;
		float maxHeight;
		BW::vector<VertexDef> vertices;
		int set;
	};

	struct SetDef
	{
		SetDef() : girth( 0.f ) { }
		float girth;
	};

	BW::vector<PolygonDef> polygons_;
	BW::vector<SetDef> sets_;
	int		nextID_;
	bool	minMaxValid_;

	BW::string identifier_;

	void parseNavPoly( DataSectionPtr pSect, int set, Chunk * pChunk = NULL );
};

BW_END_NAMESPACE

#endif

