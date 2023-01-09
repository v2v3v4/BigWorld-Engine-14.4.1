#ifndef PROPERTY_SCALER_HELPER_HPP
#define PROPERTY_SCALER_HELPER_HPP

#include "math/matrix.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

class GenScaleProperty;
class GenPositionProperty;

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;

/**
 * This helper class modifies all the current scale properties
 * based on the current gizmo
 * It also listens to the currently selected coordinate system
 * this means that if you are working in local coordinates the
 * scales will be appplied relative to their origin, otherwise
 * the scales are applied relative to the centre of the current 
 * scale properties
 */
class PropertyScalerHelper
{
public:
	PropertyScalerHelper();

	void init( const Matrix& gizmoTransform );
	void updateScale( const Vector3& scale );
	void fini( bool success );

private:

	struct PropInfo
	{
		GenScaleProperty* prop_;
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
#endif // PROPERTY_SCALER_HELPER_HPP
