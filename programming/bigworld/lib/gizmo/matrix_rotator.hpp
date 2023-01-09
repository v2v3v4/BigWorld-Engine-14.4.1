#ifndef MATRIX_ROTATOR_HPP
#define MATRIX_ROTATOR_HPP

#include "always_applying_functor.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"
#include "property_rotater_helper.hpp"

BW_BEGIN_NAMESPACE

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;

/*~ class Functor.MatrixRotator
 *	@components{ tools }
 *	
 *	This class is a functor that rotates a chunk item, or anything
 *	else that can return a matrix.
 */
class MatrixRotator : public AlwaysApplyingFunctor
{
	Py_Header( MatrixRotator, AlwaysApplyingFunctor )
public:
	MatrixRotator( MatrixProxyPtr pMatrix,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	PY_FACTORY_DECLARE()

protected:
	void doApply( float dTime, Tool& tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	bool					grabOffsetSet_;
	Matrix					invGrabOffset_;
	Vector3					initialToolLocation_;
	PropertyRotaterHelper	rotaterHelper_;
};

BW_END_NAMESPACE
#endif // MATRIX_ROTATOR_HPP
