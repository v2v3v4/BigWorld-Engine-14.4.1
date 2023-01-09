#include "pch.hpp"

#include "navloc.hpp"

#include "chunk_navigator.hpp"
#include "navigator_find_result.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"

#include <sstream>


BW_BEGIN_NAMESPACE

/**
 *	Default constructor (result always invalid)
 */
NavLoc::NavLoc() :
	pSet_( NULL ),
	waypoint_( 0 ),
	point_(),
	gridSize_()
{
}


/**
 *	Constructor from a space and a point in that space's world coords.
 */
NavLoc::NavLoc( ChunkSpace * pSpace, const Vector3 & point, float girth ) :
	pSet_( NULL ),
	waypoint_( 0 ),
	point_(),
	gridSize_( pSpace->gridSize() )
{
	// To ensure that we always get the correct chunk because
	// sometimes a shell is on a chunk exactly, which causes
	// y conflicts.
	Vector3 p( point );
	p.y += 0.01f;
	Chunk * pChunk = pSpace->findChunkFromPointExact( p );
	//DEBUG_MSG( "NavLoc::NavLoc: from Space: chunk %s\n",
	//	pChunk ? pChunk->identifier().c_str() : "no chunk" );
	if (pChunk != NULL)
	{
		point_ = point;
		NavigatorFindResult res;
		if (ChunkNavigator::instance( *pChunk ).find(
				MappedVector3( point_, pChunk, MappedVector3::WORLD_SPACE ),
				girth, res ))
		{
			//DEBUG_MSG( "NavLoc::NavLoc: waypoint %d\n", res.waypoint_ );
			pSet_ = res.pSet();
			waypoint_ = res.waypoint();
		}
		//DEBUG_MSG( "NavLoc::NavLoc: point (%f,%f,%f)\n",
		//	point_.x, point_.y, point_.z );
	}
}


/**
 *	Constructor from a chunk and a point in that chunk's local coords.
 */
NavLoc::NavLoc( Chunk * pChunk, const Vector3 & point, float girth ) :
	pSet_( NULL ),
	waypoint_( 0 ),
	point_( point ),
	gridSize_( pChunk->space()->gridSize() )
{
	MF_ASSERT_DEBUG( pChunk != NULL );

	point_.y += 0.01f;

	//DEBUG_MSG( "NavLoc::NavLoc: from Chunk: chunk %s\n",
	//	pChunk ? pChunk->identifier().c_str() : "no chunk" );
	NavigatorFindResult res;
	if (ChunkNavigator::instance( *pChunk ).find(
			MappedVector3( point_, pChunk, MappedVector3::WORLD_SPACE ),
			girth, res ))
	{
		pSet_ = res.pSet();
		waypoint_ = res.waypoint();
	}
	//DEBUG_MSG( "NavLoc::NavLoc: point (%f,%f,%f)\n",
	//	point_.x, point_.y, point_.z );
}


/**
 *	Constructor from a similar navloc and a point in world coords.
 *
 *	First it tries the same waypoint, then the same waypoint set,
 *	and if that fails then it resorts to the full world point search.
 */
NavLoc::NavLoc( const NavLoc & guess, const Vector3 & point ) : 
	pSet_( NULL ),
	waypoint_( 0 ),
	point_( point ),
	gridSize_( 0.f )
{
	MF_ASSERT_DEBUG( guess.valid() );

	point_.y += 0.01f;

	//DEBUG_MSG( "NavLoc::NavLoc: Guessing...\n");
	//DEBUG_MSG( "NavLoc::NavLoc: guessing point (%f,%f,%f)\n",
	//		point_.x, point_.y, point_.z );

	//DEBUG_MSG( "NavLoc::NavLoc: guess NavLoc (%f,%f,%f) in %s id %d\n",
	//	guess.point().x, guess.point().y, guess.point().z,
	//	guess.pSet()->chunk()->identifier().c_str(), guess.waypoint());

	ChunkSpacePtr pSpace = guess.pSet()->chunk()->space();
	gridSize_ = pSpace->gridSize();
	if (guess.waypoint() == -1)
	{
		Vector3 ndPt( point_.x, point_.y - 100.f, point_.z );
		float dist = pSpace->collide( point_, ndPt, ClosestObstacle::s_default );

		if (dist > 0.f)
		{
			point_.y = point_.y - dist + 0.01f;
		}

		waypoint_ = guess.pSet()->find(
			MappedVector3( point_, guess.pSet()->chunk(), 
				MappedVector3::WORLD_SPACE ) );
	}
	else if (guess.containsProjection(point_))
	{
		waypoint_ = guess.waypoint();
	}
	else
	{
		waypoint_ = guess.pSet()->find(
			MappedVector3( point_, guess.pSet()->chunk(), 
				MappedVector3::WORLD_SPACE ) );
	}

	pSet_ = guess.pSet();

	if (waypoint_ < 0)
	{
		*this = NavLoc( pSet_->chunk()->space(), point_, pSet_->girth() );
	}

	//DEBUG_MSG( "NavLoc::NavLoc: point (%f,%f,%f)\n",
	//	point_.x, point_.y, point_.z );
}


/**
 *	Destructor
 */
NavLoc::~NavLoc()
{
}


/**
 * This method returns whether or not the lpoint is within the waypoint
 */
bool NavLoc::isWithinWP() const
{
	if (pSet_ && waypoint_ >= 0)
	{
		return pSet_->waypoint( waypoint_ ).contains( *pSet_,
			MappedVector3( point_, pSet_->chunk(), 
			MappedVector3::WORLD_SPACE ) );
	}
	else
	{
		return 0;
	}
}


/**
 * This method clips the lpoint so that it is within the waypoint.
 */
void NavLoc::clip()
{
	this->clip( point_ );
}


/**
 * This method make the input point to the same height as maxHeight
 * of waypoint
 */
void NavLoc::makeMaxHeight( Vector3 & point ) const
{
	if ( pSet_ && waypoint_ >= 0 )
	{
		WorldSpaceVector3 v( point );
		pSet_->waypoint( waypoint_ ).makeMaxHeight( pSet_->chunk(), v );
		point = v;
	}
}


/**
 * This method clips the point so that it is within the waypoint.
 */
void NavLoc::clip( Vector3 & point ) const
{
	if (pSet_ && waypoint_ >= 0)
	{
		WorldSpaceVector3 v( point );
		pSet_->waypoint( waypoint_ ).clip( *pSet_, pSet_->chunk(), v );
		point = v;
	}
}


/**
 * This method checks if the NavLoc contains the given point
 */
bool NavLoc::containsProjection( const Vector3 & point ) const
{
	if ( pSet_ == NULL || pSet_->chunk() == NULL || waypoint_ < 0 )
	{
		return false;
	}

	if ((point_ - point).lengthSquared() < 0.00001f)
	{
		return true;
	}

	return pSet_->waypoint( waypoint_ ).containsProjection( *pSet_, 
			MappedVector3( point, pSet_->chunk(), MappedVector3::WORLD_SPACE )
			);
}


/**
 * This method gives a description of the current NavLoc
 */
BW::string NavLoc::desc() const
{
	BW::stringstream ss;

	if (pSet_ && pSet_->chunk())
	{
		ss << pSet_->chunk()->identifier() << ':' << pSet_.get() << ':'
			<< this->waypoint() << ' ';

		ss << point_;

		if (waypoint_ != -1)
		{
			ss << " - ";

			const ChunkWaypoint & wp = pSet_->waypoint( waypoint_ );

			for (ChunkWaypoint::Edges::size_type i = 0; i < wp.edgeCount_; ++i)
			{
				const Vector2 & start = pSet_->vertexByIndex(
					wp.edges_[i].vertexIndex_ );

				ss << '(' << start.x << ", " << start.y << ')';

				if (i != wp.edgeCount_ - 1)
				{
					ss << ", ";
				}
			}
		}
	}
	else
	{
		ss << point_ << (this->valid() ? "" : " (invalid) ");
	}

	return ss.str();
}

BW_END_NAMESPACE

// navloc.cpp

