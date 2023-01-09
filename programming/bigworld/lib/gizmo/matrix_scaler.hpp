#ifndef MATRIX_SCALER_HPP
#define MATRIX_SCALER_HPP

#include "always_applying_functor.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;
class FloatProxy;
typedef SmartPointer<FloatProxy> FloatProxyPtr;

/*~ class Functor.MatrixScaler
 *	@components{ tools }
 *
 *	This class is a functor that scales a chunk item, or anything
 *	else that can return a matrix.
 */
class MatrixScaler : public AlwaysApplyingFunctor
{
	Py_Header( MatrixScaler, AlwaysApplyingFunctor )
public:
	MatrixScaler( MatrixProxyPtr pMatrix,
		float scaleSpeedFactor = 1.f,
		FloatProxyPtr scaleX = NULL,
		FloatProxyPtr scaleY = NULL,
		FloatProxyPtr scaleZ = NULL,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	PY_FACTORY_DECLARE()

protected:
	virtual void doApply( float dTime, Tool& tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	MatrixProxyPtr	pMatrix_;
	Matrix			initialMatrix_;
	Matrix			invInitialMatrix_;
	Vector3			initialScale_;
	Vector3			grabOffset_;
	bool			grabOffsetSet_;
	float			scaleSpeedFactor_;
	FloatProxyPtr	scaleX_;
	FloatProxyPtr	scaleY_;
	FloatProxyPtr	scaleZ_;
};

BW_END_NAMESPACE
#endif // MATRIX_SCALER_HPP
