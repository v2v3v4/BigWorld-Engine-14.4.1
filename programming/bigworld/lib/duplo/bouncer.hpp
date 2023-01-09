#ifndef BOUNCER_HPP
#define BOUNCER_HPP

#include "motor.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.Bouncer
 *
 *	Bouncer is a Motor that causes the model to move with an initial velocity 
 *	(vx, vy, vz) in a balistic trajectory, following the laws of motion and
 *	bouncing against the collision scene until it eventually comes to rest. 
 *
 *	It is designed to work in a lagged network environment so that the player 
 *	originally activating the bouncer can get immediate feedback, being able to 
 *	see the balistic projectile without having to wait for the server to return 
 *	its correct final path, while still getting an end result that is consistent 
 *	with that on the server and all witnessing clients.
 *
 *	This is achieved by allowing the original client to estimate the object's
 *	path (using estimatePath) and start animating it right away. When the final 
 *	authoritative trajectory arrives from the server, the two paths are blended 
 *	together over time (using updatePath), so that the final position on the
 *	client is the same as the one returned by the server.
 *
 *	Clients witnessing the balistic object will animate the server-computed 
 *	trajectory from the begining (using calcPath).
 *
 *	Example: 
 *	@{
 *		def throwGrenade(self):
 *			# when throwing the grenade
 *			motor = BigWorld.Bouncer()
 *			motor.vx = throwSpeed[0]
 *			motor.vy = throwSpeed[1]
 *			motor.vz = throwSpeed[2]
 *			motor.tripTime  = Grenade.FUSETIME # no point in simulating past explosion time
 *			motor.timeSlice = 1.0 / 30.0
 *			motor.estimatePath( owner.position, ELASTICITY, RADIUS, MAX_BOUNCES)
 *	
 *			model = BigWorld.Model(GRENADE_MODEL)
 *			model.motors = [motor]
 *			(...)
 *
 *		def enterWorld(self):
 *			if self.ownerId == BigWorld.player().id:
 *				# update existing motor
 *				(...)
 *				model.motors[0].updatePath(
 *					self.clientSourcePos, 
 *					self.serverSourcePos,
 *					self.destinationPos)
 *			else:
 *				# witnessing grenade being thrown
 *				(...)
 *				motor = BigWorld.Bouncer()
 *				motor.vx = self.throwSpeed[0]
 *				motor.vy = self.throwSpeed[1]
 *				motor.vz = self.throwSpeed[2]
 *				motor.tripTime  = self.fusetime 
 *				motor.timeSlice = 1.0 / 30.0
 *				motor.calcPath( 
 *					currentOwnerPosition, self.serverSourcePos,
 *					self.destPos, ELASTICITY, RADIUS, MAX_BOUNCES)
 *	@}
 */
/**
 *	Bouncer is a Motor that causes the model to move with an initial velocity 
 *	(vx, vy, vz) in a balistic trajectory, following the laws of motion and
 *	bouncing against the collision scene until it eventually comes to rest. 
 */
class Bouncer : public Motor
{
	Py_Header( Bouncer, Motor )

public:
	Bouncer( PyTypeObject * pType = &Bouncer::s_type_ );
	~Bouncer();

	virtual void rev( float dTime );

	PY_FACTORY_DECLARE()

	PY_AUTO_METHOD_DECLARE( RETOWN, calcPath, ARG( Vector3, ARG( Vector3, 
									 ARG( Vector3, ARG( float, ARG( float,
									 OPTARG( int, -1, END ) ) ) ) ) ) )
	PyObject * calcPath( const Vector3 & clientSourcePos, 
							const Vector3 & serverSourcePos,
							const Vector3 & destPos, float elasticity,
							float radius, int maxBounces );
	PY_AUTO_METHOD_DECLARE( RETOWN, estimatePath, ARG( Vector3,
									 ARG( float, ARG( float,
									 OPTARG( int, -1, END ) ) ) ) )
	PyObject * estimatePath( const Vector3 & clientSourcePos,
							float elasticity, float radius,
							int maxBounces );
	PY_AUTO_METHOD_DECLARE( RETOWN, updatePath, ARG( Vector3,
									 ARG( Vector3, END ) ) )
	PyObject * updatePath( const Vector3 & serverSourcePos,
							const Vector3 & destPos );

	PY_RW_ATTRIBUTE_DECLARE( vx_, vx )
	PY_RW_ATTRIBUTE_DECLARE( vy_, vy )
	PY_RW_ATTRIBUTE_DECLARE( vz_, vz )

	PY_RW_ATTRIBUTE_DECLARE( tripTime_, tripTime )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, timeSlice, timeSlice )

	PY_RO_ATTRIBUTE_DECLARE(
		(!pathSteps_.empty()) ? pathSteps_.back().pos_ : Vector3::zero(), destPos )

private:
	MatrixProviderPtr	target_;

	float			tripTime_;

	float			vx_;
	float			vy_;
	float			vz_;
	float			acc_;
	float			elasticity_;
	float			radius_;

	float			timeSlice_;
	float			totaldTime_;
	int				lastSlice_;		// last slice played (valid during playback only)
	int				maxBounces_;

	float timeSlice() const { return timeSlice_; }
	void timeSlice( float slice );

	struct PathInfo {
		Vector3 pos_;
		float bounceVel_;	// if non-zero it bounced at this step
	};
	BW::vector<PathInfo> pathSteps_;	// stored path info

	void makePCBHappen();
};

BW_END_NAMESPACE

#endif // BOUNCER_HPP
