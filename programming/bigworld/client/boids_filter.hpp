#ifndef BOIDS_FILTER_HPP
#define BOIDS_FILTER_HPP

#include "avatar_filter.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class ChunkEmbodiment;
class Vector3;
class PyBoidsFilter;

/**
 *	This filter is a specialisation of AvatarFilter that implements flocking
 *	behaviour for the models that make up an entity.  This would normally
 *	be used for an entity such as a flock of birds or a school of fish which
 *	has many models driven by one entity.
 *
 *	The interpolation of the Entity position is done using the same logic
 *	as its parent AvatarFilter.  However, it also updates the positions
 *	of individual models (which are known, according to flocking conventions,
 *	as boids) that are attached to the entity, using standard
 *	flocking rules.
 */
class BoidsFilter : public AvatarFilter
{
private:

	/**
	 *	This class represents the data for an individual boid in the
	 *	flock.
	 */
	class BoidData
	{
	public:
		BoidData();
		~BoidData();

		void updateModel(	const Vector3 & goal,
							IEntityEmbodiment * pModel,
							const BoidsFilter & filter,
							const float dTime,
							const float timeMultiplier,
							const int state,
							const float radius );

		Vector3			pos_;
		Vector3			dir_;		// Current direction
		float			yaw_, pitch_, roll_, dYaw_;
		float			speed_;
	};

	/**
	 *	This typedef defines an STL vector of BoidData.
	 *
	 *	@see	BoidsFilter::boids_
	 */
	typedef BW::vector< BoidData >	Boids;

public:
	static const int STATE_LANDING;

	BoidsFilter( PyBoidsFilter * pOwner );
	virtual ~BoidsFilter();

	float influenceRadius() const { return influenceRadius_; }
	void influenceRadius( float newValue ) { influenceRadius_ = newValue; }

	float collisionFraction() const { return collisionFraction_; }
	void collisionFraction( float newValue ) { collisionFraction_ = newValue; }

	float normalSpeed() const { return normalSpeed_; }
	void normalSpeed( float newValue ) { normalSpeed_ = newValue; }

	float timeMultiplier() const { return timeMultiplier_; }
	void timeMultiplier( float newValue ) { timeMultiplier_ = newValue; }

	uint state() const { return state_; }
	void state( uint newValue ) { state_ = newValue; }

	/*
	float angleTweak() const { return angleTweak_; }
	void angleTweak( float newValue ) { angleTweak_ = newValue; }

	float pitchToSpeedRatio() const { return pitchToSpeedRatio_; }
	void pitchToSpeedRatio( float newValue ) { pitchToSpeedRatio_ = newValue; }
	*/

	float goalApproachRadius() const { return goalApproachRadius_; }
	void goalApproachRadius( float newValue ) { goalApproachRadius_ = newValue; }

	float goalStopRadius() const { return goalStopRadius_; }
	void goalStopRadius( float newValue ) { goalStopRadius_ = newValue; }

	float radius() const { return radius_; }
	void radius( float newValue ) { radius_ = newValue; }

protected:
	// Override from AvatarFilter
	virtual void output( double time, MovementFilterTarget & target );

	// Override from MovementFilter
	virtual bool tryCopyState( const MovementFilter & rOtherFilter );

private:
	// Doxygen comments for all members can be found in the .cpp
	float influenceRadius_;
	float collisionFraction_;
	float normalSpeed_;
	float timeMultiplier_;
	uint state_;
	float angleTweak_;
	float pitchToSpeedRatio_;
	float goalApproachRadius_;
	float goalStopRadius_;
	float radius_;

	Boids	boidData_;
	double	prevTime_;
	float	initialHeight_;
};

BW_END_NAMESPACE

#endif // BOIDS_FILTER_HPP
