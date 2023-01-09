#ifndef TURN_CONTROLLER_HPP
#define TURN_CONTROLLER_HPP

#include "controller.hpp"
#include "pyscript/script.hpp"
#include "network/basictypes.hpp"
#include "server/updatable.hpp"
#include "math/angle.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer< Entity > EntityPtr;

/**
 *  This class is the controller for an entity that is turning at
 *  a specific rate of angle/second
 */
class YawRotatorController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( YawRotatorController )

public:
	YawRotatorController( float targetYaw = 0.f,
		float velocity = 0.f	 // in radians per second
		);

	virtual void		startReal( bool isInitialStart );
	virtual void		stopReal( bool isFinalStop );

	bool				turn();

	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );

	void				update();

	static FactoryFnRet New( float targetYaw = 0.f,
						float velocity = 3.f,	 // in radians per second
						int userArg = 0 );

	PY_AUTO_CONTROLLER_FACTORY_DECLARE( YawRotatorController,
		ARG( float, OPTARG( float, 3.f, OPTARG( int, 0, END ) ) ) )

protected:
	float				radiansPerTick_; // amount of angle updated per tick
	Angle				targetYaw_; // target yaw
};

BW_END_NAMESPACE

#endif // TURN_CONTROLLER_HPP
