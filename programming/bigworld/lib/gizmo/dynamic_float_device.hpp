#ifndef DYNAMIC_FLOAT_DEVICE_HPP
#define DYNAMIC_FLOAT_DEVICE_HPP

#include "always_applying_functor.hpp"

#include "math/matrix.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;
class FloatProxy;
typedef SmartPointer<FloatProxy> FloatProxyPtr;

/*~ class Functor.DynamicFloatDevice
 *	@components{ tools }
 *	
 *	This class is a functor that alters any float property.
 */
class DynamicFloatDevice : public AlwaysApplyingFunctor
{
	Py_Header( DynamicFloatDevice, AlwaysApplyingFunctor )
public:
	DynamicFloatDevice( MatrixProxyPtr pCenter,
		FloatProxyPtr pFloat,
		float adjFactor,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	PY_FACTORY_DECLARE()

protected:
	virtual void doApply( float dTime, Tool & tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	MatrixProxyPtr	pCenter_;
	FloatProxyPtr	pFloat_;
	Matrix			initialCenter_;
	float			initialFloat_;
	Vector3			grabOffset_;
	bool			grabOffsetSet_;
	float			adjFactor_;
};

BW_END_NAMESPACE
#endif // DYNAMIC_FLOAT_DEVICE_HPP
