#ifndef PROPERTY_ROTATER_HELPER_HPP
#define PROPERTY_ROTATER_HELPER_HPP

#include "math/matrix.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

class GenPositionProperty;
class GenRotationProperty;

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;

/**
 * This helper class modifies all the current rotation properties
 * based on the current gizmo
 * It also listens to the currently selected coordinate system
 * this means that if you are working in local coordinates the
 * rotations will be appplied relative to their origin, otherwise
 * the rotations are applied relative to the weighted centre of 
 * the current rotation properties
 */
class PropertyRotaterHelper
{
public:
	PropertyRotaterHelper();

	void init( const Matrix& gizmoTransform );
	void updateRotation( float angle, const Vector3& axis );
	void fini( bool success );

private:

	struct PropInfo
	{
		GenRotationProperty* prop_;
		Matrix			initialMatrix_;
	};

	BW::vector<PropInfo> props_;

	struct PositionOnlyPropInfo
	{
		GenPositionProperty* prop_;
		Matrix			initialMatrix_;
	};

	BW::vector<PositionOnlyPropInfo> positionOnlyProps_;

	Vector3				groupOrigin_;
	Matrix				groupFrame_;
	Matrix				invGroupFrame_;

	// Helper class to find a Propinfo structure
	// by its MatrixProxyPtr
	class PropFinder
	{
	public:
		PropFinder( MatrixProxyPtr pMatrix );
		bool operator () ( PropInfo& prop );
	private:
		MatrixProxyPtr pMatrix_;

	};
};

BW_END_NAMESPACE
#endif // PROPERTY_ROTATER_HELPER_HPP
