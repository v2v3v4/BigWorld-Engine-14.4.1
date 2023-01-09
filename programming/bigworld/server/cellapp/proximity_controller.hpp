#ifndef PROXIMITY_CONTROLLER_HPP
#define PROXIMITY_CONTROLLER_HPP

#include "controller.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

class ProximityRangeTrigger;

/**
 *  This class is the controller for placing proximity based triggers.
 *  or traps.
 *  When an entity crosses within a range of the source entity it
 *  will call the python script method onEnterTrap() and when
 *  an entity leaves the range, it will call the method onLeaveTrap()
 */

class ProximityController : public Controller
{
	DECLARE_CONTROLLER_TYPE( ProximityController )

public:
	ProximityController( float range = 20.f );
	virtual ~ProximityController();

	virtual void	startReal( bool isInitialStart );
	virtual void	stopReal( bool isFinalStop );

	void	writeRealToStream( BinaryOStream & stream );
	bool 	readRealFromStream( BinaryIStream & stream );

	float	range()			{ return range_; }

	void setRange( float range );

	static FactoryFnRet New( float range, int userArg = 0 );
	PY_AUTO_CONTROLLER_FACTORY_DECLARE( ProximityController,
		ARG( float, OPTARG( int, 0, END ) ) )

private:
	float	range_;
	ProximityRangeTrigger* pProximityTrigger_;

	BW::vector< EntityID > * pOnloadedSet_;
};

BW_END_NAMESPACE

#endif // PROXIMITY_CONTROLLER_HPP
