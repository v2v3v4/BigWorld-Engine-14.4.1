#include "script/first_include.hpp"

#include "passengers.hpp"

#include "passenger_extra.hpp"
#include "math/angle.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Passengers
// -----------------------------------------------------------------------------

/// static initialiser
const Passengers::Instance<Passengers> Passengers::instance( NULL );


/**
 *	Constructor.
 */
Passengers::Passengers( Entity & e ) :
	EntityExtra( e ),
	inDestructor_( false )
{
	this->adjustTransform();
}


/**
 *	Destructor.
 */
Passengers::~Passengers()
{
	inDestructor_ = true;
	Container::size_type size = passengers_.size();

	while (!passengers_.empty())
	{
		Entity * pPassenger = passengers_.back();
		if (PassengerExtra::instance.exists( *pPassenger ))
		{
			PassengerExtra::instance( *pPassenger ).onVehicleGone();
			// it will remove itself from our list
		}
		else
		{
			CRITICAL_MSG( "Passengers::~Passengers: "
				"PassengerExtra does not exist\n" );
			passengers_.pop_back();
		}

		--size;
		MF_ASSERT( size == passengers_.size() );
	}
}


/**
 *	This method adds an entity as a passenger of this entity.
 *
 *	@return True on success, otherwise false.
 */
bool Passengers::add( Entity & entity )
{
	// Make sure that this entity is not already a passenger.
	MF_ASSERT( std::find( passengers_.begin(), passengers_.end(), &entity ) ==
						passengers_.end() );

	passengers_.push_back( &entity );

	return true;
}


/**
 *	This method removes an entity from being a passenger of this entity.
 *
 *	@return True on success, otherwise false.
 */
bool Passengers::remove( Entity & entity )
{
	// Here we do a brute-force search to find and remove the entity. This makes
	// the assumption that there are not too many passengers on this entity. If
	// there are likely to be many, the passenger could keep the index of where
	// it is in the passengers_ vector. (Like Space::removeEntity).

	Container::iterator iter =
		std::find( passengers_.begin(), passengers_.end(), &entity );

	if (iter != passengers_.end())
	{
		passengers_.erase( iter );

		if (passengers_.empty() && !inDestructor_)
		{
			// Note: This needs to be the last thing before the return statement
			// as it deletes this object.
			Passengers::instance.clear( entity_ );
		}

		return true;
	}

	WARNING_MSG( "Passengers::remove: Failed to remove %u from %u\n",
			entity.id(), entity_.id() );
	return false;
}


/**
*this method transfer the input global pos into local pos on the vehicle space
*/
void Passengers::transformToVehiclePos( Position3D& pos )
{
	Matrix m;
	m.invert( transform_ );
	pos = m.applyPoint( pos );
}

/**
*this method transfer the input global dir into local dir on the vehicle space
*/
void Passengers::transformToVehicleDir( Direction3D& dir )
{
	// TODO: What do we do here? For now, just doing yaw.
	dir.yaw =
		Angle( dir.yaw - entity_.direction().yaw );
}

/**
 *	This method updates any information in the passengers that may be affected
 *	by the vehicle moving.
 */
void Passengers::updateInternalsForNewPosition(
		const Vector3 & /*oldPosition*/ )
{
	this->adjustTransform();

	Container::iterator iter = passengers_.begin();
	Container::iterator endIter = passengers_.end();

	while (iter != endIter)
	{
		(*iter)->onVehicleMove();

		++iter;
	}
}


/**
 *	This method sets the global-to-child transform associated with this vehicle
 *	so that passengers can calculate their global coordinates from their local.
 */
void Passengers::adjustTransform()
{
	const Direction3D & dir = entity_.direction();
	transform_.setRotate( dir.yaw, dir.pitch, dir.roll );
	transform_.translation( entity_.position() );
}

BW_END_NAMESPACE

// passengers.cpp
