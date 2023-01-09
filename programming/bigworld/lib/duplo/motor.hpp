#ifndef MOTOR_HPP
#define MOTOR_HPP


#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

class PyModel;

/*~ class BigWorld.Motor
 *
 *	This is the abstract base class for objects that change a models position
 *	and orientation.
 *	Models contain a list of motors, and each tick every motor gets a chance to
 *  update the position and rotation of the model, as well as the animation
 *  that the model is playing.  Without any motors, Models do not follow
 *	the entity, but remain at their initial position and orientation.
 *
 *	By default, a newly created Model has one Motor, which is an ActionMatcher.
 */
/**
 *	This is a base class for objects that can move a model around.
 *	When it is attached, it stores a model pointer; however, it
 *	doesn't keep a reference to it because that would be a circular
 *	reference.
 *
 *	@see PyModel
 */
class Motor : public PyObjectPlus
{
	Py_Header( Motor, PyObjectPlus )

public:
	Motor( PyTypeObject * pType ) : PyObjectPlus( pType ), pOwner_( NULL ) {}
		// no default argument as this is an abstract class

	virtual void attach( PyModel * pOwner )
		{ pOwner_ = pOwner; this->attached(); }

	virtual void detach()
		{ pOwner_ = NULL; this->detached(); }

	virtual void onOwnerReloaded(){}

	/**
	 *	Call this function to turn the motor one frame. It should
	 *	only be called by the owning PyModel.
	 */
	virtual void rev( float dTime ) = 0;

	PyObject * pyGet_owner();
	PY_RO_ATTRIBUTE_SET( owner );

	PyModel * pOwner() { return pOwner_; }

protected:
	virtual ~Motor() {}
	virtual void attached() {}
	virtual void detached()	{}

	PyModel * pOwner_;	///< The model that owns us, or NULL.
};

BW_END_NAMESPACE

#endif // MOTOR_HPP
