#ifndef MATRIX_PROVIDERS_HPP
#define MATRIX_PROVIDERS_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

class PyEntity;
typedef WeakPyPtr< PyEntity > PyEntityWPtr;
typedef ScriptObjectPtr< PyEntity > PyEntityPtr;

/*~ class BigWorld.EntityDirProvider
 *  This is a MatrixProvider which provides a matrix with the direction an 
 *  entity faces. An instance of this class can be created via the factory 
 *  method BigWorld.EntityDirProvider.
 *  
 *  It is not possible to directly access many of the internal attributes of 
 *  an EntityDirProvider. To view this data it is necessary to construct a 
 *  Matrix which is a copy of this, & read the values from it. For 
 *  example:
 *  @{
 *  # create an EntityDirProvider which provides the direction faced by the 
 *  # entity anEntity
 *  edp = BigWorld.EntityDirProvider( anEntity, 1, 0 )
 *  
 *  # create a Matrix from the EntityDirProvider
 *  m = Math.Matrix( edp )
 *
 *  # print the yaw
 *  print m.yaw
 *  @}
 *  Note that a Matrix constructed as a copy of an EntityDirProvider 
 *  will only resemble the EntityDirProvider at the time that it is created. It
 *  will not update to reflect changes.
 *
 *  Instances of this class are typically used in conjunction with a Tracker to
 *  turn nodes in a model towards an entity's facing direction.
 */
/**
 * This class provides a matrix with the direction of an entity
 * It derives from MatrixProvider
 */
class EntityDirProvider: public MatrixProvider
{
	Py_Header( EntityDirProvider, MatrixProvider )
public:
	EntityDirProvider(  PyEntity * pSource,
						int pitchIndex,
						int yawIndex,
						PyTypeObject * pType = &s_type_ );
	~EntityDirProvider();

	virtual void matrix( Matrix & m ) const;
	virtual void getYawPitch( float& yaw, float& pitch) const;

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( pitchIndex_, pitchIndex)
	PY_RW_ATTRIBUTE_DECLARE( yawIndex_,   yawIndex)

	PyObject * pyGet_source();
	int pySet_source( PyObject * value );

	PY_FACTORY_DECLARE()
	//)

private:
	PyObjectPtr pSource_;
	int pitchIndex_;
	int yawIndex_;
};

/*~ class BigWorld.DiffDirProvider
 *  This is a MatrixProvider which provides a matrix representing the
 *  direction between the points specified by two matrices.
 *  An instance of this class can be created via the factory method 
 *  BigWorld.DiffDirProvider.
 *  
 *  It is not possible to directly access many of the internal attributes of a
 *  DiffDirProvider. To view this data it is necessary to construct a 
 *  Matrix which is a copy of this, & read the values from it. For 
 *  example:
 *  @{
 *  # create a DiffDirProvider which points in the same direction as the 
 *  # translation of matrix2 minus the translation of matrix1
 *  ddp = BigWorld.DiffDirProvider( matrix1, matrix2 )
 *  
 *  # create a Matrix from the DiffDirProvider
 *  m = Math.Matrix( ddp )
 *
 *  # print the yaw
 *  print m.yaw
 *  @}
 *  Note that a Matrix constructed as a copy of a DiffDirProvider will
 *  only resemble the DiffDirProvider at the time that it is created. It will 
 *  not update to reflect changes.
 *
 *  Instances of this class are typically used in conjunction with a Tracker 
 *  to cause nodes in a model to point towards something. For example, this 
 *  could be used to cause a character to turn it's head so as to look at 
 *  another object.
 */
/**
 * This class provides a matrix with the  direction between two entities.
 * It derives from MatrixProvider
 */
class DiffDirProvider: public MatrixProvider
{
	Py_Header( DiffDirProvider, MatrixProvider )
public:
	DiffDirProvider(MatrixProviderPtr pSource, MatrixProviderPtr pTarget,
		PyTypeObject * pType = &s_type_ );
	~DiffDirProvider();

	virtual void matrix( Matrix & m ) const;
	//virtual void getYawPitch( float& yaw, float& pitch) const;

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( pSource_, source )
	PY_RW_ATTRIBUTE_DECLARE( pTarget_, target )

	static PyObject * New( MatrixProviderPtr pSource, MatrixProviderPtr pTarget );
	PY_AUTO_FACTORY_DECLARE( MatrixProviderPtr, \
		NZARG( MatrixProviderPtr, NZARG( MatrixProviderPtr, END ) ) )
	//)

private:
	MatrixProviderPtr pSource_;
	MatrixProviderPtr pTarget_;
};

/*~ class BigWorld.ScanDirProvider
 *  A MatrixProvider whose direction oscillates around the y axis, and may be
 *  instructed to call a function when it hits it's leftmost and rightmost 
 *  limits. An instance of this class can be created via the factory method 
 *  BigWorld.ScanDirProvider, wherein the amplitude, period, and period offset
 *  of the oscillation are specified.
 *  
 *  It is not possible to directly access many of the internal attributes of a
 *  ScanDirProvider. To view this data it is necessary to construct a 
 *  Matrix which is a copy of this, & read the values from it. For 
 *  example:
 *  @{
 *  # create a ScanDirProvider
 *  sdp = BigWorld.ScanDirProvider( math.pi/2, 10, 0 )
 *  
 *  # create a MatrixProvider from the ScanDirProvider
 *  m = Math.Matrix( sdp )
 *
 *  # print the yaw
 *  print m.yaw
 *  @}
 *  Note that a Matrix constructed as a copy of a ScanDirProvider will
 *  only resemble the ScanDirProvider at the time that it is created. It will 
 *  not update to reflect changes.
 *
 *  Instances of this class are typically used in conjunction with a Tracker to
 *  cause a nodes in a model to "scan" an area by turning left & right.
 */
/**
 * This class provides a matrix with the direction based on the game time
 * It derives from MatrixProvider
 */
class ScanDirProvider: public MatrixProvider
{
	Py_Header( ScanDirProvider, MatrixProvider )
public:
	ScanDirProvider(float amplitude, 
					float period, 
					float offset, 
					PyObject *pHitLimitCallBack = NULL,			 
					PyTypeObject * pType = &s_type_ );
	virtual void matrix( Matrix & m ) const;
	virtual void getYawPitch( float& yaw, float& pitch) const;

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( amplitude_, amplitude)
	PY_RW_ATTRIBUTE_DECLARE( period_, period_)
	PY_RW_ATTRIBUTE_DECLARE( offset_, offset_)
	PY_FACTORY_DECLARE()
	//)

private:
	float amplitude_;
	float period_;
	float offset_;

	mutable float oldPosInCycle_;

	SmartPointer<PyObject>	pHitLimitCallBack_;

};

BW_END_NAMESPACE

#endif
