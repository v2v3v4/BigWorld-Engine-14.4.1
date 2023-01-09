#ifndef PROJECTION_ACCESS_HPP
#define PROJECTION_ACCESS_HPP


#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.ProjectionAccess
 *
 *	This class exposes the projection matrix, and allows manipulation of the
 *  clipping planes and the field of view.
 *
 *	There is one projection matrix for the client, which is accessed using 
 *	BigWorld.projection().  This is automatically created by BigWorld.
 *
 */
/**
 *	This class manages the projection matrix, and allows script access
 *	to its parameters.
 */
class ProjectionAccess : public PyObjectPlusWithWeakReference
{
	Py_Header( ProjectionAccess, PyObjectPlusWithWeakReference )

public:
	ProjectionAccess( PyTypeObject * pType = &s_type_ );
	~ProjectionAccess();

	void update( float dTime );

	void rampFov( float val, float time );

	float nearPlane() const;
	void nearPlane( float val );

	float farPlane() const;
	void farPlane( float val );

	float fov() const;
	void fov( float val );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, nearPlane, nearPlane )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, farPlane, farPlane )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, fov, fov )

	PY_AUTO_METHOD_DECLARE( RETVOID, rampFov, ARG( float, ARG( float, END ) ) )

private:
	ProjectionAccess( const ProjectionAccess& );
	ProjectionAccess& operator=( const ProjectionAccess& );

	bool	smoothFovTransition_;

	float	fovTransitionStart_;
	float	fovTransitionEnd_;
	float	fovTransitionTime_;
	float	fovTransitionTimeElapsed_;
};

typedef SmartPointer<ProjectionAccess> ProjectionAccessPtr;

PY_SCRIPT_CONVERTERS_DECLARE( ProjectionAccess )


#ifdef CODE_INLINE
#include "projection_access.ipp"
#endif

BW_END_NAMESPACE

#endif // PROJECTION_ACCESS_HPP
