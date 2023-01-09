#ifndef _WAYPOINT_VIEW
#define _WAYPOINT_VIEW

#include "math/vector4.hpp"
#include "math/vector3.hpp"
#include "math/vector2.hpp"

#include "chunk/chunk.hpp"


BW_BEGIN_NAMESPACE

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

class IWaypointView
{
	static const int navPolySetEltSize = 16;
	static const int navPolyEltSize = 12;
	static const int navPolyEdgeEltSize = 12;

public:

	struct BSPNodeInfo
	{
		BW::vector<Vector2>	boundary_;
		float					minHeight_;
		float					maxHeight_;
		bool					internal_;
		bool					waypoint_;
	};

	virtual uint32	gridX() const = 0;
	virtual uint32 	gridZ() const = 0;
	virtual uint8 	gridMask(int x, int z) const = 0;
	virtual Vector3	gridMin() const = 0;
	virtual Vector3 gridMax() const = 0;
	virtual float	gridResolution() const = 0;
	virtual int		gridPollCount() const	{ return 0; }
	virtual bool	gridPoll( int cursor, Vector3 & pt, Vector4 & ahgts ) const
											{ return false; }

	virtual int		getBSPNodeCount() const	{ return 0; }
	virtual int		getBSPNode( int cursor, int depth, BSPNodeInfo & bni ) const
											{ return -1; }

	virtual int		getPolygonCount() const = 0;
	virtual float	getGirth(int set) const = 0;
	virtual int		getVertexCount(int polygon) const = 0;
	virtual float	getMinHeight(int polygon) const = 0;
	virtual float	getMaxHeight(int polygon) const = 0;
	virtual int		getSet(int polygon) const = 0;
	virtual int		getSets() const = 0;
	virtual void	getVertex(int polygon, int vertex, 
		Vector2& v, int& adjacency, bool & adjToAnotherChunk) const = 0;
	virtual void	setAdjacency( int polygon, int vertex, int newAdj ) { }

	virtual BW::vector<int>	findWaypoints( const Vector3&, float girth ) const = 0;

	void			writeToSection( DataSectionPtr pSection, Chunk * pChunk,
						float defaultGirth, bool removeOld = true ) const;
	void			saveOut( Chunk * pChunk, float defaultGirth,
						bool removeAllOld = true );
	BinaryPtr		asBinary( const Matrix & transformToLocal,
						float defaultGirth );

	bool			equivGirth( int polygon, float girth ) const;

	virtual const BW::string& identifier() const = 0;
	/**
	 *	Wrapper class to access a navPoly (legacy: waypoint) vertex
	 */
	class VertexRef
	{
	public:
		VertexRef( IWaypointView & view, int pindex, int vindex ) :
			view_( &view ),
			pindex_( pindex ),
			vindex_( vindex ),
			adjNavPoly_(),
			adjToAnotherChunk_()
		{
			this->getVertex();
		}

		const Vector2 & pos() const				{ return pos_; }
		const int adjNavPoly() const			{ return adjNavPoly_; }
		const bool adjToAnotherChunk() const	{ return adjToAnotherChunk_; }

		void adjNavPoly( int newAdjNavPoly )
		{
			view_->setAdjacency( pindex_, vindex_, newAdjNavPoly );
			this->getVertex();
		}

	private:
		void getVertex()
		{
			view_->getVertex( pindex_, vindex_, pos_, adjNavPoly_, adjToAnotherChunk_ );
			pos_ *= view_->gridResolution();
			Vector3 gm( view_->gridMin() );
			pos_.x += gm.x;
			pos_.y += gm.z;
		}

		IWaypointView *	view_;
		int				pindex_;
		int				vindex_;

		Vector2			pos_;
		int				adjNavPoly_;
		bool			adjToAnotherChunk_;
	};

	/**
	 *	Wrapper class to access a waypoint
	 */
	class PolygonRef
	{
	public:
		PolygonRef( IWaypointView & view, int index ) :
			view_( &view ), index_( index ) { }
		
		float minHeight() const
			{ return view_->getMinHeight( index_ ); }
		float maxHeight() const
			{ return view_->getMaxHeight( index_ ); }
		int navPolySetIndex() const
			{ return view_->getSet( index_ ); }

		uint size() const
			{ return view_->getVertexCount( index_ ); }
		VertexRef operator[]( int i ) const
			{ return VertexRef( *view_, index_, i ); }

	private:
		IWaypointView *	view_;
		int				index_;
	};
};

BW_END_NAMESPACE

#endif

