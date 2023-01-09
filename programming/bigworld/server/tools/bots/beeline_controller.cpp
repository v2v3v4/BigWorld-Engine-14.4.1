#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "beeline_controller.hpp"
#include "cstdmf/bw_string.hpp"
using namespace std;

#include "cstdmf/debug.hpp"

// #include "resmgr/bwresource.hpp"
// #include "resmgr/datasection.hpp"
// #include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Bots", 100 );


BW_BEGIN_NAMESPACE

namespace Beeline
{

BeelineController::BeelineController( const Vector3 &destinationPos ) :
	destinationPos_( destinationPos )
{
}

bool BeelineController::nextStep (float & speed,
	float dTime,
	Vector3 &pos,
	Direction3D &dir)
{
	// Distance travelled per tick
	float distance = speed * dTime;

	// Vector to dest and zzp respectively
	Vector3 destVec = destinationPos_ - pos;

	// Move closer if we are aren't there yet
	if (destVec.length() > distance)
	{
		pos += destVec * (distance / destVec.length());
		dir.yaw = destVec.yaw();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Section: PatrolFactory
// -----------------------------------------------------------------------------

namespace
{

/**
 *	This class is used to create patrol movement controllers.
 */
class BeelineControllerFactory : public MovementFactory
{
public:
	BeelineControllerFactory() : MovementFactory( "Beeline" )
	{
	}

	/**
	 *	This method returns a patrol movement controller.
	 */
	MovementController *create( const BW::string & data, float & speed, Vector3 & position )
	{
		Vector3 destination;

		uint com1 = data.find_first_of( ',' ), com2 = data.find_last_of( ',' );

		destination.x = atof( data.substr( 0, com1 ).c_str() );
		destination.y = atof( data.substr( com1 + 1, com2 - com1 - 1 ).c_str() );
		destination.z = atof( data.substr( com2 + 1 ).c_str() );

		return new BeelineController( destination );
	}
};

BeelineControllerFactory s_beelineFactory;

} // anon namespace

}; // namespace Beeline

BW_END_NAMESPACE

// BEELINE_CONTROLLER_CPP
