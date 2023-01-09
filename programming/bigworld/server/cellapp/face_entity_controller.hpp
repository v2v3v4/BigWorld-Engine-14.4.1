#ifndef FACE_ENTITY_CONTROLLER_HPP
#define FACE_ENTITY_CONTROLLER_HPP

#include "controller.hpp"
#include "network/basictypes.hpp"
#include "server/updatable.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer< Entity > EntityPtr;

/**
 *  This class is the controller for an entity that is turning at a specific
 *  rate of angle/second.
 */
class FaceEntityController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( FaceEntityController )

public:
	FaceEntityController( int entityId = 0,
		float velocity = 2*MATH_PI, // in radians per second
		int period = 10 );

	// Overrides from Controller
	virtual void		startReal( bool isInitialStart );
	virtual void		stopReal( bool isFinalStop );

	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );

	void				update();

protected:
	EntityPtr  pTargetEntity_;
	int		targetEntityId_;
	float	radiansPerTick_;
	int		ticksToNextUpdate_;
	int		period_;
};

BW_END_NAMESPACE

#endif // FACE_ENTITY_CONTROLLER_HPP
