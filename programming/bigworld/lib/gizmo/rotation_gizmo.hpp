#ifndef ROTATION_GIZMO_HPP
#define ROTATION_GIZMO_HPP

#include "gizmo_manager.hpp"
#include "solid_shape_mesh.hpp"
#include "input/input.hpp"
#include "coord_mode_provider.hpp"

BW_BEGIN_NAMESPACE

typedef SmartPointer<class MatrixProxy> MatrixProxyPtr;


class RotationShapePart;

/**
 *	This class implements a gizmo for the world editor
 *	that allows interactive manipulation of the rotation
 *	part of a matrix.
 *
 *	The matrix property is provided via a MatrixProxy passed
 *	into the constructor.
 *
 *	By default, rotation is performed using the SHIFT key while
 *	an object's gizmo is visible.
 */
class RotationGizmo : public Gizmo
{
public:
	RotationGizmo( MatrixProxyPtr pMatrix,
		int enablerModifier = MODIFIER_SHIFT,
		int disablerModifier = MODIFIER_CTRL |MODIFIER_ALT );
	~RotationGizmo();

	bool draw( Moo::DrawContext& drawContext, bool force );
	bool intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );

	Matrix getCoordModifier() const;

protected:
	void init();

	Matrix	objectTransform() const;
	Matrix  objectCoord() const;

	bool					active_;
	bool					inited_;
	SolidShapeMesh			selectionMesh_;
	Moo::VisualPtr			drawMesh_;
	RotationShapePart *		currentPart_;
	MatrixProxyPtr			pMatrix_;
	Moo::Colour				lightColour_;
	int						enablerModifier_;
	int						disablerModifier_;

	RotationGizmo( const RotationGizmo& );
	RotationGizmo& operator=( const RotationGizmo& );
};

typedef SmartPointer<RotationGizmo> RotationGizmoPtr;

BW_END_NAMESPACE
#endif // ROTATION_GIZMO_HPP
