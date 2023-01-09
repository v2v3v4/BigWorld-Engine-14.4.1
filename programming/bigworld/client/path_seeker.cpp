
#include "pch.hpp"

#include "entity.hpp"
#include "path_seeker.hpp"

#include "math/angle.hpp"
#include "math/vector3.hpp"
#include "motion_constants.hpp"

#include "moo/debug_draw.hpp"

#include <algorithm>


BW_BEGIN_NAMESPACE

/// 2cm - prevent divide by 0.
const float PathSeeker::MIN_DISTANCE_TO_POINT = 0.02f;

/// 5m - for when we get stuck, we can be a little off the desired point on
/// the path. And be a bit generous for slopes etc.
const float PathSeeker::MAX_DISTANCE_TO_POINT = 5.0f;

bool PathSeeker::debugDrawPaths = false;

/**
 *	Gives the seeker a queue of points to follow.
 *	The first point should be at the player's current position.
 *	The points are expected to be dropped to the ground.
 *	@param pathPoints the points to seek along.
 *	@param yawAtEnd the yaw to turn to at the end of the path.
 */
void PathSeeker::setPath( const PathPoints& pathPoints, float yawAtEnd )
{
	BW_GUARD;

	// -- Invalidate the path that was using old queue of points
	t_ = 0.0f;

	// -- Delete old points
	pathQueue_.clear();

	// -- Copy new points to queue
	// Create paths from points
	PathPoints::const_iterator startPointItr = pathPoints.begin();
	PathPoints::const_iterator endPointItr = startPointItr + 1;
	for (;
		(startPointItr != pathPoints.end()) && (endPointItr != pathPoints.end());
		++startPointItr, ++endPointItr)
	{
		const Vector3& end = *endPointItr;

		// Set start y to end y
		// The path _must_ be flat so that the player's speed does not slow
		// down as it reaches the end, because the player is only following it on
		// X/Z.
		// eg. -> x
		//    |
		//    v y
		// Start - - - End  (t moves at constant speed)
		//  Start
		//        \
		//          \
		//            \ End (t slows down at end)
		const Vector3 start( (*startPointItr).x, end.y, (*startPointItr).z );

		// Create path to point
		Path path( start, end );

		// Path too short
		const float pathLength = path.length();
		if (pathLength <= MIN_DISTANCE_TO_POINT)
		{
			//DEBUG_MSG( "Path too short %f\n", pathLength );
		}
		else
		{
			pathQueue_.push_back( path );
		}
	}

	// -- Set yaw
	yawAtEnd_ = yawAtEnd;
}


/**
 *	This method performs one tick of a seeking movement, which seeks along a
 *	path.
 *
 *	@param	mc			MotionConstants information structure
 *	@param	useVelocity	The velocity at which to move
 *	@param	dTime		delta time.
 *	@param	outNewHeadYaw	[out]Resultant head yaw
 *	@param	outNewPos		[out]Resultant position
 *	@param	callback	the callback for when seeking is done.
 */
void PathSeeker::update( const MotionConstants& mc,
	const Vector3& useVelocity,
	float dTime,
	Angle& outNewHeadYaw,
	Vector3& outNewPos,
	SeekListener& callback )
{
	BW_GUARD;

	// -- Move towards next point
	this->updateParameter( useVelocity, dTime );
	this->moveAlongPath( mc, outNewPos, callback );
	this->turnToPath( mc, outNewHeadYaw );

	// -- No more points in path - stop seeking
	if (this->isFinished())
	{
		//DEBUG_MSG( "No more points\n" );
		// Finished successfully, set end yaw
		outNewHeadYaw = yawAtEnd_ - mc.modelYaw;
		this->cancel( true, callback );
		return;
	}

	/*const float distanceMoved = (oldPos - newPos).length();
	const float actualSpeed = distanceMoved / dTime;
	const float desiredSpeed = useVelocity.length();
	DEBUG_MSG( "Moved %fm in %fs speed %f desired %f\n",
		distanceMoved, dTime, actualSpeed, desiredSpeed );

	if ( (actualSpeed < (desiredSpeed - 0.01f)) ||
		 (actualSpeed > (desiredSpeed + 0.01f)) )
	{
		DEBUG_MSG( "Speed is slow\n" );
	}*/

	this->drawDebugPath( mc, outNewPos );
	this->drawDebugYaw( mc, outNewPos );
}


/**
 *	Draw the path between nodes for debugging.
 *	@param mc motion constants containing the yaw that the model is at.
 *	@param newPos the new position.
 */
void PathSeeker::drawDebugPath( const MotionConstants& mc,
	const Vector3& newPos ) const
{
	BW_GUARD;

	if (PathSeeker::debugDrawPaths && !this->isFinished())
	{
		// Do not have points for next path
		PathQueue::const_iterator pathItr = pathQueue_.begin();
		const Path* path;
		const Path* lastPath;

		// First segment red
		{
			path = &(*pathItr);
			Vector3 start( path->getStart() );
			Vector3 end( path->getEnd() );
			start.y += mc.scrambleHeight;
			end.y += mc.scrambleHeight;
			DebugDraw::arrowAdd( start, end, 0xaaffaaaa );

			lastPath = &(*pathItr);
			++pathItr;
		}

		// Second segment green
		if (pathItr != pathQueue_.end())
		{
			path = &(*pathItr);
			Vector3 start( lastPath->getEnd() );
			Vector3 end( path->getEnd() );
			start.y += mc.scrambleHeight;
			end.y += mc.scrambleHeight;
			DebugDraw::arrowAdd( start, end, 0xaaaaffaa );

			lastPath = &(*pathItr);
			++pathItr;
		}

		// Others white
		while (pathItr != pathQueue_.end())
		{
			path = &(*pathItr);
			Vector3 start( lastPath->getEnd() );
			Vector3 end( path->getEnd() );
			start.y += mc.scrambleHeight;
			end.y += mc.scrambleHeight;
			DebugDraw::arrowAdd( start, end, 0xaaaaaaaa );

			lastPath = &(*pathItr);
			++pathItr;
		}

		// Draw player to goal, white
		path = &pathQueue_.front();
		const Vector3& start = newPos;
		Vector3 end = path->getPointAt( t_ );
		end.y += mc.scrambleHeight;
		DebugDraw::arrowAdd( start, end, 0xffffffff );
	}
}

/**
 *	Draw the yaw for debugging.
 *	@param mc motion constants containing the yaw that the model is at.
 *	@param newPos the new position.
 */
void PathSeeker::drawDebugYaw( const MotionConstants& mc,
	const Vector3& newPos ) const
{
	BW_GUARD;

	if (PathSeeker::debugDrawPaths)
	{
		// Model/body yaw, blue
		const float length = 1.0f;
		{
			const float yaw = mc.modelYaw;
			const Vector3 start( newPos );
			Vector3 end;

			end.x = sin( yaw );
			end.y = 0.0f;
			end.z = cos( yaw );

			end.normalise();
			end *= length;
			end += start;

			DebugDraw::arrowAdd( start, end, 0xaaaaaaff );
		}
	}
}


/**
 *	Get the path from the current node to the next node.
 *	@param currentNode the current node.
 *	@param currentT the distance along the path, defaults to 0.
 */
void PathSeeker::getNextPath( const Vector3& currentNode, float currentT )
{
	BW_GUARD;

	// Do not have points for next path
	if (this->isFinished())
	{
		return;
	}

	// Get path to next point
	// Pop off next point
	pathQueue_.pop_front();
	t_ = currentT;
}


/**
 *	Update 't', for the position along the line.
 *	@param useVelocity the velocity to move along the path.
 *	@param dTime the change in time since last frame.
 */
void PathSeeker::updateParameter( const Vector3& velocity,
	float dTime )
{
	BW_GUARD;

	// No path to move along
	if (this->isFinished())
	{
		return;
	}

	// Update position along path
	const float velocityLength = velocity.length();
	const Path& path = pathQueue_.front();
	const float pathLength = path.length();
	const float maxDisplacement = velocityLength * dTime;

	// Prevent divide by 0, paths that are too short should be rejected
	MF_ASSERT( pathLength > 0.0f );
	t_ += (maxDisplacement / pathLength);
}


/**
 *	Move the character's position along the path.
 *	@param mc motion constants.
 *	@param outNewPos the position to move.
 *	@param callback the callback for when seeking is done.
 */
void PathSeeker::moveAlongPath( const MotionConstants& mc,
	Vector3& outNewPos,
	SeekListener& callback )
{
	BW_GUARD;

	// No path to move along
	if (this->isFinished())
	{
		return;
	}

	const Path& path = pathQueue_.front();

	// Moving along path
	if (t_ < 1.0f)
	{
		const Vector3 goal = path.getPointAt( t_ );

		// Check if we are not close enough to the goal due to getting stuck.
		// Ignore y component.
		Vector3 distance = outNewPos - goal;
		distance.y = 0.0f;
		if (distance.length() > MAX_DISTANCE_TO_POINT)
		{
			//DEBUG_MSG( "Too far from path, cancelling\n" );
			this->cancel( false, callback );
			return;
		}

		outNewPos.x = goal.x;
		outNewPos.z = goal.z;
	}

	// Reached end
	else
	{
		outNewPos.x = path.getEnd().x;
		outNewPos.z = path.getEnd().z;

		t_ = t_ - 1.0f;

		// Get path to next point
		if (!this->isFinished())
		{
			//DEBUG_MSG( "Past point t %f\n", t_ );
			this->getNextPath( outNewPos, t_ );
			this->moveAlongPath(
				mc, outNewPos, callback );
		}
	}
}


/**
 *	Turn yaw towards the path.
 *	@param mc motion constants.
 *	@param outNewHeadYaw [out]Resultant head yaw
 */
void PathSeeker::turnToPath( const MotionConstants& mc, Angle& outNewHeadYaw )
{
	BW_GUARD;
	if (this->isFinished())
	{
		return;
	}

	const Path& path = pathQueue_.front();

	const Vector3& targetDirection = path.getDirection( t_ );
	float targetYaw = atan2f( targetDirection.x, targetDirection.z );
	outNewHeadYaw = targetYaw - mc.modelYaw;
}


/**
 *	Check if we have reached the end of all of the points.
 *	@return true if seeking has finished.
 */
bool PathSeeker::isFinished() const
{
	BW_GUARD;

	// Have paths - not finished
	return pathQueue_.empty();
}


/**
 *	Cancel seek.
 *	@param success if we have finished following the path successfully.
 *	@param callback notify callback that seek has finished.
 */
void PathSeeker::cancel( bool success,
	SeekListener& callback )
{
	BW_GUARD;
	pathQueue_.clear();
	t_ = 0.0f;

	// Note: This callback could set another seek target,
	// so make sure this function stays safe if that happens.
	callback.seekFinished( success );
}

BW_END_NAMESPACE

// seeker.cpp
