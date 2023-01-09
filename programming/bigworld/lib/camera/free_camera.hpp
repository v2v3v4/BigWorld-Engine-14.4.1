#ifndef FREE_CAMERA_HPP
#define FREE_CAMERA_HPP


#include "base_camera.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.FreeCamera
 *
 *	A FreeCamera is a camera that can be moved around using the mouse and arrow
 *	keys.  It doens't have any ties to entities, and doesn't do any physics or
 *  geometry checks, so it can be used to easily explore the world.
 *
 *	It processes both mouse and keyboard events, and consumes the events it
 *  uses.  It consumes: all mouse events, and all key events involving the
 *	arrow keys.
 *
 *
 *	It inherits from BaseCamera.
 *
 *	A FreeCamera can be created using the BigWorld.FreeCamera function.
 */
/**
 *	This class is a camera that can be moved around freely by the mouse
 */
class FreeCamera : public BaseCamera
{
	Py_Header( FreeCamera, BaseCamera )

public:
	FreeCamera( PyTypeObject * pType = &s_type_ );
	~FreeCamera();

	virtual bool handleKeyEvent( const KeyEvent & ev );
	virtual bool handleMouseEvent( const MouseEvent & event );
	virtual bool handleAxisEvent( const AxisEvent & event );

	virtual void set( const Matrix & viewMatrix );
	virtual void update( float dTime );

	PY_RW_ATTRIBUTE_DECLARE( fixed_, fixed )
	PY_RW_ATTRIBUTE_DECLARE( invViewProvider_, invViewProvider )

	PY_FACTORY_DECLARE()

private:
	FreeCamera( const FreeCamera& );
	FreeCamera& operator=( const FreeCamera& );

	MatrixProviderPtr	invViewProvider_;
	bool		fixed_;
};

BW_END_NAMESPACE


#endif // FREE_CAMERA_HPP
