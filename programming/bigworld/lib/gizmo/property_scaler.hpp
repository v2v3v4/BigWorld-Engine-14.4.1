#ifndef PROPERTY_SCALER_HPP
#define PROPERTY_SCALER_HPP

#include "always_applying_functor.hpp"
#include "math/matrix.hpp"
#include "property_scaler_helper.hpp"

BW_BEGIN_NAMESPACE

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;
class FloatProxy;
typedef SmartPointer<FloatProxy> FloatProxyPtr;

/*~ class NoModule.PropertyScaler
 *	@components{ tools }
 *	
 *	This scales all the current scale properties
 */
class PropertyScaler : public AlwaysApplyingFunctor
{
	Py_Header( PropertyScaler, AlwaysApplyingFunctor )
public:
	PropertyScaler( float scaleSpeedFactor = 1.f,
		FloatProxyPtr scaleX = NULL,
		FloatProxyPtr scaleY = NULL,
		FloatProxyPtr scaleZ = NULL,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

protected:
	virtual void doApply( float dTime, Tool & tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	float scaleSpeedFactor_;

	FloatProxyPtr scaleX_;
	FloatProxyPtr scaleY_;
	FloatProxyPtr scaleZ_;

	bool					grabOffsetSet_;
	Matrix					invGrabOffset_;

	PropertyScalerHelper	scalerHelper_;

};

BW_END_NAMESPACE
#endif // PROPERTY_SCALER_HPP
