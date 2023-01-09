#ifndef SPEED_CONTROLLER_HPP
#define SPEED_CONTROLLER_HPP

#include "cellapp/controller.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is an example controller. It works with a SpeedExtra to calculate
 *	the speed of an entity. The controller's main responsibilities are creating
 *	and destroying the SpeedExtra and streaming the SpeedExtra with the real
 *	entity.
 */
class SpeedController : public Controller
{
	DECLARE_CONTROLLER_TYPE( SpeedController );

public:
	void startReal( bool isInitialStart );
	void stopReal( bool isFinalStop );

	void startGhost();
	void stopGhost();

	void writeGhostToStream( BinaryOStream & stream );
	bool readGhostFromStream( BinaryIStream & stream );

	// Controller::cancel is protected. It really shouldn't be.
	void stop()		{ this->cancel(); }

private:
};

BW_END_NAMESPACE

#endif // SPEED_CONTROLLER_HPP
