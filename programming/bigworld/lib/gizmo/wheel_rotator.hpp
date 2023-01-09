#ifndef WHEEL_ROTATOR_HPP
#define WHEEL_ROTATOR_HPP

#include "always_applying_functor.hpp"
#include "property_rotater_helper.hpp"

BW_BEGIN_NAMESPACE

class GenRotationProperty;

/*~ class NoModule.WheelRotator
 *	@components{ tools }
 *
 *	Rotates the current GenRotationProperty via the mousewheel
 */
class WheelRotator : public AlwaysApplyingFunctor
{
	Py_Header( WheelRotator, AlwaysApplyingFunctor )
public:
	WheelRotator( bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	virtual void update( float dTime, Tool& tool );
	virtual bool handleKeyEvent( const KeyEvent & event, Tool& tool );
	virtual bool handleMouseEvent( const MouseEvent & event, Tool& tool );

	PY_FACTORY_DECLARE()

protected:
	virtual bool checkStopApplying() const;
	virtual void doApply( float dTime, Tool & tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	float					timeSinceInput_;
	float					rotAmount_;
	bool					needsInit_;
	BW::vector< GenRotationProperty * > curProperties_;

	PropertyRotaterHelper	rotaterHelper_;

	bool arePropertiesValid() const;

	/** Programmatically add rotation */
	void rotateBy( float degs, bool useLocalYaxis );
};

BW_END_NAMESPACE
#endif // WHEEL_ROTATOR_HPP
