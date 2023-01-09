#ifndef AVATAR_FILTER_HELPER_HPP
#define AVATAR_FILTER_HELPER_HPP

#include "network/basictypes.hpp"
#include "filter_helper.hpp"


BW_BEGIN_NAMESPACE

/**
 *
 */
class AvatarFilterHelper : public FilterHelper
{
public:
	static const uint NUM_STORED_INPUTS = 8;

	AvatarFilterHelper( FilterEnvironment & environment,
		const AvatarFilterHelper * pReferenceHelper = NULL );
	AvatarFilterHelper( const AvatarFilterHelper & other );
	AvatarFilterHelper & operator=( const AvatarFilterHelper & );


	void reset( double time );

	void input( double time, SpaceID spaceID, EntityID vehicleID,
		const Position3D & position, const Vector3 & positionError,
		const Direction3D & direction );

	double output( double time, SpaceID & rSpaceID, EntityID & rVehicleID,
		Position3D & rPosition, Vector3 & rVelocity,
		Direction3D & rDirection );

	bool getLastInput( double & time, SpaceID & spaceID,
		EntityID & vehicleID, Position3D & position,
		Vector3 & positionError, Direction3D & direction ) const;

	float latency() const { return latency_; }
	void latency( float v ) { latency_ = v; }

	void extract( double time, SpaceID & outputSpaceID,
		EntityID & outputVehicleID, Position3D & outputPosition,
		Vector3 & outputVelocity, Direction3D & outputDirection );

	/**
	 *	This is an internal structure used to encapsulate a single set of
	 *	received input values for use by the AvatarFilter. Currently the
	 *	avatar filter ignores 'roll' and that is continued here.
	 */
	class StoredInput
	{
	public:
		double		time_;
		SpaceID		spaceID_;
		EntityID	vehicleID_;
		Position3D	position_;
		Vector3		positionError_;
		Direction3D	direction_;
		bool		onGround_;
	};

	const StoredInput & getStoredInput( uint index ) const;

	static bool isActive();
	static void isActive( bool value );

protected:
	void init( const AvatarFilterHelper * pHelper );

	StoredInput & getStoredInput( uint index );

	/**
	 *	This structure stores a location in time and space for the filter
	 *	output to move to or from.
	 */
	class Waypoint
	{
	public:
		double			time_;
		SpaceID			spaceID_;
		EntityID		vehicleID_;
		Position3D		position_;
		Direction3D		direction_;

		StoredInput		storedInput_;

		void changeCoordinateSystem( FilterEnvironment & environment,
			SpaceID spaceID, EntityID vehicleID );
	};

	void resetStoredInputs(	double time,
							SpaceID spaceID,
							EntityID vehicleID,
							const Position3D & position,
							const Vector3 & positionError,
							const Direction3D & direction );

	void chooseNextWaypoint( double time );


	// Doxygen comments for all members can be found in the .cpp
	StoredInput		storedInputs_[NUM_STORED_INPUTS];
	uint			currentInputIndex_;
	uint			inputCount_;

	Waypoint		nextWaypoint_;
	Waypoint		previousWaypoint_;

	float			latency_;
	float			idealLatency_;
	double			timeOfLastOutput_;
	bool			gotNewInput_;
	bool			reset_;
};


/**
 *	This class is used to store settings associated with AvatarFilter.
 */
class AvatarFilterSettings
{
public:
	static float s_latencyVelocity_;
	static float s_latencyMinimum_;
	static float s_latencyFrames_;
	static float s_latencyCurvePower_;
	static bool  s_isActive_;
};

BW_END_NAMESPACE

#endif // AVATAR_FILTER_HELPER_HPP

