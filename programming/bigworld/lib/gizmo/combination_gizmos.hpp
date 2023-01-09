#ifndef COMBINATION_GIZMOS_HPP
#define COMBINATION_GIZMOS_HPP

#include "gizmo_manager.hpp"
#include "position_gizmo.hpp"
#include "radius_gizmo.hpp"

BW_BEGIN_NAMESPACE

class VectorGizmo : public PositionGizmo
{
public:
	VectorGizmo( int disablerModifiers = MODIFIER_ALT, MatrixProxyPtr matrix = NULL,
		Moo::Colour arrowColor = 0xFFFFFF00, float arrowRadius = 0.1f, MatrixProxyPtr originMatrixProxy = NULL,
		float radius = 0.2f);

	bool draw( Moo::DrawContext& drawContext, bool force );
	
	void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix );	

private:
	Moo::Colour arrowColor_;
	SolidShapeMesh mesh_;
	MatrixProxyPtr originMatrixProxy_;
};


///////////////////////////////////////////////////////////////////////////////

class BoxGizmo : public Gizmo
{
public:
	BoxGizmo( MatrixProxyPtr matrix1, MatrixProxyPtr matrix2,
		Moo::Colour boxColor = 0xFF0000FF, 
		bool drawVectors = false, MatrixProxyPtr originMatrixProxy = NULL,
		int disablerModifiers = MODIFIER_ALT );
	~BoxGizmo();

	bool draw( Moo::DrawContext& drawContext, bool force );
	bool intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );

	void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix );
	Matrix objectTransform() const;

private:
	PositionGizmo * cornerGizmo1_;
	PositionGizmo * cornerGizmo2_;

	MatrixProxyPtr matrix1_;
	MatrixProxyPtr matrix2_;

	Moo::Colour boxColor_;

	MatrixProxyPtr originMatrixProxy_;
};


///////////////////////////////////////////////////////////////////////////////

class LineGizmo : public Gizmo
{
public:
	LineGizmo( MatrixProxyPtr matrix1, MatrixProxyPtr matrix2,
		Moo::Colour lineColor = 0xFF0000FF,
		bool drawVectors = false, MatrixProxyPtr originMatrixProxy = NULL,
		int disablerModifiers = MODIFIER_ALT );
	~LineGizmo();

	bool draw( Moo::DrawContext& drawContext, bool force );
	bool intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );

	void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix );
	Matrix objectTransform() const;

private:
	PositionGizmo * pointGizmo1_;
	PositionGizmo * pointGizmo2_;

	MatrixProxyPtr matrix1_;
	MatrixProxyPtr matrix2_;

	Moo::Colour lineColor_;

	MatrixProxyPtr originMatrixProxy_;
};


///////////////////////////////////////////////////////////////////////////////

class CylinderGizmo : public Gizmo
{
public:
	CylinderGizmo( MatrixProxyPtr matrix1, MatrixProxyPtr matrix2,
		FloatProxyPtr radius,
		Moo::Colour cylinderColor = 0xFFFF0000, bool drawVectors = false, 
		MatrixProxyPtr originMatrixProxy = NULL,
		float radiusGizmoRadius = 4.f,
		int disablerModifiers = MODIFIER_ALT,
		bool isPlanar = false);
	~CylinderGizmo();

	bool draw( Moo::DrawContext& drawContext, bool force );
	bool intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );

	void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix );
	Matrix objectTransform() const;

private:
	PositionGizmo * pointGizmo1_;
	PositionGizmo * pointGizmo2_;
	RadiusGizmo * radiusGizmo_;

	MatrixProxyPtr matrix1_;
	MatrixProxyPtr matrix2_;

	FloatProxyPtr radius_;

	Moo::Colour cylinderColor_;

	Matrix radiusGizmoMatrix_;
	
	MatrixProxyPtr originMatrixProxy_;
};


///////////////////////////////////////////////////////////////////////////////

class SphereGizmo : public Gizmo
{
public:
	SphereGizmo( MatrixProxyPtr matrix1, 
					FloatProxyPtr radius1, FloatProxyPtr radius2, 
					Moo::Colour cylinderColor = 0xFFFF0000,
					bool drawVectors = false,
					MatrixProxyPtr originMatrixProxy = NULL,
					int disablerModifiers = MODIFIER_ALT );
	~SphereGizmo();

	bool draw( Moo::DrawContext& drawContext, bool force );
	void drawZBufferedStuff( bool force );
	bool intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );

	void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix );
	Matrix objectTransform() const;

private:
	PositionGizmo * pointGizmo1_;
	RadiusGizmo * radiusGizmo1_;
	RadiusGizmo * radiusGizmo2_;

	MatrixProxyPtr matrix1_;
	FloatProxyPtr radius1_;
	FloatProxyPtr radius2_;

	Moo::Colour color_;

	Matrix radiusGizmoMatrix_;
	
	MatrixProxyPtr originMatrixProxy_;
};
BW_END_NAMESPACE

#endif // COMBINATION_GIZMOS_HPP
