#include "pch.hpp"

#include "combination_gizmos.hpp"
#include "moo/geometrics.hpp"
#include "moo/visual_manager.hpp"
#include "moo/dynamic_index_buffer.hpp"

BW_BEGIN_NAMESPACE

static float gizmo_axis_radius = 0.1f;

VectorGizmo::VectorGizmo( int disablerModifiers, MatrixProxyPtr matrix, Moo::Colour arrowColor, 
						 float arrowRadius, MatrixProxyPtr originMatrixProxy, float radius )
	: PositionGizmo(disablerModifiers, matrix, originMatrixProxy, radius)
	, arrowColor_(arrowColor)
	, originMatrixProxy_(originMatrixProxy)
{
	BW_GUARD;

	// add a fixed sized mesh and scale later
	mesh_.transform( Matrix::identity );

	const float arrowLength = 0.2f;
	const float shaftLength = 1.f - arrowLength;
	mesh_.addCylinder( Vector3( 0, 0, 0 ), arrowRadius, shaftLength, true, false, arrowColor_, NULL);
	mesh_.addCone( Vector3( 0, 0, shaftLength ), arrowRadius*2.f, arrowLength, true, arrowColor_, NULL);
}

void VectorGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	BW_GUARD;

	originMatrixProxy_ = matrix;
	PositionGizmo::setVisualOffsetMatrixProxy(originMatrixProxy_);
}


bool VectorGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	if (!PositionGizmo::draw( drawContext, force ))
		return false;

	// create an arrow
	Matrix mat = objectTransform();
	Vector3 worldPosition = mat.applyToOrigin();

	// check for valid position
	if (worldPosition == Vector3::zero())
		return true;

	// draw the arrow
	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
//	rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
	Moo::Material::setVertexColour();

	rc.setRenderState( D3DRS_LIGHTING, FALSE );
	rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
	rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	uint32 tfactor = arrowColor_;
	rc.setRenderState( D3DRS_TEXTUREFACTOR, tfactor );

	// set the mesh matrix s.t. z axis is along the direction we want
	// offset the local origin by that specified in originMatrixProxy_
	Vector3 localOrigin;
	localOrigin.setZero();
	if (originMatrixProxy_)
	{
		Matrix mat;
		originMatrixProxy_->getMatrix(mat);
		localOrigin = mat.applyToOrigin();
	}

	// worldPosition has already been shifted by the offset, have to undo this..
	worldPosition -= localOrigin;

	Vector3 rightAngleVector;
	if ( ( worldPosition.x == 0 ) && ( worldPosition.y == 0 ) )
		rightAngleVector = Vector3(worldPosition.x, -worldPosition.z, worldPosition.y);
	else
		rightAngleVector = Vector3(worldPosition.y, -worldPosition.x, worldPosition.z);

	Vector3 worldPositionNormalised = worldPosition;
	worldPositionNormalised.normalise();
	Matrix m;
	m.row(3, Vector4(localOrigin, 1.f) );
	m.row(2, Vector4(worldPositionNormalised, 0.f) );
	m.row(1, Vector4(rightAngleVector, 0.f) );
	Vector4 row1 = m.row(1); row1.normalise();
	m.row(1, row1 );
	m.row(0, Vector4(rightAngleVector.crossProduct(worldPositionNormalised), 0.f) );
	Vector4 row0 = m.row(0); row0.normalise();
	m.row(0, row0 );

	const float scale = worldPosition.length();
	Matrix scaleMatrix;
	scaleMatrix.setScale(1.f, 1.f, scale);
	m.preMultiply(scaleMatrix);

	pDev->SetTransform( D3DTS_WORLD, &m );
	pDev->SetTransform( D3DTS_VIEW, &rc.view() );
	pDev->SetTransform( D3DTS_PROJECTION, &rc.projection() );
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZND::fvf() );

	Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
	Moo::IndicesReference ind = dib.lock( (uint32)mesh_.indices().size() );
	if (ind.size())
	{
		ind.fill( &mesh_.indices().front(), (uint32)mesh_.indices().size() );

		dib.unlock();
		uint32 ibLockIndex = dib.lockIndex();

		typedef Moo::VertexXYZND VertexType;
		int vertexSize = sizeof(VertexType);

		Moo::DynamicVertexBufferBase2 & dvb = 
			Moo::DynamicVertexBufferBase2::instance( vertexSize );

		uint32 vbLockIndex = 0;
		if ( dvb.lockAndLoad( &mesh_.verts().front(), 
			static_cast<uint32>(mesh_.verts().size()), vbLockIndex ) &&
			 SUCCEEDED(dvb.set()) &&
			 SUCCEEDED(dib.indexBuffer().set()) )
		{
			rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, vbLockIndex, 0, (uint32)(mesh_.verts().size()),
				ibLockIndex, (uint32)(mesh_.indices().size()) / 3 );
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////


BoxGizmo::BoxGizmo(MatrixProxyPtr matrix1, MatrixProxyPtr matrix2, Moo::Colour boxColor, 
				   bool drawVectors, MatrixProxyPtr originMatrixProxy, int disablerModifiers )
	: matrix1_(matrix1)
	, matrix2_(matrix2)
	, boxColor_(boxColor)
	, originMatrixProxy_(originMatrixProxy)
{
	BW_GUARD;

	if (drawVectors)
	{
		cornerGizmo1_ = new VectorGizmo(disablerModifiers, matrix1_, boxColor, 0.015f, originMatrixProxy, gizmo_axis_radius);
		cornerGizmo2_ = new VectorGizmo(disablerModifiers, matrix2_, boxColor, 0.015f, originMatrixProxy, gizmo_axis_radius);
	}
	else
	{
		cornerGizmo1_ = new PositionGizmo(disablerModifiers, matrix1_, originMatrixProxy, gizmo_axis_radius);
		cornerGizmo2_ = new PositionGizmo(disablerModifiers, matrix2_, originMatrixProxy, gizmo_axis_radius);
	}
}

BoxGizmo::~BoxGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(cornerGizmo1_);
	GizmoManager::instance().removeGizmo(cornerGizmo2_);
}

void BoxGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	BW_GUARD;

	originMatrixProxy_ = matrix;
	cornerGizmo1_->setVisualOffsetMatrixProxy(originMatrixProxy_);
	cornerGizmo2_->setVisualOffsetMatrixProxy(originMatrixProxy_);
}

Matrix BoxGizmo::objectTransform() const
{
	BW_GUARD;

	if (originMatrixProxy_)
		return originMatrixProxy_->get();
	static Matrix ret( Matrix::identity );	
	return ret;
}

bool BoxGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	cornerGizmo1_->draw( drawContext, force );
	cornerGizmo2_->draw( drawContext, force );

	// draw the box
	Matrix m;
	Vector3 offset = Vector3::zero();
	if (originMatrixProxy_)
	{
		Matrix offsetMatrix;
		originMatrixProxy_->getMatrix(offsetMatrix);
		offset = offsetMatrix.applyToOrigin();
	}

	matrix1_->getMatrix(m);
	Vector3 point1 = m.applyToOrigin() + offset;
	matrix2_->getMatrix(m);
	Vector3 point2 = m.applyToOrigin() + offset;

	BoundingBox boundingBox;
	boundingBox.addBounds( point1 );
    boundingBox.addBounds( point2 );

	Geometrics::wireBox(boundingBox, boxColor_);

	return true;
}

bool BoxGizmo::intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force )
{
	BW_GUARD;

	bool result = cornerGizmo1_->intersects( origin, direction, t, force );
	return result || cornerGizmo2_->intersects( origin, direction, t, force );
}

void BoxGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	cornerGizmo1_->click(origin, direction);
	cornerGizmo2_->click(origin, direction);
}

void BoxGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	cornerGizmo1_->rollOver(origin, direction);
	cornerGizmo2_->rollOver(origin, direction);
}


///////////////////////////////////////////////////////////////////////////////


LineGizmo::LineGizmo( MatrixProxyPtr matrix1, MatrixProxyPtr matrix2, Moo::Colour lineColor,
					 bool drawVectors, MatrixProxyPtr originMatrixProxy, int disablerModifiers )
	: matrix1_(matrix1)
	, matrix2_(matrix2)
	, lineColor_(lineColor)
	, originMatrixProxy_(originMatrixProxy)
{
	BW_GUARD;

	if (drawVectors)
	{
		pointGizmo1_ = new VectorGizmo(disablerModifiers, matrix1_, lineColor, 0.015f, originMatrixProxy, gizmo_axis_radius);
		pointGizmo2_ = new VectorGizmo(disablerModifiers, matrix2_, lineColor, 0.015f, originMatrixProxy, gizmo_axis_radius);
	}
	else
	{
		pointGizmo1_ = new PositionGizmo(disablerModifiers, matrix1_, originMatrixProxy, gizmo_axis_radius);
		pointGizmo2_ = new PositionGizmo(disablerModifiers, matrix2_, originMatrixProxy, gizmo_axis_radius);
	}
}

LineGizmo::~LineGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(pointGizmo1_);
	GizmoManager::instance().removeGizmo(pointGizmo2_);
}

void LineGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	BW_GUARD;

	originMatrixProxy_ = matrix;
	pointGizmo1_->setVisualOffsetMatrixProxy(originMatrixProxy_);
	pointGizmo2_->setVisualOffsetMatrixProxy(originMatrixProxy_);
}

Matrix LineGizmo::objectTransform() const
{
	BW_GUARD;

	if (originMatrixProxy_)
		return originMatrixProxy_->get();
	static Matrix ret( Matrix::identity );	
	return ret;
}

bool LineGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	if (!pointGizmo1_->draw( drawContext, force ))
		return false;

	pointGizmo2_->draw( drawContext, force );

	// draw the line
	Matrix m;

	matrix1_->getMatrix(m);
    Vector3 start = m.applyToOrigin();

	matrix2_->getMatrix(m);
    Vector3 end = m.applyToOrigin();

	if (originMatrixProxy_)
	{
		Matrix offsetMatrix;
		originMatrixProxy_->getMatrix(offsetMatrix);
		Vector3 offset = offsetMatrix.applyToOrigin();
		start += offset;
		end += offset;
	}

	Geometrics::drawLine(start, end, lineColor_);

	return true;
}

bool LineGizmo::intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force )
{
	BW_GUARD;

	bool result = pointGizmo1_->intersects( origin, direction, t, force );
	return result || pointGizmo2_->intersects( origin, direction, t, force );
}

void LineGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	pointGizmo1_->click(origin, direction);
	pointGizmo2_->click(origin, direction);
}

void LineGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	pointGizmo1_->rollOver(origin, direction);
	pointGizmo2_->rollOver(origin, direction);
}


///////////////////////////////////////////////////////////////////////////////


CylinderGizmo::CylinderGizmo( MatrixProxyPtr matrix1, MatrixProxyPtr matrix2,
							 FloatProxyPtr radius, Moo::Colour cylinderColor,
							 bool drawVectors, MatrixProxyPtr originMatrixProxy,
							 float radiusGizmoRadius,
							 int disablerModifiers,
							 bool isPlanar)
	: matrix1_(matrix1)
	, matrix2_(matrix2)
	, radius_(radius)
	, cylinderColor_(cylinderColor)
	, radiusGizmoMatrix_(Matrix::identity)
	, originMatrixProxy_(originMatrixProxy)
	, pointGizmo1_(NULL)
	, pointGizmo2_(NULL)
{
	BW_GUARD;

	if (drawVectors)
	{
		pointGizmo1_ = new VectorGizmo(disablerModifiers, matrix1_, cylinderColor, 0.015f, originMatrixProxy, gizmo_axis_radius);

		if (matrix2_)
			pointGizmo2_ = new VectorGizmo(disablerModifiers, matrix2_, cylinderColor, 0.015f, originMatrixProxy, gizmo_axis_radius);
	}
	else
	{
		pointGizmo1_ = new PositionGizmo(disablerModifiers, matrix1_, originMatrixProxy, gizmo_axis_radius, isPlanar);
	
		if (matrix2_)
			pointGizmo2_ = new PositionGizmo(disablerModifiers, matrix2_, originMatrixProxy, gizmo_axis_radius, isPlanar);
	}
	radiusGizmo_ = new RadiusGizmo(radius, matrix1_, "radius", cylinderColor, radiusGizmoRadius, MODIFIER_ALT, 1.75f, false, &radiusGizmoMatrix_, NULL, RadiusGizmo::SHOW_SPHERE_NEVER);
}

CylinderGizmo::~CylinderGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(pointGizmo1_);

	if (pointGizmo2_)
		GizmoManager::instance().removeGizmo(pointGizmo2_);
	
	GizmoManager::instance().removeGizmo(radiusGizmo_);
}

void CylinderGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	BW_GUARD;

	originMatrixProxy_ = matrix;
	radiusGizmo_->setVisualOffsetMatrixProxy(originMatrixProxy_);
	pointGizmo1_->setVisualOffsetMatrixProxy(originMatrixProxy_);
	if (pointGizmo2_)
		pointGizmo2_->setVisualOffsetMatrixProxy(originMatrixProxy_);
}

Matrix CylinderGizmo::objectTransform() const
{
	BW_GUARD;

	if (originMatrixProxy_)
		return originMatrixProxy_->get();
	static Matrix ret( Matrix::identity );	
	return ret;
}

bool CylinderGizmo::draw( Moo::DrawContext& drawContext,  bool force )
{
	BW_GUARD;

	bool pointDrawn = pointGizmo1_->draw( drawContext, force );
	
	if (pointGizmo2_)
		pointGizmo2_->draw( drawContext, force );

	bool radiusDrawn = radiusGizmo_->draw( drawContext, force );

	if (!(pointDrawn || radiusDrawn))
		return false;

	// draw the line
	Matrix m;

	matrix1_->getMatrix(m);
	Vector3 start = m.applyToOrigin();
	Vector3 end = Vector3::zero();

	// if no second point, draw a vertical cylinder
	if (pointGizmo2_)
	{
		matrix2_->getMatrix(m);
		end = m.applyToOrigin();
	}
	else
	{
		end = start;
		end.y = 10.f;
	}

	if (originMatrixProxy_)
	{
		Matrix offsetMatrix;
		originMatrixProxy_->getMatrix(offsetMatrix);
		Vector3 offset = offsetMatrix.applyToOrigin();
		start += offset;
		end += offset;
	}

	// set the radius disk to be in the plane described at the starting point
	if (start != end)
	{
		Vector3 planeNormal = end - start;
		planeNormal.normalise();

		Vector3 referenceVector = Vector3(planeNormal.y, -planeNormal.x, planeNormal.z);
		if (planeNormal.z == 1.f)
			referenceVector = Vector3(1.f, 0.f, 0.f);

		radiusGizmoMatrix_.row(3, Vector4(start, 1.f) );
		radiusGizmoMatrix_.row(2, Vector4(planeNormal, 0.f) );
		radiusGizmoMatrix_.row(1, Vector4(referenceVector.crossProduct(radiusGizmoMatrix_[2]), 0.f) );
		Vector4 row1 = radiusGizmoMatrix_.row(1); row1.normalise();
		radiusGizmoMatrix_.row(1, row1 );
		radiusGizmoMatrix_.row(0, Vector4(radiusGizmoMatrix_[1].crossProduct(radiusGizmoMatrix_[2]), 0.f) );
		Vector4 row0 = radiusGizmoMatrix_.row(0); row0.normalise();
		radiusGizmoMatrix_.row(0, row0 );

	}


	if (!pointGizmo2_)
		start.y = -10.f;

	Geometrics::instance().wireCylinder(start, end, radius_->get(), cylinderColor_);

	return true;
}

bool CylinderGizmo::intersects( const Vector3& origin,
							const Vector3& direction, float& t, bool force )
{
	BW_GUARD;

	bool result = pointGizmo1_->intersects( origin, direction, t, force );

	if (pointGizmo2_)
		result |= pointGizmo2_->intersects( origin, direction, t, force );
	
	return result || radiusGizmo_->intersects( origin, direction, t, force );
}

void CylinderGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	pointGizmo1_->click(origin, direction);

	if (pointGizmo2_)
		pointGizmo2_->click(origin, direction);
	
	radiusGizmo_->click(origin, direction);
}

void CylinderGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	pointGizmo1_->rollOver(origin, direction);

	if (pointGizmo2_)
		pointGizmo2_->rollOver(origin, direction);
	
	radiusGizmo_->rollOver(origin, direction);
}


///////////////////////////////////////////////////////////////////////////////


SphereGizmo::SphereGizmo( MatrixProxyPtr matrix1, 
							FloatProxyPtr radius1, FloatProxyPtr radius2, 
							Moo::Colour color,
							bool drawVectors, MatrixProxyPtr originMatrixProxy,
							int disablerModifiers )
	: matrix1_(matrix1)
	, radius1_(radius1)
	, radius2_(radius2)
	, radiusGizmo1_(NULL)
	, radiusGizmo2_(NULL)
	, color_(color)
	, radiusGizmoMatrix_(Matrix::identity)
	, originMatrixProxy_(originMatrixProxy)
	, pointGizmo1_(NULL)
{
	BW_GUARD;

	if (drawVectors)
	{
		pointGizmo1_ = new VectorGizmo(disablerModifiers, matrix1_, color_, 0.015f, originMatrixProxy, gizmo_axis_radius);
	}
	else
	{
		pointGizmo1_ = new PositionGizmo(disablerModifiers, matrix1_, originMatrixProxy, gizmo_axis_radius);
	}

	radiusGizmo1_ = new RadiusGizmo( radius1_, matrix1_, "min radius", 
									Moo::Colour(0.f, 1.f, 0.f, 1.f), 2.0f,
									MODIFIER_ALT, 1.75, false, NULL, originMatrixProxy, 
									RadiusGizmo::SHOW_SPHERE_ALWAYS);

	if (radius2_)
		radiusGizmo2_ = new RadiusGizmo( radius2_, matrix1_, "max radius", 
									Moo::Colour(1.f, 0.f, 0.f, 1.f), 4.0f,
									MODIFIER_ALT, 1.75f, false, NULL, originMatrixProxy,
									RadiusGizmo::SHOW_SPHERE_ALWAYS);
}

SphereGizmo::~SphereGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(pointGizmo1_);
	GizmoManager::instance().removeGizmo(radiusGizmo1_);

	if (radiusGizmo2_)
		GizmoManager::instance().removeGizmo(radiusGizmo2_);
}

void SphereGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	BW_GUARD;

	originMatrixProxy_ = matrix;
	pointGizmo1_->setVisualOffsetMatrixProxy(originMatrixProxy_);
	radiusGizmo1_->setVisualOffsetMatrixProxy(originMatrixProxy_);
	if (radiusGizmo2_)
		radiusGizmo2_->setVisualOffsetMatrixProxy(originMatrixProxy_);
}

Matrix SphereGizmo::objectTransform() const
{
	BW_GUARD;

	if (originMatrixProxy_)
		return originMatrixProxy_->get();
	static Matrix ret( Matrix::identity );	
	return ret;
}

void SphereGizmo::drawZBufferedStuff( bool force )
{
	BW_GUARD;

	pointGizmo1_->drawZBufferedStuff( force );
	radiusGizmo1_->drawZBufferedStuff( force );

	if (radiusGizmo2_)
		radiusGizmo2_->drawZBufferedStuff( force );
}


bool SphereGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	bool pointDrawn = pointGizmo1_->draw( drawContext, force );
	bool radiusDrawn = radiusGizmo1_->draw( drawContext, force );

	if (radiusGizmo2_)
		radiusGizmo2_->draw( drawContext, force );

	if (!(pointDrawn || radiusDrawn))
		return false;

	return true;
}

bool SphereGizmo::intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force )
{
	BW_GUARD;

	bool result = pointGizmo1_->intersects( origin, direction, t, force );

	if (radiusGizmo2_)
		result |= radiusGizmo2_->intersects( origin, direction, t, force );
	
	return result || radiusGizmo1_->intersects( origin, direction, t, force );
}

void SphereGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	pointGizmo1_->click(origin, direction);
	radiusGizmo1_->click(origin, direction);

	if (radiusGizmo2_)
		radiusGizmo2_->click(origin, direction);
}

void SphereGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	pointGizmo1_->rollOver(origin, direction);
	radiusGizmo1_->rollOver(origin, direction);

	if (radiusGizmo2_)
		radiusGizmo2_->rollOver(origin, direction);
}
BW_END_NAMESPACE

