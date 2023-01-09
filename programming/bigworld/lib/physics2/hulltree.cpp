#include "pch.hpp"
#include "hulltree.hpp"

#ifndef CODE_INLINE
#include "hulltree.ipp"
#endif

#include "math/lineeq.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dprintf.hpp"

DECLARE_DEBUG_COMPONENT2( "Physics", 0 )

BW_BEGIN_NAMESPACE

//#define HULLTREE_TIMING

// -----------------------------------------------------------------------------
// Section: HullTree
// -----------------------------------------------------------------------------

/*
static char * pFreeHullTrees = NULL;
static const int HULLTREES_AT_ONCE = 65536;

void * HullTree::operator new( size_t sz )
{
	char * ret;

	// do we have any free ones?
	if (pFreeHullTrees)
	{
		ret = pFreeHullTrees;
		pFreeHullTrees = *(char**)pFreeHullTrees;
		return ret;
	}

	// ok, make some then
	ret = (char*)bw_malloc( sizeof(HullTree) * HULLTREES_AT_ONCE );
	HullTree * pBlank = (HullTree*)ret;
	for (int i = 0; i < HULLTREES_AT_ONCE-1; i++, pBlank++)
	{
		*(char**)pBlank = (char*)(pBlank+1);
	}
	*(char**)pBlank = NULL;

	// and return the first of those
	pFreeHullTrees = ret + sizeof(HullTree);
	return ret;
}

void HullTree::operator delete( void * p )
{
	*(char**)p = pFreeHullTrees;
	pFreeHullTrees = (char*)p;
}
*/

/**
 *	Default constructor.
 */
HullTree::HullTree() :
	divider_( Vector3(0,0,0), 0 ),
	pFront_( NULL ),
	pBack_( NULL ),
	marked_( 0 ),
	pNextMarked_( NULL )
{
}


/**
 *	Internal constructor. New planes are always marked on the front.
 */
HullTree::HullTree( const PlaneEq & plane, HullTree * & firstMarked ) :
	divider_( plane ),
	pFront_( NULL ),
	pBack_( NULL ),
	marked_( 2 | 65536 ),
	pNextMarked_( firstMarked )
{
	firstMarked = this;
}


/**
 *	Destructor
 */
HullTree::~HullTree()
{
	bw_safe_delete(pFront_);
	bw_safe_delete(pBack_);
}

#ifdef HULLTREE_TIMING
	uint64 g_hullTreeAddTime, g_hullTreePlaneTime, g_hullTreeMarkTime;

	#define HTT_EXPR( x ) x
#else
	#define HTT_EXPR( x )
#endif

/**
 *	This method adds a new hull to the tree
 */
void HullTree::add( const HullBorder & border, const HullContents * tag )
{
	HTT_EXPR( uint64 tt = timestamp(); )

	HullTree * firstMark = (HullTree*)1;

	HullBorder::const_iterator bend = border.end();
	for (HullBorder::const_iterator it = border.begin(); it != bend; it++)
	{
		Portal2D outline;

		// calculate the outline polygon of this plane
		BW::vector< LineEq >	lines;

		//  first project the other polygons into lines in the plane
		for (HullBorder::const_iterator bt = border.begin(); bt != bend; bt++)
		{
			if (bt == it) continue;

			LineEq aline = it->intersect( *bt );

			// only add it if it's not parallel (=> facing plane since convex)
			if (aline.normal() != Vector2::zero())
				lines.push_back( aline );
		}

		//  now intersect all the lines to form the outline
		for (unsigned int i = 0; i < lines.size(); i++)
		{
			float minDist = -1000000.f;
			float maxDist =  1000000.f;
			for (unsigned int j = 0; j < lines.size(); j++)
			{
				if (j == i) continue;

				// if it's parallel then deal with it specially
				if (lines[i].isParallel( lines[j] ))
				{
					// if it's a facing line then ignore it
					if (lines[i].normal().dotProduct( lines[j].normal() ) < 0.f)
						continue;
					// if it's outside us then ignore it too
					if (lines[i].isInFrontOf( lines[j].param( 0.f ) ))
						continue;
					// okay we are completely cut off by this line,
					// so we do not feature as part of the outline
					minDist = 1;
					maxDist = -1;
					break;
				}

				float iDist = lines[i].intersect( lines[j] );

				// see what side we intersect on
				if (lines[i].isMinCutter( lines[j] ))
				{
					if (minDist < iDist)
					{
						minDist = iDist;
					}
				}
				else
				{
					if (maxDist > iDist)
					{
						maxDist = iDist;
					}
				}
			}

			// make sure there's an edge between these two planes
			if (minDist >= maxDist)	continue;

			//Vector2 lineBeg = lines[i].param( minDist );
			Vector2 lineEnd = lines[i].param( maxDist );
			// add them to the outline (but in the right order!)
			outline.addPoint( lineEnd );
		}

		HTT_EXPR( uint64 pt = timestamp(); )

		// add the plane
		Portal3D outline3D( *it, outline );
#if 0
		WorldPolygon worldPoly;
		worldPoly.push_back( it->param( Vector2( 100*1000, 100*1000 ) ) );
		worldPoly.push_back( it->param( Vector2( 100*1000,-100*1000 ) ) );
		worldPoly.push_back( it->param( Vector2(-100*1000,-100*1000 ) ) );
		worldPoly.push_back( it->param( Vector2(-100*1000, 100*1000 ) ) );

		// this method seems to have precision problems...
		for (HullBorder::const_iterator bt = border.begin(); bt != bend; bt++)
		{
			if (bt == it) continue;
			worldPoly.chop( *bt );
		}

		Portal2D dummy;
		Portal3D outline3D( *it, dummy );
		outline3D.points() = worldPoly;
#endif
#if 0
		// check it
		dprintf( "Outline for plane (%f,%f,%f) > %f  (with %d points):\n",
			it->normal().x, it->normal().y, it->normal().z, it->d(),
			outline3D.points().size() );
		for (uint i = 0; i < outline3D.points().size(); i++)
		{
			const Vector3 & pt = outline3D.points()[i];
			dprintf( "  point (%f,%f,%f)\n", pt.x, pt.y, pt.z );
			for (HullBorder::const_iterator bt = border.begin(); bt != bend; bt++)
			{
				if (bt == it) continue;
				float paway = bt->distanceTo( pt );
				if (paway <= -0.01f)
				{
					dprintf( "  problem with plane (%f,%f,%f) > %f  (dist %f):\n",
						bt->normal().x, bt->normal().y, bt->normal().z, bt->d(), paway );
					MF_ASSERT( paway > -0.01f );
				}
			}
		}
#endif
		this->addPlane( *it, outline3D, firstMark );

		HTT_EXPR( g_hullTreePlaneTime += timestamp() - pt; )

		//dprintf( "After step %d:\n", it - border.begin() );
		//this->print(0,false);
	}

	HTT_EXPR( uint64 mt = timestamp(); )

	// go through all the marks and tag them
	int nBack = 0;
	int nFront = 0;
	HullTree * pNM;
	for (HullTree * walk = firstMark; walk != (HullTree *)1; walk = pNM )
	{
		// I can't see how you'd have both conditions, but this is neater

		// see if the back is marked
		if (walk->marked_ & 1)
		{
			walk->tagBack_.push_back( tag );
			nBack++;
		}

		// see if the front is marked
		if (walk->marked_ & 2)
		{
			walk->tagFront_.push_back( tag );
			nFront++;
		}

		pNM = walk->pNextMarked_;

		walk->marked_ = 0;
		walk->pNextMarked_ = NULL;
	}

	HTT_EXPR( g_hullTreeMarkTime += timestamp() - mt; )

	HTT_EXPR( g_hullTreeAddTime += timestamp() - tt; )

	//dprintf( "Added hull: %d back tags, %d front tags\n", nBack, nFront );
	//this->print( 0, false );
}


/**
 *	This method adds one plane of a new hull to the tree
 */
void HullTree::addPlane( const PlaneEq & plane, Portal3D & outline,
	HullTree * & firstMarked )
{
	bool anySide[2] = { false, false };

	// see which side of the current plane the outline lies
	if (divider_.normal() != Vector3::zero())
	{
		for (unsigned int i = 0; i < outline.points().size(); i++)
		{
			//Vector3 point = plane.param( outline.points()[i] );
			const Vector3 & point = outline.points()[i];
			float d = divider_.distanceTo( point );
			if		(d >  0.01f) anySide[1] = true;
			else if	(d < -0.01f) anySide[0] = true;
		}
	}
	else
	{
		*this = HullTree( plane, firstMarked );
		firstMarked = this;	// constructor sets it to temporary
		return;
	}

	// we've looked at this plane so add it to the marked list
	if (pNextMarked_ == NULL)
	{
		pNextMarked_ = firstMarked;
		firstMarked = this;
	}

	// if it's on top of us, we can just mark it
	if (!(anySide[0] | anySide[1]))
	{
		bool sameDir = (divider_.normal().dotProduct( plane.normal() ) > 0.f);

		//dprintf( "Plane (%f,%f,%f) > %f is on top of (%f,%f,%f) > %f, sameDir %d\n",
		//	plane.normal().x, plane.normal().y, plane.normal().z, plane.d(),
		//	divider_.normal().x, divider_.normal().y, divider_.normal().z, divider_.d(),
		//	sameDir );

		// mark it if we're allowed to mark this direction
		//  (i.e. no child already has traversed it)
		if (!(marked_ & (256 << int(sameDir))))
		{
			marked_ |= (1 << int(sameDir));	// | 1 for back, | 2 for front
		}

		// if this is our of our edge planes,
		// then we should never consider the other side
		marked_ &= ~(1 << int(!sameDir));
		marked_ |= (256 << int(!sameDir));

		marked_ |= 65536;

		return;	// this return is assumed by later code
	}

	Portal3D inFront;
	Portal3D outBack;
//	Portal2D * outin[2] = { &outBack, &inFront };
	if (anySide[0] && anySide[1])
	{
		inFront = outline;
		outBack = outline;
		/*
		// split into inFront and outBack
		dprintf( "Splitting plane (%f,%f,%f) > %f\n",
			plane.normal().x, plane.normal().y, plane.normal().z, plane.d() );

		const V2Vector & opts = outline.points();

		// go through the list of points, and add them to whatever side they
		//  belong on. also record the minimum and maximum points along the
		//  fine line on each side

		LineEq fine = plane.intersect( divider_ );
		// [ out, in ] [ max, min ]
		float fineVals[2][2] = { { -1000000.f, 1000000.f } , { -1000000.f, 1000000.f } };
		int fineIdxs[2][2] = { { -1, -1 }, { -1, -1 } };

		for (int i = 0; i < opts.size(); i++)
		{
			const Vector2 & cand = opts[i];
			bool isInside = fine.isInFrontOf( cand );
			outin[ isInside ]->addPoint( cand );

			float t = fine.project( cand );
			if (t > fineVals[isInside][0])
			{
				fineVals[isInside][0] = t;
				fineIdxs[isInside][0] = i;
			}
			if (t < fineVals[isInside][1])
			{
				fineVals[isInside][1] = t;
				fineIdxs[isInside][1] = i;
			}

			//Vector3 point = plane->param( outline.points()[i] );
			//dprintf( " An outline point is (%f,%f,%f)\n",
			//	point.x, point.y, point.z );
		}

		// now add in the points at the intersection of the min and max points
		LineEq minEdge( opts[fineIdxs[1][0]], opts[fineIdxs[0][0]] );
		Vector2 minNewPt = fine.param( fine.intersect( minEdge ) );
		outBack.addPoint( minNewPt );
		inFront.addPoint( minNewPt );

		LineEq maxEdge( opts[fineIdxs[0][1]], opts[fineIdxs[1][1]] );
		Vector2 maxNewPt = fine.param( fine.intersect( maxEdge ) );
		outBack.addPoint( maxNewPt );
		inFront.addPoint( maxNewPt );
		*/
	}
	else if (!(marked_ & 65536))
	{
		// if we are one of the boundary planes of the hull being added,
		// then don't do what's below

		// we get here if we're not traversing both sides. we want
		// to find out if the side we're not traversing might
		// still be in the hull we're adding.

		// if we ever traverse through the other side, then it isn't,
		// but that's handled below (the mark is taken from it when
		// traversing it)

		// so what we do is find out if the plane we're adding is pointing
		// towards the divider. i.e., we're pointing in the same direction
		// and this plane is behind the divider, or we're pointing in
		// opposite directions, and this plane is in front of the divider.

		// if this is the case, then we set the mark on the side we're
		// not adding ourselves to, unless it has already been marked
		// negatively by another plane traversing through it. If this
		// is not the case, then we know that that side cannot be wholly
		// within our hull, and we mark it negatively ourselves.

		int sideWeAreNotOn = int(anySide[0]);
		// sideWeAreNotOn is 1 if we're adding to the back,
		// and 0 if we're adding to the front.
		// '256 << 0' means 'don't add tag to back'
		// '1 << 0' means 'add tag to back'
		// '256 << 1' means 'don't add tag to front'
		// '1 << 1' means 'add tag to front'

		bool pointingSameWay = divider_.normal().dotProduct( plane.normal() ) > 0.f;

		if (pointingSameWay ^ anySide[1])
		{
			if (!(marked_ & (256 << sideWeAreNotOn)))
				marked_ |= (1 << sideWeAreNotOn);
		}
		else
		{
			marked_ &= ~(1 << sideWeAreNotOn);
			marked_ |= (256 << sideWeAreNotOn);
		}
	}


	if (anySide[0]) // some out back
	{
		marked_ &= ~1;	// clear the back mark
		marked_ |= 256;	// make sure no-one sets it

		Portal3D & clipped = anySide[1] ? outBack : outline;

		if (pBack_ != NULL)
			pBack_ ->addPlane( plane, clipped, firstMarked );
		else
			pBack_  = new HullTree( plane, firstMarked );
	}
	if (anySide[1])	// some in front
	{
		marked_ &= ~2;	// clear the front mark
		marked_ |= 512;	// make sure no-one sets it

		Portal3D & clipped = anySide[0] ? inFront : outline;

		if (pFront_ != NULL)
			pFront_->addPlane( plane, clipped, firstMarked );
		else
			pFront_ = new HullTree( plane, firstMarked );
	}
}


/**
 *	Get all the hulls that the given point is in.
 *
 *	Assumes that the 'tags' vector is cleared to begin with
 */
const HullContents * HullTree::testPoint( const Vector3 & point ) const
{
	const HullContents * tags = NULL;

	int outBack;
	for (const HullTree * pLook = this;
		pLook != NULL;
		pLook = (&pLook->pFront_)[outBack])
	{
		outBack = !pLook->divider_.isInFrontOf( point );

		const HullContentsSet & ownTag = (&pLook->tagFront_)[outBack];

		for (HullContentsSet::const_iterator it = ownTag.begin();
			it != ownTag.end();
			it++)
		{
			const HullContents * pHC = *it;
			pHC->pNext_ = tags;
			tags = pHC;
		}
	}

	return tags;
}


void HullTree::print( int depth, bool back ) const
{
	BW::string tabs( depth, '\t' );
	dprintf( "%s", tabs.c_str() );
	dprintf( "%c Plane (%f,%f,%f) > %f", back ? 'B':'F',
		divider_.normal().x, divider_.normal().y, divider_.normal().z, divider_.d() );
	if (!tagFront_.empty()) dprintf( " Ftag %" PRIzu "", tagFront_.size() );
	if (!tagBack_ .empty()) dprintf( " Btag %" PRIzu "", tagBack_ .size() );
	if (marked_ & 1 ) dprintf( " BMARK" );
	if (marked_ & 2 ) dprintf( " FMARK" );
	if (marked_ & 256 ) dprintf( " !BMARK" );
	if (marked_ & 512 ) dprintf( " !FMARK" );
	if (marked_ & 65536 ) dprintf( " NEW" );
	dprintf( "\n" );
	if (pFront_ != NULL) pFront_->print( depth+1, false );
	if (pBack_ != NULL)  pBack_ ->print( depth+1, true );
}


/**
 *	Get the size of this hull tree and all its child nodes
 */
long HullTree::size( int depth, bool /*back*/ ) const
{
	long sz = sizeof( HullTree ) + sizeof( HullContents * ) *
		static_cast<long>( (tagFront_.capacity() + tagBack_.capacity()) );
	if (pFront_ != NULL) sz += pFront_->size( depth+1, false );
	if (pBack_  != NULL) sz += pBack_ ->size( depth+1, false );
	return sz;
}


// -----------------------------------------------------------------------------
// Section: HullTree::Traversal
// -----------------------------------------------------------------------------

/// static initialiser for the hulltree traversal stack
VectorNoDestructor< HullTree::Traversal::StackElt >	HullTree::Traversal::stack_;

/**
 *	This method returns the next hit in this traversal of the hull tree
 */
const HullContents * HullTree::Traversal::next()
{
	// see if we're just distributing a hull contents set
	if (pPull_ != NULL)
	{
		if (pullAt_ < pPull_->size())
		{
			return (*pPull_)[pullAt_++];
		}
		pPull_ = NULL;
	}

	float	sDist, eDist;
	int		sBack, eBack;

	// loop until we run out of things on the stack
	//  (or find a tag to return)
	while (!stack_.empty())
	{
		// ok, pull the top tree off the stack
		StackElt cur = stack_.back();

		// default / starting values for selected line interval
		sDist = cur.st_;
		eDist = cur.et_;

		// if we've already done the source side, only look at the extent side
		if (cur.eBack_ != -1)
		{
			sBack = cur.eBack_;
			eBack = cur.eBack_;

			stack_.pop_back();
		}
		// otherwise see which sides of it the line segment is on
		else
		{
			const PlaneEq & pe = cur.pNode_->divider_;
			float sOff = pe.distanceTo( source_ + sDist * delta_ );
			float eOff = pe.distanceTo( source_ + eDist * delta_ );
			if (zeroRadius_)
			{
				sBack = int(sOff < 0.f);
				eBack = int(eOff < 0.f);
			}
			else
			{
				bool sOff_l_nr = sOff < -radius_, sOff_l_pr = sOff < radius_;
				bool eOff_l_nr = eOff < -radius_, eOff_l_pr = eOff < radius_;
				bool sOff_l_eOff = sOff < eOff;
				sBack = int(sOff_l_nr | (sOff_l_pr &  sOff_l_eOff));
				eBack = int(eOff_l_nr | (eOff_l_pr & !sOff_l_eOff));
			}

			// if we're on different sides, push the extent side and come back
			//  to it later, flagging it as having already done the source side.
			if (sBack != eBack)
			{
				if (zeroRadius_)
				{
					// but first, find the intersection of the line with the
					//  plane and only test that part in the other side
					eDist = pe.intersectRayHalf( source_,
						pe.normal().dotProduct( delta_ ) );

					// re-use the existing stack element
					stack_.back().eBack_ = eBack;
					stack_.back().st_ = eDist;
				}
				else
				{
					// unless we have a radius, when we really should
					// find eDist as above and add to it the radius
					// of the cylinder projected onto the plane,
					// and also worry about parallel lines, but I
					// can't be bothered for now.
					stack_.back().eBack_ = eBack;
				}
			}
			else stack_.pop_back();
		}

		// if we get here then we're either on the same side,
		// or we're looking at the source side first,
		// or we've already done the source side and are now doing the
		//  extent side (in which case sBack is set to what eBack was)

		// put the node of the side we're looking at on the stack if it's there
		const HullTree * pDown = (&cur.pNode_->pFront_)[sBack];
		if (pDown != NULL)
			stack_.push_back( StackElt( pDown, -1, sDist, eDist ) );

		// pull out any tags that it might have
		pPull_ = (&cur.pNode_->tagFront_) + sBack;
		if (!pPull_->empty())
		{
			pullAt_ = 1;
			return pPull_->front();
		}
	}

	// ok, stack is empty - that's it then
	return NULL;
}


// -----------------------------------------------------------------------------
// Section: HullTree::Portal3D
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
HullTree::Portal3D::Portal3D( const PlaneEq & plane, Portal2D & portal )
{
	for (unsigned int i = 0; i < portal.points().size(); i++)
	{
		points_.push_back( plane.param( portal.points()[i] ) );
	}
}

BW_END_NAMESPACE

// hulltree.cpp
