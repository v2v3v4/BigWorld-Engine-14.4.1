#ifndef HULLTREE_HPP
#define HULLTREE_HPP

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/vectornodest.hpp"
#include "math/planeeq.hpp"
#include "math/portal2d.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the contents of a convex hull. Pointers to it
 *	are stored in a HullTree, and are returned back from some routines.
 *
 *	@see HullTree
 */
class HullContents
{
public:
	HullContents() : pNext_( NULL ) {}
	virtual ~HullContents() {}

	mutable const HullContents * pNext_;
};

typedef BW::vector< const HullContents * >	HullContentsSet;


/**
 *	This class is the border of a convex hull. It is simply a vector
 *	of plane equations.
 *
 *	@see HullTree
 */
class HullBorder : public BW::vector< class PlaneEq >
{
};


/**
 *	This class is a tree of convex hulls. The hull is defined by the
 *	planes that make it up, and a (user-derived) HullContents object.
 */
class HullTree
{
public:
	//void * operator new( size_t sz );
	//void operator delete( void * p );

	HullTree();
	~HullTree();

	void add( const HullBorder & border, const HullContents * tag );

	const HullContents * testPoint( const Vector3 & point ) const;

	void print( int depth, bool back ) const;
	long size( int depth, bool back ) const;

	/**
	 *	This class implements the traversal of a hulltrees contents.
	 */
	class Traversal
	{
	public:
		const HullContents * next();

	private:
		Traversal( const HullTree * pTree, const Vector3 & source,
				const Vector3 & extent, float radius ) :
			delta_( extent - source ),
			source_( source ),
			pPull_( NULL ),	// pullAt not initialised
			radius_( radius ),
			zeroRadius_( almostZero( radius ) )
		{
			stack_.clear();
			stack_.push_back( StackElt( pTree, -1, 0, 1 ) );
		}

		Vector3			delta_;
		const Vector3 & source_;

		const HullContentsSet * pPull_;
		unsigned int	pullAt_;

		float			radius_;
		bool			zeroRadius_;

		struct StackElt
		{
			StackElt( const HullTree * pNode, int eBack, float st, float et ) :
				pNode_( pNode ), eBack_( eBack ), st_( st ), et_( et ) {}

			const HullTree *	pNode_;
			int					eBack_;
			float				st_;
			float				et_;
		};
		static VectorNoDestructor< StackElt >	stack_;

		friend class HullTree;
	};

	friend class Traversal;

	Traversal traverse( const Vector3 & source, const Vector3 & extent,
		float radius = 0.f ) const
	{
		return Traversal( this, source, extent, radius );
	}

private:
	HullTree( const PlaneEq & plane, HullTree * & firstMarked );

	/**
	 *	Helper class to expand a Portal2D and make addPlane fater
	 */
	class Portal3D
	{
	public:
		Portal3D() { }
		Portal3D( const PlaneEq & plane, Portal2D & portal );

		typedef BW::vector<Vector3> V3Vector;
		V3Vector & points()		{ return points_; }

	private:
		V3Vector	points_;
	};

	void addPlane( const PlaneEq & plane, Portal3D & outline,
		HullTree * & firstMarked );

	PlaneEq				divider_;

	HullTree			* pFront_;
	HullTree			* pBack_;

	HullContentsSet		tagFront_;
	HullContentsSet		tagBack_;

	int					marked_;
	HullTree			* pNextMarked_;

	// values for marked_:
	// 0 => none, & 1 => back, & 2 => front
	// & 256 => not back, & 512 => not front
	// & 65536 => new (in this hull)
};

#ifdef CODE_INLINE
#include "hulltree.ipp"
#endif

BW_END_NAMESPACE

#endif // HULLTREE_HPP
