#include "pch.hpp"
#include "bouncer.hpp"
#include "pymodel.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_space.hpp"

DECLARE_DEBUG_COMPONENT2( "Motor", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( Bouncer )

PY_BEGIN_METHODS( Bouncer )

	/*~ function Bouncer.calcPath
	 *
	 *	Pre-calculates the path of the object, as determined 
	 *	by the Bouncer motor. The Bouncer will move the object
	 *	along this path as the motor is updated over time.
	 *
	 *	The clientSourcePos is the initial position, and the model has its
	 *	position interpolated over the first second of its trajectory to
	 *	join on the calculated path from serverSourcePos to destPos.
	 *
	 *	<i>This method should be used on witnessing clients.</i>
	 *
	 *	<i>This needs to be called before the motor is used.</i>
	 *
	 *	<i>The tripTime attribute must be set to an estimate before this 
	 *	function is called. The simulation will be carried up to tripTime.
	 *	This can either be sent by the server or be a constant in the game
	 *	(e.g., the fuse time of a grenade).</i>
	 *
	 *  <i>Velocity is taken from the vx, vy and vz attributes. These and 
	 *	timeSlice must also be set before calcPath can be called.</i>
	 *
	 *	@param clientSourcePos	Sequence of floats (x, y, z): initial position on the client.
	 *	@param serverSourcePos	Sequence of floats (x, y, z): initial position on the server. 
	 *	@param destPos			Sequence of floats (x, y, z): destination position. 
	 *	@param elasticity		float: bouncing object elasticity factor. 
	 *	@param radius			float: bouncing object radius.
	 *	@param maxBounces		int: maximum number of bounces before stoping the simulation.
	 *	@return					2-tuple: (total simulation time, time to reach destination).
	 */
	PY_METHOD_WITH_DOC( calcPath, "Pre-calculates the path of the object, as "
		"determined by the Bouncer motor. The Bouncer will move the object "
		"along this path as the motor is updated over time." )

	/*~ function Bouncer.estimatePath
	 *
	 *	Estimates a path before the final information arrives from the server. 
	 *
	 *	<i>This method should be used on the client activating the bouncer.</i>
	 *
	 *	<i>This needs to be called before the motor is used.</i>
	 *
	 *	<i>The tripTime attribute must be set to an estimate before this 
	 *	function is called. The simulation will be carried up to tripTime.
	 *	The lifetime of the bouncing object is usually the best guess.</i>
	 *
	 *  <i>Velocity is taken from the vx, vy and vz attributes. These and 
	 *	timeSlice must also be set before estimatePath is called.</i>
	 *
	 *	@param clientSourcePos	Sequence of floats (x, y, z): initial position on the client.
	 *	@param elasticity		float: bouncing object elasticity factor. 
	 *	@param radius			float: bouncing object radius.
	 *	@param maxBounces		int: maximum number of bounces before stoping the simulation.
	 *	@return					float: total simulation time.
	 */
	PY_METHOD_WITH_DOC( estimatePath, "Estimates a path before the final "
		"information arrives from the server." )

	/*~ function Bouncer.updatePath
	 *
	 *	Updates an estimated path, based on the final authoritative information 
	 *	received from the server. The combination of estimatePath and updatePath 
	 *	should end up with the same result as calcPath, but it can start earlier.
	 *	The path is smoothed from the estimate to the correct result, based on
	 *	what has already been played back.
	 *
	 *	<i>This method should be used on the client that activated the bouncer,
	 *	after it has received the server-computed source and destination positions.</i>
	 *
	 *	@param clientSourcePos	Sequence of floats (x, y, z): initial position on the client.
	 *	@param serverSourcePos	Sequence of floats (x, y, z): initial position on the server. 
	 *	@param destPos			Sequence of floats (x, y, z): destination position. 
	 *	@return					float: total simulation time.
	 */
	PY_METHOD_WITH_DOC( updatePath,
	"Updates an estimated path, based on the final authoritative information "
	"received from the server. The combination of estimatePath and updatePath "
	"should end up with the same result as calcPath, but it can start earlier. "
	"The path is smoothed from the estimate to the correct result, based on "
	"what has already been played back.")
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Bouncer )
	/*~ attribute Bouncer.vx
	 *	Initial velocity in the models x-axis direction.
	 *	@type Float. Default is 0.
	 */
	PY_ATTRIBUTE( vx )
	/*~ attribute Bouncer.vy
	 *	Initial velocity in the models y-axis direction.
	 *	@type Float. Default is 0.
	 */
	PY_ATTRIBUTE( vy )
	/*~ attribute Bouncer.vz
	 *	Initial velocity in the models z-axis direction.
	 *	@type Float. Default is 0.
	 */
	PY_ATTRIBUTE( vz )
	/*~ attribute Bouncer.tripTime
	 *	The attribute tripTime is used to specify in rough terms the expected
	 *	time that the object will take to come to rest (reach its destination).
	 *	This needs to be set to a value greater than 0 before calcPath is
	 *	called. It will serve as the upper bound for the simulation time. 
	 *	This, along with timeSlice allows you to specify how many steps 
	 *	are precalculated by calcPath for the objects trajectory, and to 
	 *	control how much processor time is used by calcPath.
	 *	@type Float. Default is -1 (this must be set before calcPath is called).
	 */
	PY_ATTRIBUTE( tripTime )
	/*~ attribute Bouncer.timeSlice
	 *	The attribute timeSlice is used to specify the time interval between
	 *	pre-calculated positions for the objects path. This needs to be set
	 *	to a value greater than 0 before calcPath is called. A rough estimate
	 *	for this would be the time interval between frames. This along with 
	 *	tripTime allows you to specify how many steps are precalculated by 
	 *	calcPath for the objects trajectory, and to control how much processor 
	 *	time is used by calcPath.
	 *	@type Float. Default is 0 (this must be set before calcPath is called).
	 */
	PY_ATTRIBUTE( timeSlice )
	/*~ attribute Bouncer.destPos
	 *	The attribute destPos can be used to access the destination position
	 *	for the path of the Bouncer. This attribute is Read Only. If called
	 *	before calcPath or estimatePath are called, the return value will be
	 *	the zero vector.
	 *	@type Sequence of floats (x, y, z).
	 */
	PY_ATTRIBUTE( destPos )
PY_END_ATTRIBUTES()


/*~ function BigWorld.Bouncer
 *	Bouncer is a factory function to create a Bouncer Motor. A Bouncer is a
 *	Motor that starts with an initial velocity, and bounces according to the
 *	laws of physics, finally coming to rest. This Motor could be used for thrown
 *	objects, such as grenades.
 *	@return A new Bouncer object.
 */
PY_FACTORY( Bouncer, BigWorld )


/**
 *	Constructor
 */
Bouncer::Bouncer( PyTypeObject * pType ):
	Motor( pType ),
	tripTime_( -1.f ),
	vx_(0.f),
	vy_(0.f),
	vz_(0.f),
	acc_(-9.8f),
	elasticity_(),
	radius_(),
	timeSlice_(0.f / 30.f),
	totaldTime_(0.f),
	lastSlice_(),
	maxBounces_( -1 )
{
}

/**
 *	Destructor
 */
Bouncer::~Bouncer()
{
}


/**
 *	Static python factory method
 */
PyObject * Bouncer::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.Bouncer brooks no arguments (%d given)",
			PyTuple_Size( args ) );
		return NULL;
	}

	return new Bouncer();
}


/**
*	This class finds the closest triangle hit and records it
*/
class BouncerClosestTriangle : public CollisionCallback
{
public:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float /*dist*/ )
	{
		// transform into world space
		hit_ = WorldTriangle(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ) );
		return COLLIDE_BEFORE;
	}

	WorldTriangle hit_;
};

#define LOG_BOUNCER 0
#define LOG_BOUNCES_ONLY 0

/**
 *	This method allows the Python script to work out the bounce path.
 *
 *  @param clientSourcePos	The client side initial position.
 *  @param serverSourcePos	The server side initial position.
 *  @param destPos			The final position of the bounce path.
 *  @param elasticity		How bouncy the path should be.
 *  @param radius			Radius to limit the bounces within.
 *  @param maxBounces		Maximum number of bounces to make.
 *
 *  @returns	A Python tuple containing the bounce path
 */
PyObject* Bouncer::calcPath( const Vector3 & clientSourcePos,
							const Vector3 & serverSourcePos,
							const Vector3 & destPos, float elasticity,
							float radius, int maxBounces )
{
	BW_GUARD;
	elasticity_ = elasticity;
	radius_ = radius;
	lastSlice_ = -1;
	int numSteps = int( tripTime_ / timeSlice_ );
	pathSteps_.clear();
	pathSteps_.reserve(numSteps);

	Vector3 worldPos = serverSourcePos;
	Vector3 vel( vx_, vy_, vz_ );
	float dTime = timeSlice_;

	ChunkSpacePtr cs = ChunkManager::instance().cameraSpace();
	MF_ASSERT_DEV( cs.exists() );
	BouncerClosestTriangle ct;
	WorldTriangle resTriangle;

	bool done = false;
	int loopCount = 0;
	int bounceCount = 0;
	int closeCount = -1;
	while (!done)
	{
		++loopCount;
		vel[1] += -9.8f * dTime * 0.5f;

		bool checkForBounce = true;
		bool bounced = false;
		Vector3 endPos = worldPos;
		while (checkForBounce) {
			checkForBounce = false;

			Vector3 newPos = endPos + vel * dTime;
			float distance = 0;
			if( cs.exists() )
				distance = cs->collide( endPos, newPos, ct );
			resTriangle = ct.hit_;
#if LOG_BOUNCER
			INFO_MSG( "New pos (%f, %f, %f); collision distance = %f\n", (double)newPos[0],
							(double)newPos[1], (double)newPos[2], (double)distance );
#endif

			if (distance > 0.f && distance <= (newPos - endPos).length()) {
				bounced = true;
				// Don't move to the triangle we're bouncing off, in case
				// we end up just on the other side. Try to stay 5cm on this
				// side of it.
				float proportionBeforeBounce( (distance - radius_) / vel.length() );
				if (proportionBeforeBounce < 0.0f)
					proportionBeforeBounce = 0.0f;
				endPos = endPos + vel * proportionBeforeBounce;

#if LOG_BOUNCER || LOG_BOUNCES_ONLY
				INFO_MSG( "Bouncing at (%f, %f, %f)\n", (double)endPos[0],
							(double)endPos[1], (double)endPos[2] );
#endif

				if (maxBounces != -1 && bounceCount == maxBounces)
				{
					checkForBounce = false;
				}
				else
				{
					checkForBounce = true;
					float oldSpeed = vel.length();
					resTriangle.bounce( vel, 1.0f );
					vel = elasticity_ * vel;
					endPos = endPos + vel * (dTime - proportionBeforeBounce);
#if LOG_BOUNCER
					INFO_MSG( "Bounce changed speed from %f to %f\n",
							(double)oldSpeed, (double)vel.length() );
#endif
				}
			}
			else
			{
				endPos = newPos;
			}
		}
		if (bounced)
		{
			++bounceCount;
			if (maxBounces != -1 && bounceCount > maxBounces)
			{
#if LOG_BOUNCER
				INFO_MSG( "Ending because bounceCount(%d) > maxBounces(%d)\n", bounceCount, maxBounces);
#endif
				done = true;
			}
		}

		// Revise the new position
		if (destPos.y > -1000000.f)
		{
			float distance = (endPos - destPos).length();
			if (closeCount == -1 && distance < 0.1f )
			{
				// This is when we've got close to the final position
				closeCount = loopCount;
#if LOG_BOUNCER
				INFO_MSG( "close enough at %d, %f\n", closeCount, distance );
#endif
			}
			if (distance < 0.005f || loopCount >= numSteps * 2) {
				done = true;
#if LOG_BOUNCER
				INFO_MSG("(endPos - destPos).length() = %f (%d/%d)\n", distance, loopCount, numSteps);
#endif
			}
#if LOG_BOUNCER
			else
				INFO_MSG("    distance to destination = %f (%d/%d)\n", distance, loopCount, numSteps);
#endif
		}
		else
		{
			if ((endPos-worldPos).length() / dTime < 0.25f || loopCount >= numSteps)
				done = true;
		}
		worldPos = endPos;
		PathInfo step;
		step.pos_ = worldPos;
		// NB: it stores the new velocity (after rebound) which is lower than
		// the actual impact velocity.  It should be fine for our purposes
		step.bounceVel_ = (bounced) ? vel.length() : 0;
		pathSteps_.push_back(step);

		// Update velocity
		if (vel.length() < 0.25)
			vel = Vector3(0.0, vel[1], 0.0);
	}

	if (closeCount == -1)
		closeCount = loopCount;

#if LOG_BOUNCER || LOG_BOUNCES_ONLY
	INFO_MSG( "Bounce ending at (%f, %f, %f)\n", (double)worldPos[0],
							(double)worldPos[1], (double)worldPos[2] );
#endif

	// Adjust the first second's positions to get us from our start pos to the path.
	Vector3 startErrorAmount( clientSourcePos - serverSourcePos );
	int numAdjustments = min(int( 1.f / timeSlice_ ),
								int(pathSteps_.size()) - 1);
	float adjustPerStep = 1.f / numAdjustments;
	BW::vector<PathInfo>::iterator iter = pathSteps_.begin();
	int i = 0;
	for (i = numAdjustments; i > 0 && iter != pathSteps_.end();
												++iter, --i)
	{
		Vector3 adjustment(startErrorAmount);
		adjustment *= i * adjustPerStep;
		(*iter).pos_ += adjustment;
	}

	if (destPos.y > -1000000.f)
	{
		// Adjust the last two seconds' positions to get us to the correct final spot.
		Vector3 endErrorAmount( destPos - pathSteps_.back().pos_ );
		numAdjustments = int( 2.f / timeSlice_ );
		numAdjustments = min(numAdjustments, int(pathSteps_.size()));
		adjustPerStep = 1.f / numAdjustments;
		iter = pathSteps_.end() - numAdjustments;
		for (i = 1; iter != pathSteps_.end(); ++iter, ++i)
		{
			Vector3 adjustment(endErrorAmount);
			adjustment *= i * adjustPerStep;
			(*iter).pos_ += adjustment;
		}
	}

	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( pathSteps_.size() * timeSlice_ ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( closeCount * timeSlice_ ) );
	return pTuple;
}


/**
 *	Estimate without the real results from the server
 */
PyObject * Bouncer::estimatePath( const Vector3 & clientSourcePos,
									float elasticity, float radius,
									int maxBounces )
{
	BW_GUARD;
	elasticity_ = elasticity;
	radius_ = radius;
	lastSlice_ = -1;
	maxBounces_ = maxBounces;
	int numSteps = int( tripTime_ / timeSlice_ );
	pathSteps_.reserve(numSteps);

	Vector3 worldPos = clientSourcePos;
	Vector3 vel( vx_, vy_, vz_ );
	float dTime = timeSlice_;

	ChunkSpace * cs = &*ChunkManager::instance().cameraSpace();
	BouncerClosestTriangle ct;
	WorldTriangle resTriangle;

	bool done = false;
	int loopCount = 0;
	int bounceCount = 0;
	while (!done)
	{
		++loopCount;
		vel[1] += -9.8f * dTime * 0.5f;

		bool checkForBounce = true;
		bool bounced = false;
		Vector3 endPos = worldPos;
		while (checkForBounce) {
			checkForBounce = false;

			Vector3 newPos = endPos + vel * dTime;
			float distance = cs->collide( endPos, newPos, ct );
			resTriangle = ct.hit_;
#if LOG_BOUNCER
			INFO_MSG( "Est pos (%f, %f, %f); collision distance = %f\n", (double)newPos[0],
							(double)newPos[1], (double)newPos[2], (double)distance );
#endif

			if (distance > 0.f && distance <= (newPos - worldPos).length()) {
				bounced = true;
				float proportionBeforeBounce( (distance - radius_) / vel.length() );
				if (proportionBeforeBounce < 0.0f)
					proportionBeforeBounce = 0.0f;
				endPos = endPos + vel * proportionBeforeBounce;
				Vector3 preVel( vel );

				if (maxBounces != -1 && bounceCount == maxBounces)
				{
					checkForBounce = false;
				}
				else
				{
					checkForBounce = true;
					float oldSpeed = vel.length();
					resTriangle.bounce( vel, 1.0f );
					vel = elasticity_ * vel;
					endPos = endPos + vel * (dTime - proportionBeforeBounce);
#if LOG_BOUNCER
					INFO_MSG( "Bounce changed speed from %f to %f\n",
							(double)oldSpeed, (double)vel.length() );
#endif
				}
#if LOG_BOUNCER
				INFO_MSG( "Bouncing off (%f, %f, %f) (%f)\n", (double)endPos[0],
							(double)endPos[1], (double)endPos[2], (double)proportionBeforeBounce );
				INFO_MSG( "Bouncing to (%f, %f, %f) (%f)\n", (double)endPos[0],
							(double)endPos[1], (double)endPos[2], (double)proportionBeforeBounce );
				INFO_MSG( "Bouncing off (%f, %f, %f)\n", (double)resTriangle.v0()[0],
							(double)resTriangle.v0()[1], (double)resTriangle.v0()[2] );
				INFO_MSG( "Bouncing off (%f, %f, %f)\n", (double)resTriangle.v1()[0],
							(double)resTriangle.v2()[1], (double)resTriangle.v1()[2] );
				INFO_MSG( "Bouncing off (%f, %f, %f)\n", (double)resTriangle.v2()[0],
							(double)resTriangle.v2()[1], (double)resTriangle.v2()[2] );
				INFO_MSG( "Velocity from %f (%f, %f, %f)\n", (double)preVel.length(),
							(double)preVel[0], (double)preVel[1], (double)preVel[2] );
				INFO_MSG( "Velocity to   %f (%f, %f, %f)\n", (double)vel.length(),
							(double)vel[0], (double)vel[1], (double)vel[2] );
#endif
			}
			else
			{
				endPos = newPos;
			}
		}
		if (bounced)
		{
			++bounceCount;
			if (maxBounces != -1 && bounceCount >= maxBounces)
				done = true;
		}

		// Revise the new position
		if ((endPos-worldPos).length() / dTime < 0.25f || loopCount >= numSteps)
			done = true;
		worldPos = endPos;
		PathInfo step;
		step.pos_ = worldPos;
		// NB: it stores the new velocity (after rebound) which is lower than
		// the actual impact velocity.  It should be fine for our purposes
		step.bounceVel_ = (bounced) ? vel.length() : 0;
		pathSteps_.push_back(step);

		// Update velocity
		if (vel.length() < 0.25)
			vel = Vector3(0.0, vel[1], 0.0);
	}

#if LOG_BOUNCER
	INFO_MSG( "Estimate length %f\n", pathSteps_.size() * timeSlice_ );
#endif

	return Script::getData( pathSteps_.size() * timeSlice_ );
}


/**
 *	Revised the estimate now that we have the real information from the server.
 */
PyObject * Bouncer::updatePath( const Vector3 & serverSourcePos,
									const Vector3 & destPos )
{
	BW_GUARD;	
#if LOG_BOUNCER
	INFO_MSG( "Updated path from (%f, %f, %f) to (%f, %f, %f)\n",
				serverSourcePos[0], serverSourcePos[1], serverSourcePos[2],
				destPos[0], destPos[1], destPos[2]);
#endif

	// If doneSlices is already at or past the end of the old path the grenade will
	// appear to have finished it's bouncing. But, we still need to move the
	// grenade. We'll just snap it to the final position. We'll do this by
	// making sure pathSteps_ is long enough to cover a little bit of the
	// future, and just setting every value to the final position.
	int doneSlices = int( totaldTime_ / timeSlice_ );
	int slicesPerSecond = int( 1.0f / timeSlice_ );
	if ( doneSlices >= (int)pathSteps_.size() - 1 )
	{
		// First we'll need to make pathSteps_ large enough for the new
		// path we'll construct. Add enough steps to take us 1/4s past now.
		// Do this by replicating the last element.
		PathInfo newValue;
		newValue.pos_ = destPos;
		newValue.bounceVel_ = 0.0f;
		pathSteps_.clear();
		pathSteps_.insert( pathSteps_.end(), doneSlices + slicesPerSecond / 2,
							newValue );

		PyObject * pTuple = PyTuple_New( 2 );
		PyTuple_SET_ITEM( pTuple, 0, Script::getData( 0.25 ) );
		PyTuple_SET_ITEM( pTuple, 1, Script::getData( 0.25 ) );
		return pTuple;
	}


	// Use calcPath to work out what we should have done.
	BW::vector<PathInfo> originalSteps = pathSteps_;
	int lastSlice = lastSlice_;
	PyObject* pathTimes = this->calcPath(serverSourcePos, serverSourcePos,
											destPos, elasticity_,
											radius_, maxBounces_ );
	lastSlice_ = lastSlice;

	float pathTime;
	float closeTime;
	PyArg_ParseTuple( pathTimes, "ff", &pathTime, &closeTime );

#if LOG_BOUNCER
	INFO_MSG( "calcPath returned  pathTime = %f, closeTime = %f, size = %d)\n", (double)pathTime,
							(double)closeTime, pathSteps_.size() );
#endif

	// Now to adjust the remaining path to be played out.
	// Set up some variables:
	//  - doneSlices is where we're up to.
	//  - originalSlices is the number of slices of the estimated path we'll
	//    keep (of course, we don't need to bother with the first doneSlices
	//	  at all).
	//  - adjustEndSlice is the slice where we'll stop the adjustment and use
	//	  the result from calcPath.
	// We'll be adjusting the values between min(doneSlices, originalSlices)
	// and adjustEndSlice only.
	int originalSlices = doneSlices + 1;
	int adjustEndSlice = 0;
	int i;

	// If doneSlices is already past the end of the new path we need to
	// take drastic measures. Over a quarter of a second, move the path to
	// where the new path ends.
	if ( doneSlices >= (int)pathSteps_.size() )
	{
		// First we'll need to make pathSteps_ large enough for the new
		// path we'll construct. Add enough steps to take us 1/4s past now.
		// Do this by replicating the last element.
		size_t extraSteps = doneSlices - pathSteps_.size() + slicesPerSecond / 4;
		pathSteps_.insert( pathSteps_.end(), extraSteps, pathSteps_.back() );

		adjustEndSlice = static_cast<int>(pathSteps_.size()) - 1;

		closeTime = pathSteps_.size() * timeSlice_ ;
#if LOG_BOUNCER
		INFO_MSG( "Adjusted closeTime to %f\n", (double)closeTime );
#endif
	}
	else
	{
		// If the next original path bounce occurs in the next half second,
		// but with at least another half second of the path after the bounce,
		// we'll leave the path alone until the bounce, otherwise we'll
		// start the adjustment immediately.
		int end = min( doneSlices + slicesPerSecond + 1,
						int(pathSteps_.size() - slicesPerSecond / 2) );
		end = min( end, int(originalSteps.size()) );
		for (i = doneSlices + 1; i < end; ++i)
		{
			if (originalSteps[i].bounceVel_ > 1.0f)
			{
				originalSlices = i;
				break;
			}
		}

		// If the adjustment is starting at a bounce and the time from the start
		// of the adjustment to the bounce following is more than 0.5 seconds
		// we'll adjust over that bounce only.
		// Otherwise, we'll adjust over the next second.
		adjustEndSlice = originalSlices + slicesPerSecond;
		if (originalSteps[ originalSlices ].bounceVel_ > 1.0f)
		{
			end = min( originalSlices + slicesPerSecond / 2, int(pathSteps_.size() - 1) );
			for (i = originalSlices + 1; i <= end; ++i)
			{
				if (pathSteps_[i].bounceVel_ > 1.0f)
				{
					adjustEndSlice = i;
					break;
				}
			}
		}
		if (adjustEndSlice >= (int)pathSteps_.size())
			adjustEndSlice = static_cast<int>(pathSteps_.size()) - 1;
	}
	if (adjustEndSlice >= (int)originalSteps.size())
		adjustEndSlice = static_cast<int>(originalSteps.size()) - 1;

	// Have we ended up with too small an adjustment period at the end?
	// This could happen if the original path was much shorter than the
	// final path. Make sure the adjustment is at least half a second long.
	if ( (adjustEndSlice - originalSlices) < slicesPerSecond / 2 )
	{
		originalSlices = adjustEndSlice - slicesPerSecond / 2;
		if (originalSlices <= doneSlices)
			originalSlices = doneSlices + 1;
	}

	// Copy from from the original path to the new path for the slices
	// from originalSlices to doneSlices.
	for (i = doneSlices; i < originalSlices; ++i)
	{
#if LOG_BOUNCER
		INFO_MSG( "Step %d keeping original (%f, %f, %f)\n", i, (double)originalSteps[i].pos_[0],
							(double)originalSteps[i].pos_[1], (double)originalSteps[i].pos_[2] );
#endif
		pathSteps_[i] = originalSteps[i];
	}

	// Adjust the path from i to adjustEndSlice
	int numAdjustSlices = adjustEndSlice - i + 1;
	float adjustStart = float(i - 1);
	for (; i < adjustEndSlice; ++i )
	{
		float level = (i - adjustStart) / float(numAdjustSlices);
		Vector3 result = (1.0f - level) * originalSteps[i].pos_
								+ level * pathSteps_[i].pos_;
#if LOG_BOUNCER
		INFO_MSG( "Step %d blending from %f*(%f, %f, %f) to %f*(%f, %f, %f) giving (%f, %f, %f)\n",
					i, (double)(1.0f - level), (double)originalSteps[i].pos_[0],
					(double)originalSteps[i].pos_[1], (double)originalSteps[i].pos_[2],
					(double)level, (double)pathSteps_[i].pos_[0],
					(double)pathSteps_[i].pos_[1], (double)pathSteps_[i].pos_[2],
					(double)result[0],
					(double)result[1], (double)result[2] );
#endif
		pathSteps_[i].pos_ = result;
	}

	float timeLeft = (pathSteps_.size() - doneSlices) * timeSlice_ ;

#if LOG_BOUNCER
		INFO_MSG( "Elapsed time %f, time left %f, gets close in %f\n",
					totaldTime_, timeLeft, closeTime - totaldTime_ );
#endif

	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( timeLeft ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( closeTime - totaldTime_ ) );
	return pTuple;
}


/**
 *	Run this motor
 */
void Bouncer::rev( float dTime )
{
	BW_GUARD;
	// How many slices have we done so far? How many do we need done?
	int doneSlices = int( totaldTime_ / timeSlice_ );
	int neededSlices = int( (totaldTime_ + dTime) / timeSlice_ - doneSlices );

	totaldTime_ += dTime;
	dTime = timeSlice_;

	if ((doneSlices + neededSlices) >= int(pathSteps_.size()))
		return;

	Matrix world = pOwner_->worldTransform();
	Vector3 worldPos = pathSteps_[doneSlices + neededSlices].pos_;
	world.translation( worldPos );
	pOwner_->worldTransform( world );
#if LOG_BOUNCER
	INFO_MSG("bounce position %d (%f, %f, %f)\n", doneSlices + neededSlices, (double)worldPos[0], (double)worldPos[1], (double)worldPos[2]);
#endif

	// check all intervening slices for a bounce
	for (int i=lastSlice_ + 1; i<=doneSlices; i++) {
		if (pathSteps_[i].bounceVel_ > 1.0f && PyModel::pCurrent()) {
			static char* tag[] = {
				"grenades/bounce_ppp",
				"grenades/bounce_pp",
				"grenades/bounce_p",
				"grenades/bounce"
			};
			int v = int(pathSteps_[i].bounceVel_) - 1;
			if (v > 3) v = 3;
			PyModel::pCurrent()->playSound( tag[v] );
		}
	}

	lastSlice_ = doneSlices;
}

/**
 *	set value of timeSlice_
 */
void Bouncer::timeSlice( float slice )
{
	BW_GUARD;
	if (slice <= 0.0f)
	{
		ERROR_MSG( "timeSlice must be positive. %f is invalid\n", slice );
		return;
	}

	if (totaldTime_ > 0.0f)
	{
		// Adjust totaldTime_ so we appear to have done the same number
		// of slices after the change to timeSlice_
		totaldTime_ *= slice / timeSlice_;
		timeSlice_ = slice;
	}
	else
	{
		timeSlice_ = slice;
	}
}

BW_END_NAMESPACE

// bouncer.cpp
