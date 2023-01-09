#ifndef MATRIX_POSITIONER_HPP
#define MATRIX_POSITIONER_HPP

#include "always_applying_functor.hpp"
#include "math/vector3.hpp"
#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;


/*~ class Functor.MatrixPositioner
 *	@components{ tools }
 *	
 *	This class is a functor that actually moves around a matrix.
 *  (as opposed to a property, as is done above..)
 */
class MatrixPositioner : public AlwaysApplyingFunctor
{
	Py_Header( MatrixPositioner, AlwaysApplyingFunctor )
public:
	MatrixPositioner( MatrixProxyPtr pMatrix,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	PY_FACTORY_DECLARE()

protected:
	virtual void doApply( float dTime, Tool& tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	Vector3 lastLocatorPos_;
	Vector3 totalLocatorOffset_;
	bool gotInitialLocatorPos_;
	MatrixProxyPtr matrix_;
};


BW_END_NAMESPACE
#endif // MATRIX_POSITIONER_HPP
