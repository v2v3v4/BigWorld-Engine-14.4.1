#include "pch.hpp"

#include "boids_filter.hpp"

#include "connection_control.hpp"
#include "entity.hpp"
#include "py_boids_filter.hpp"

#include "duplo/chunk_embodiment.hpp"
#include "duplo/chunk_attachment.hpp"

#include "math/vector3.hpp"

#include "terrain/base_terrain_block.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
///  Member Documentation for BoidsFilter
///////////////////////////////////////////////////////////////////////////////


/**
 *	@property	float BoidsFilter::influenceRadius_
 *
 *	This member is used to define the distance at which boids begin receiving
 *	influence from other boids.
 */


/**
 *	@property	float BoidsFilter::collisionFraction_
 *
 *	The collision fraction defines the proportion of the influence radius
 *	within which neighbours will be considered colliding.
 */


/**
 *	@property	float BoidsFilter::normalSpeed_
 *
 *	The normal speed of the boids.  While boids will speed up and slow down
 *	during flocking, their speed will always be damped toward this speed.
 *
 *	@note	Python name is 'speed'
 */


 /**
 *	@property	float BoidsFilter::timeMultiplier_
 *
 *	Multiplies the change in time used to update the boids. Affects the
 *	overall speed that they all move at.
 *
 *	@note	Python name is 'timeMultiplier'
 */


/**
 *	@property	float BoidsFilter::state_
 *
 * Controls whether the boids are flying or landing. By default they are in
 * the flying state which is specified with 0. The landing state will cause
 * the boids to land and is specified with 1. When a boid lands the method
 * boidsLanded is called.
 *
 *	@note	Python name is 'state_'
 */


 /**
 *	@property	float BoidsFilter::angleTweak_
 *
 *	This member determines the speed at which the boids adjust their pitch.\n
 *	Units: radians per 30th of a second.

 *
 *	@see	BoidsFilter::BoidData::updateModel()
 */


/**
 *	@property	float BoidsFilter::pitchToSpeedRatio_
 *
 *	This member defines how much the boids will slowdown or speedup based on
 *	their rate of climb or decent.
 *
 *	Units: metres per second per radian
 */


/**
 *	@property	float BoidsFilter::goalApproachRadius_
 *
 *	This member holds the distance within which landing boids will snap to
 *	face their target.
 */


/**
 *	@property	float BoidsFilter::goalStopRadius_
 *
 *	This member holds the distance within which boids are considered to have
 *	reached their target and will stop.
 */


/**
 *	@property	Boids BoidsFilter::boidData_
 *
 *	This member holds the data for all the boids in the flock. It is sized to
 *	mach the number of auxiliary embodiments possessed by its owner entity.
 */


/**
 *	@property	double BoidsFilter::prevTime_
 *
 *	This member holds the last time output was produced so that the correct
 *	delta time can be calculated for the ticking of the boids. 
 */


///////////////////////////////////////////////////////////////////////////////
///  End Member Documentation for BoidsFilter
///////////////////////////////////////////////////////////////////////////////


// These constants are not used in favour of member and local constants of
// the same name. However they have been left in the code for now in case
// their values are still useful. 
//
//const float INFLUENCE_RADIUS				= 10.0f;
//const float INFLUENCE_RADIUS_SQUARED		= INFLUENCE_RADIUS * INFLUENCE_RADIUS;
//const float COLLISION_FRACTION				= 0.5f;
//const float INV_COLLISION_FRACTION			= 1.0f/(1.0f-COLLISION_FRACTION);
//const float NORMAL_SPEED					= 0.5f;
//const float ANGLE_TWEAK						= 0.02f;
//const float PITCH_TO_SPEED_RATIO			= 0.002f;
//const float GOAL_APPROACH_RADIUS_SQUARED	= (10.0f * 10.0f);
//const float GOAL_STOP_RADIUS_SQUARED		= (1.0f * 1.0f);


/**
 *	This is the state value of the flock when it is landed.
 */
const int BoidsFilter::STATE_LANDING = 1;


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter. 
 */
BoidsFilter::BoidsFilter( PyBoidsFilter * pOwner ) :
	AvatarFilter( pOwner ),
	prevTime_( -1.f ),
	influenceRadius_( 10.f ),
	collisionFraction_( 0.5f ),
	normalSpeed_( 0.5f ),
	angleTweak_( 0.02f ),
	pitchToSpeedRatio_( 0.002f ),
	goalApproachRadius_( 10.f ),
	goalStopRadius_( 1.f ),
	initialHeight_( -20.f ),
	radius_( 1000.0f ),
	timeMultiplier_( 30.0f )
{
	BW_GUARD;	
}


/**
 *	Destructor
 */
BoidsFilter::~BoidsFilter()
{
	BW_GUARD;	
}


/*~ callback Entity.boidsLanded
 *
 *	This callback method is called on entities using the boids filter.
 *	It lets the entity know when the boids have landed. A typical response
 *	to this notifier is to delete the boids models.
 *
 *	@see BigWorld.BoidsFilter
 */
/**
 *	This method updates an entity's position as well as the auxiliary
 *	embodiment of the entity in such a was as to produce flocking behaviour of
 *	those embodiments. The individual 'boids' are each encapsulated in a
 *	BoidData object which are each updated during this output.\n\n
 *	This method also initiates a python callback on the entity when the
 *	'landed' state changes.\n
 *	Entity.boidsLanded()
 *
 *	@param	time	The client game time in seconds that the entity's volatile
 *					members should be updated for.
 *	@param	target	The entity to be updated
 *
 *	@see	BoidData::updateModel()
 */
void BoidsFilter::output( double time, MovementFilterTarget & target )
{
	BW_GUARD;

	static DogWatch dwBoidsFilterOutput("BoidsFilter");
	dwBoidsFilterOutput.start();

	Entity & entity = static_cast< Entity & >( target );

	this->AvatarFilter::output( time, target );

	Vector3 goal( entity.position() );

	if (prevTime_ == -1.f )
	{
		initialHeight_ = goal.y;
	}

	if ( state_ != STATE_LANDING )
	{
		goal.y = Terrain::BaseTerrainBlock::getHeight( goal.x, goal.z ) + 20.f;

		if (goal.y < initialHeight_)
			goal.y = initialHeight_;
	}

	float dTime = 1.f;

	if ( prevTime_ != -1.f )
	{
		dTime = float(time - prevTime_);
	}
	prevTime_ = time;

	EntityEmbodiments & models = entity.auxiliaryEmbodiments();
	int size = static_cast<int>(models.size());

	int oldSize = static_cast<int>(boidData_.size());

	if (oldSize != size)
	{
		boidData_.resize( size );

		for (int i = oldSize; i < size; i++)
		{
			boidData_[i].pos_ += goal;
		}
	}

	bool boidsLanded = false;

	for ( int i = 0; i < size; i++ )
	{
		IEntityEmbodimentPtr pCA = models[i];

		boidData_[i].updateModel( goal,
			&*pCA,
			*this,
			dTime,
			timeMultiplier_,
			state_,
			radius_ );

		if ( state_ == STATE_LANDING && boidData_[i].pos_ == goal )
		{
			boidsLanded = true;
		}
	}

	// If any boids landed, call the script and it will delete the models.
	if ( boidsLanded )
	{
		MF_ASSERT( !entity.isDestroyed() );
		entity.pPyEntity().callMethod( "boidsLanded", ScriptErrorPrint(),
			/* allowNullMethod */ true );
	}

	dwBoidsFilterOutput.stop();
}


/**
 *	This method will grab the history from another BoidsFilter if one is
 *	so provided
 */
bool BoidsFilter::tryCopyState( const MovementFilter & rOtherFilter )
{
	const BoidsFilter * pOtherBoidsFilter =
		dynamic_cast< const BoidsFilter * >( &rOtherFilter );

	if (pOtherBoidsFilter == NULL)
	{
		return this->AvatarFilter::tryCopyState( rOtherFilter );
	}

	influenceRadius_ = pOtherBoidsFilter->influenceRadius_;
	collisionFraction_ = pOtherBoidsFilter->collisionFraction_;
	normalSpeed_ = pOtherBoidsFilter->normalSpeed_;
	timeMultiplier_ = pOtherBoidsFilter->timeMultiplier_;
	state_ = pOtherBoidsFilter->state_;
	angleTweak_ = pOtherBoidsFilter->angleTweak_;
	pitchToSpeedRatio_ = pOtherBoidsFilter->pitchToSpeedRatio_;
	goalApproachRadius_ = pOtherBoidsFilter->goalApproachRadius_;
	goalStopRadius_ = pOtherBoidsFilter->goalStopRadius_;
	radius_ = pOtherBoidsFilter->radius_;
	boidData_ = pOtherBoidsFilter->boidData_;
	prevTime_ = pOtherBoidsFilter->prevTime_;
	initialHeight_ = pOtherBoidsFilter->initialHeight_;

	MF_VERIFY( this->AvatarFilter::tryCopyState( rOtherFilter ) );

	return true;
}


/**
 *	Constructor.
 */
BoidsFilter::BoidData::BoidData() :
	pos_(	20.0f * (unitRand()-unitRand()),
			 2.0f * unitRand(),
			20.0f * ( unitRand() - unitRand() ) ),
	dir_( unitRand(), unitRand(), unitRand() ),
	yaw_( 0.f ),
	pitch_( 0.f ),
	roll_( 0.f ),
	dYaw_( 0.f ),
	speed_( 0.1f )
{
	// Make sure it is not the zero vector.
	dir_.x += 0.0001f;
	dir_.normalise();
}


/**
 *	Destructor
 */
BoidsFilter::BoidData::~BoidData()
{
	BW_GUARD;	
}


/**
 *	This method updates the state of the model so that it is in its new
 *	position.
 *
 *	@param	goal	The current goal direction of the group.
 *	@param	pCA		The model that will be updated using the position and
 *					rotation of the boid.
 *	@param	filter	The BoidsFilter controlling the group of which this
 *					boid is a member.
 *	@param	dTime	The time in 1/30th of a second since the last update.
 *	@param	state	The state of the group. Landing or not landing.
 *
 *	@see
 *	@link	BoidsFilter::output()
 */
void BoidsFilter::BoidData::updateModel(	const Vector3 & goal,
											IEntityEmbodiment * pCA,
											const BoidsFilter & filter,
											const float dTime,
											const float timeMultiplier,
											const int state,
											const float radius )
{
	BW_GUARD;
	// The settings currently assume that dTime is in 30th of a second.
	const float DT = dTime * timeMultiplier;

	// No movement
	if (DT <= 0.0f)
	{
		return;
	}

	const BoidsFilter::Boids & boids = filter.boidData_;
	Vector3 deltaPos = Vector3::zero();
	Vector3 deltaDir = Vector3::zero();
	float count = 0;
	const float INFLUENCE_RADIUS_SQUARED =
		filter.influenceRadius_ * filter.influenceRadius_;
	const float INV_COLLISION_FRACTION =
		1.0f / ( 1.0f - filter.collisionFraction_ );
	const float GOAL_APPROACH_RADIUS_SQUARED =
		filter.goalApproachRadius_ * filter.goalApproachRadius_;
	const float GOAL_STOP_RADIUS_SQUARED =
		filter.goalStopRadius_ * filter.goalStopRadius_;

	for (uint i = 0; i < boids.size(); i++)
	{
		const BoidData & otherBoid = boids[i];

		if ( &otherBoid != this )
		{
			Vector3 diff = pos_ - otherBoid.pos_;
			float dist = diff.lengthSquared();
			dist = INFLUENCE_RADIUS_SQUARED - dist;

			if (dist > 0.f)
			{
				dist /= INFLUENCE_RADIUS_SQUARED;
				count += 1.f;

				diff.normalise();
				float collWeight = dist - filter.collisionFraction_;

				if (collWeight > 0.f)
				{
					collWeight *= INV_COLLISION_FRACTION;
				}
				else
				{
					collWeight = 0.f;
				}

				if (dist - ( 1.f - filter.collisionFraction_ ) > 0.f)
				{
					collWeight -= dist * ( 1.f - filter.collisionFraction_ );
				}

				Vector3 delta = collWeight * diff;
				deltaPos += delta;
				deltaDir += otherBoid.dir_ * dist;
			}
		}
	}

	if (count != 0.f)
	{
		deltaDir /= count;
		deltaDir -= dir_;
		deltaDir *= 1.5f;
	}

	Vector3 delta = deltaDir + deltaPos;

	// Add in the influence of the global goal
	Vector3 goalDir = goal - pos_;
	goalDir.normalise();
	goalDir *= 0.5f;
	delta += goalDir;

	// First deal with pitch changes
	if (delta.y > 0.01f)
	{   // We're too low
		pitch_ += filter.angleTweak_ * DT;

		if (pitch_ > 0.8f)
		{
			pitch_ = 0.8f;
		}
	}
	else if (delta.y < -0.01f)
	{   // We're too high
		pitch_ -= filter.angleTweak_ * DT;
		if (pitch_ < -0.8f)
		{
			pitch_ = -0.8f;
		}
	}
	else
	{
		// Add damping
		pitch_ *= 0.98f;
	}

	// Speed up or slow down depending on angle of attack
	speed_ -= pitch_ * filter.pitchToSpeedRatio_;

	// Damp back to normal
	speed_ = (speed_ - filter.normalSpeed_) * 0.99f + filter.normalSpeed_;

	if ( speed_ < filter.normalSpeed_ / 2 )
	{
		speed_ = filter.normalSpeed_ / 2;
	}
	if ( speed_ > filter.normalSpeed_ * 5 )
	{
		speed_ = filter.normalSpeed_ * 5;
	}

	// Now figure out yaw changes
	Vector3 offset	= delta;

	if (state != STATE_LANDING)
	{
		offset[Y_COORD] = 0.0f;
	}

	delta = dir_;
	offset.normalise( );
	float dot = offset.dotProduct( delta );

	// Speed up slightly if not turning much
	if (dot > 0.7f)
	{
		dot -= 0.7f;
		speed_ += dot * 0.05f;
	}

	Vector3 vo = offset.crossProduct( delta );
	dot = (1.0f - dot)/2.0f * 0.07f;

	if (vo.y > 0.05f)
	{
		dYaw_ = (dYaw_*19.0f + dot) * 0.05f;
	}
	else if (vo.y < -0.05f)
	{
		dYaw_ = (dYaw_*19.0f - dot) * 0.05f;
	}
	else
	{
		dYaw_ *= 0.98f; // damp it
	}

	yaw_ += dYaw_ * DT;
	roll_ = -dYaw_ * 20.0f * DT;

	// Set matrix for model
	Matrix m;
	m.setTranslate( pos_ );
	m.preRotateY( -yaw_ );
	m.preRotateX( -pitch_ );
	m.preRotateZ( -roll_ );

	dir_ = m.applyToUnitAxisVector( Z_AXIS );

	// Figure out position
	if (state == STATE_LANDING)
	{
		Vector3 goalDiff = pos_ - goal;
		float len = goalDiff.lengthSquared();

		if (len < GOAL_APPROACH_RADIUS_SQUARED)
		{
			dir_ = goalDir;
		}

		if (len > 0 && len < GOAL_STOP_RADIUS_SQUARED)
		{
			pos_ = goal;
			speed_ = 0;
		}
	}

	// Limit how far a boid can fly in the one direction for each frame
	// Ideally this should prevent the boid exiting the radius
	const Vector3 goalDiff = goal - pos_;
	const float goalDist = goalDiff.length();

	const float dirMultiplier = speed_ * DT;
	Vector3 flyVec = dir_ * dirMultiplier;
	if (dirMultiplier != 0.0f)
	{
		const float flyDist = flyVec.length();

		// Maximum distance we can fly in dir_, birds that are further out
		// get a smaller distance
		// Set a min distance so that birds do not get stuck
		const float maxFly = std::max(0.1f, radius - goalDist - 5.0f);

		if (flyDist > maxFly)
		{
			// Limit length of fly vector
			flyVec.normalise();
			flyVec *= maxFly;
		}
	}

	// Move the boid
	pos_ += flyVec;

	// Clamp position of boid to within the radius
	// (just in case one gets out too far, don't want boids getting lost if
	// there is a bug)
	if ( goalDist > radius )
	{
		// Respawn at centre
		// Don't print warning during a replay because fast forwarding can cause
		// boids to go out of range
		if (ConnectionControl::instance().pReplayConnection() == NULL)
		{
			WARNING_MSG( "BoidData::updateModel: "
				"boid flew too far out: distance %f, max distance %f\n",
				goalDist, radius );
		}
		pos_ = goal;
		dir_ = goalDir;
	}

	// Dev assert that model is within a reasonable area of the space
	const float LIMIT = 100000.f;
	IF_NOT_MF_ASSERT_DEV( -LIMIT < pos_.x && pos_.x < LIMIT &&
		-LIMIT < pos_.z && pos_.z < LIMIT  )
	{
		ERROR_MSG( "BoidData::updateModel: Tried to move boid to bad "
			"translation: ( %f, %f, %f )\n",
			pos_.x, pos_.y, pos_.z);
		pos_ = goal;
		dir_ = goalDir;
	}

	pCA->worldTransform( m );
}


BW_END_NAMESPACE

// boids_filter.cpp
