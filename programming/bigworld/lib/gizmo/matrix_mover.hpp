#ifndef MATRIX_MOVER_HPP
#define MATRIX_MOVER_HPP

#include "always_applying_functor.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "snap_provider.hpp"

BW_BEGIN_NAMESPACE

class GenPositionProperty;

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;


/*~ class Functor.MatrixMover
 *	@components{ tools }
 *	
 *	This class is a functor that moves around a chunk item, or anything
 *	else that can return a matrix.
 */
class MatrixMover : public AlwaysApplyingFunctor
{
	Py_Header( MatrixMover, AlwaysApplyingFunctor )
public:
	MatrixMover( MatrixProxyPtr pMatrix,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );

	MatrixMover( MatrixProxyPtr pMatrix,
		bool snap,
		bool rotate,
		bool allowedToDiscardChanges = true,
		PyTypeObject * pType = &s_type_ );
	~MatrixMover();

	static bool moving();

	PY_FACTORY_DECLARE()

protected:
	virtual bool checkStopApplying() const;
	virtual void doApply( float dTime, Tool& tool );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

private:
	SnapProvider::SnapMode snapMode_;
	Vector3 lastLocatorPos_;
	Vector3 totalLocatorOffset_;
	bool gotInitialLocatorPos_;
	bool snap_;
	bool rotate_;
	static int moving_;
};

BW_END_NAMESPACE
#endif // MATRIX_MOVER_HPP
