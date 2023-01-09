#ifndef BASE_CAMERA_HPP
#define BASE_CAMERA_HPP

#include "moo/moo_math.hpp"
#include "math/boundbox.hpp"
#include "input/input.hpp"
#include "input/py_input.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

/*~ class NoModule.BaseCamera
 *	@components{ tools }
 *
 *	This class is used to keep track of the camera inside the World Editor. 
 *	World Editor uses different camera types, and you can obtain these cameras
 *	by using the WorldEditor.camera function, which will return a camera type
 *	derived from this BaseCamera class.
 */
class BaseCamera : public PyObjectPlus
{
	Py_Header( BaseCamera, PyObjectPlus )

public:
	BaseCamera( PyTypeObject * pType = &s_type_ );
	virtual ~BaseCamera();

	//This method must calculate the view matrix
	virtual void update( float dTime, bool activeInputHandler = true ) = 0;
	virtual void render( float dTime );
	virtual bool handleKeyEvent( const KeyEvent & );
	virtual bool handleMouseEvent( const MouseEvent & );

    //common tool camera properties
    float	speed() const;
    void	speed( float s );

	float	turboSpeed() const;
    void	turboSpeed( float s );

    bool	invert() const;
    void	invert( bool state );

	const BoundingBox & limit() const;
	void limit( const BoundingBox & );
	

	virtual const Matrix & view() const;
	virtual void view( const Matrix & );

	virtual Vector3 position() const;
	virtual void position( const Vector3 & );

    void windowHandle( const HWND handle );

	PY_AUTO_METHOD_DECLARE( RETVOID, update, ARG( float, OPTARG( bool, true, END ) ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, render, ARG( float, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleKeyEvent, ARG( KeyEvent, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleMouseEvent, ARG( MouseEvent, END ) )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, speed, speed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, turboSpeed, turboSpeed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, invert, invert )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Matrix, view, view )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, position, position )
	PY_RO_ATTRIBUTE_DECLARE( limit().minBounds(), limitMin )
	PY_RO_ATTRIBUTE_DECLARE( limit().maxBounds(), limitMax )

protected:
	Matrix	view_;
    float	speed_[2];
    bool	invert_;

    HWND	windowHandle_;

private:
	BaseCamera(const BaseCamera&);
	BaseCamera& operator=(const BaseCamera&);

	// the extents the camera may move to
	BoundingBox limit_;
};

typedef SmartPointer<BaseCamera> BaseCameraPtr;

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "base_camera.ipp"
#endif

#endif // BASE_CAMERA_HPP
