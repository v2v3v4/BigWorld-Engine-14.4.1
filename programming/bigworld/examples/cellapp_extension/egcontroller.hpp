#ifndef EGCONTROLLER_HPP
#define EGCONTROLLER_HPP

#include "cellapp/controller.hpp"
#include "server/updatable.hpp"


BW_BEGIN_NAMESPACE

class EgController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( EgController );

public:
	virtual void startReal( bool isInitialStart );
	virtual void stopReal( bool isFinalStop );
	virtual void update();
};

BW_END_NAMESPACE

#endif // EGCONTROLLER_HPP
