#ifndef SEEKER_HPP
#define SEEKER_HPP

#include "cstdmf/smartpointer.hpp"
#include "linear_path.hpp"
#include "math/vector3.hpp"

#include <deque>


BW_BEGIN_NAMESPACE

class Angle;
struct MotionConstants;

/**
 *	Seek along path.
 */
class PathSeeker
{
public:

	/**
	 *	A class for a callback to Python when seeking has finished.
	 */
	class SeekListener
	{
	public:
		virtual void seekFinished( bool successfully ) = 0;
	};

	static const float MIN_DISTANCE_TO_POINT;
	static const float MAX_DISTANCE_TO_POINT;

	static bool debugDrawPaths;

	typedef BW::vector<Vector3> PathPoints;
	void setPath( const PathPoints& pathPoints, float yawAtEnd );

	void update( const MotionConstants& mc,
		const Vector3& useVelocity,
		float dTime,
		Angle& outNewHeadYaw,
		Vector3& outNewPos,
		SeekListener& callback );

	void cancel( bool success,
		SeekListener& callback );

private:
	void drawDebugPath( const MotionConstants& mc,
		const Vector3& newPos ) const;
	void drawDebugYaw( const MotionConstants& mc,
		const Vector3& newPos ) const;
	void getNextPath( const Vector3& currentPos, float displacement = 0.0f );
	void updateParameter( const Vector3& velocity, float dTime );
	void moveAlongPath( const MotionConstants& mc,
		Vector3& outNewPos,
		SeekListener& callback );
	void turnToPath( const MotionConstants& mc, Angle& outNewHeadYaw );
	bool isFinished() const;

	typedef LinearPath Path;
	typedef std::deque<Path> PathQueue;

	PathQueue pathQueue_;
	float yawAtEnd_;
	float t_;
};

BW_END_NAMESPACE

#endif // SEEKER_HPP
