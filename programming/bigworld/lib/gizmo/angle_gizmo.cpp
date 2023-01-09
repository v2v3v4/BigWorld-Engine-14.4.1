#include "pch.hpp"

#include "Angle_gizmo.hpp"
#include "tool.hpp"
#include "dynamic_float_device.hpp"
#include "general_properties.hpp"
#include "formatter.hpp"
#include "tool_view.hpp"
#include "tool_manager.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "input/input.hpp"

BW_BEGIN_NAMESPACE

static Moo::Colour s_unlit( 0.25, 0.25, 0.25, 0.25 );

class AngleShapePart : public ShapePart
{
public:
	AngleShapePart( const Moo::Colour& col ) 
	:	colour_( col )
	{
	}
	
	const Moo::Colour& colour() const { return colour_; }
private:
	Moo::Colour	colour_;
};

// -----------------------------------------------------------------------------
// Section: AngleGizmo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
AngleGizmo::AngleGizmo( MatrixProxyPtr pMatrix,
					   FloatProxyPtr pAngle,
					   int enablerModifier ) 
:	active_( false ),
	inited_( false ),
	currentPart_( NULL ),
	pMatrix_( pMatrix ),
	pAngle_( pAngle ),
	lightColour_( 0, 0, 0, 0 ),
	enablerModifier_( enablerModifier )
{
}


void AngleGizmo::init()
{
	BW_GUARD;

	if (!inited_)
	{
		float radius = 2.f;
		float length = 2.f;

		mesh_.addCone( Vector3(0,0,0), radius, length, true, 0xffffff00, new AngleShapePart( Moo::Colour( 1, 1, 1, 0 ) ) );
		inited_ = true;
	}
}


/**
 *	Destructor.
 */
AngleGizmo::~AngleGizmo()
{

}

void AngleGizmo::drawZBufferedStuff( bool force )
{
	BW_GUARD;

	active_ = false;
	if 
	( 
		!force 
		&& 
		enablerModifier_ != ALWAYS_ENABLED
		&&
		(InputDevices::modifiers() & enablerModifier_) == 0 
	)
	{
		return;
	}
	active_ = true;

	// Draw the cone
	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	SolidShapeMesh coneMesh;

	coneMesh.transform( objectTransform() );

	float coneLength = 75.f;
	float coneRadius = coneLength * tanf( DEG_TO_RAD( pAngle_->get() ) / 2.f );

	coneMesh.addCone( Vector3( 0.f, 0.f, coneLength ), coneRadius, -coneLength, false, 0xffffff00, 0 );

	rc.setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	rc.setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
	Moo::Material::setVertexColour();
	rc.setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	rc.setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
	rc.setRenderState( D3DRS_LIGHTING, FALSE );
	rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
	rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	uint32 tfactor = lightColour_;
	rc.setRenderState( D3DRS_TEXTUREFACTOR, tfactor );


	//pDev->SetTransform( D3DTS_WORLD, &gizmoTransform() );
	pDev->SetTransform( D3DTS_WORLD, &Matrix::identity );
	pDev->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	pDev->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZND::fvf() );

	rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
	rc.setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
	Moo::IndicesReference ind = dib.lock( (uint32)coneMesh.indices().size() );
	if (ind.size())
	{
		ind.fill( &coneMesh.indices().front(), (uint32)coneMesh.indices().size() );
		dib.unlock();

		uint32 ibLockIndex = dib.lockIndex();

		typedef Moo::VertexXYZND VertexType;
		int vertexSize = sizeof(VertexType);
		
		Moo::DynamicVertexBufferBase2 & dvb = 
			Moo::DynamicVertexBufferBase2::instance( vertexSize );
		uint32 vbLockIndex = 0;
		if ( dvb.lockAndLoad( &coneMesh.verts().front(),
			static_cast<uint32>(coneMesh.verts().size()), vbLockIndex ) &&
			 SUCCEEDED(dvb.set()) &&
			 SUCCEEDED(dib.indexBuffer().set()) )
		{
			rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, vbLockIndex, 0, (uint32)(coneMesh.verts().size()),
				ibLockIndex, (uint32)(coneMesh.indices().size()) / 3 );
		}
	}

	rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	rc.setRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	rc.setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}

bool AngleGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	active_ = false;
	if ( !force && (InputDevices::modifiers() & enablerModifier_) == 0 )
		return false;
	active_ = true;

	init();

	// Draw the gizmo
	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
	Moo::Material::setVertexColour();
	rc.setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	rc.setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
	rc.setRenderState( D3DRS_LIGHTING, FALSE );
	rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
	rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	uint32 tfactor = lightColour_;
	rc.setRenderState( D3DRS_TEXTUREFACTOR, tfactor );

	Matrix gizmo = gizmoTransform();
	pDev->SetTransform( D3DTS_WORLD, &gizmo );
	pDev->SetTransform( D3DTS_VIEW, &rc.view() );
	pDev->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZND::fvf() );

	Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
	Moo::IndicesReference ind = dib.lock( (uint32)mesh_.indices().size() );
	if (ind.size())
	{
		ind.fill( &mesh_.indices().front(), (uint32)mesh_.indices().size() );
		dib.unlock();

		typedef Moo::VertexXYZND VertexType;
		int vertexSize = sizeof(VertexType);

		Moo::DynamicVertexBuffer & dvb = 
			Moo::DynamicVertexBuffer::instance( vertexSize );
		void * verts = dvb.lock( static_cast<uint32>(mesh_.verts().size()) );	
		if (verts)
		{
			memcpy( verts, &mesh_.verts().front(), 
				vertexSize * mesh_.verts().size() );
			dvb.unlock();

			dvb.set();
			dib.indexBuffer().set();

			rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, dvb.lockIndex(), 0, (UINT)mesh_.verts().size(),
				dib.lockIndex(), (UINT)mesh_.indices().size() / 3 );
		}
	}

	return true;
}

bool AngleGizmo::intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force )
{
	BW_GUARD;

	if ( !active_ )
	{
		currentPart_ = NULL;
		return false;
	}

	init();

	lightColour_ = s_unlit;
	Matrix m = gizmoTransform();
	m.invert();
	
	Vector3 lo = m.applyPoint( origin );
	Vector3 ld = m.applyVector( direction );
	float l = ld.length();
	t *= l;
	ld /= l;;

	currentPart_ = (AngleShapePart*)mesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );

	t /= l;

	return currentPart_ != NULL;
}

void AngleGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	// handle the click


	if (currentPart_ != NULL) 
	{
		ToolPtr angleTool( new Tool(
			ToolLocatorPtr( new AdaptivePlaneToolLocator( this->objectTransform().applyToOrigin() ), true ),
			ToolViewPtr( new FloatVisualiser( pMatrix_, pAngle_, 0xffffff00, "angle", AngleFormatter::s_def ), true ),
			ToolFunctorPtr( new DynamicFloatDevice( pMatrix_, pAngle_, 0.4f ), true )
			), true );
		this->pushTool( angleTool );
	}
}

void AngleGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	// roll it over.
	if (currentPart_ != NULL)
	{
		lightColour_ = Moo::Colour( 1,1,1,1 );
	}
	else
	{
		lightColour_ = s_unlit;
	}
}

Matrix AngleGizmo::objectTransform() const
{
	BW_GUARD;

	Matrix m;
	pMatrix_->getMatrix( m );
	return m;
}

Matrix AngleGizmo::gizmoTransform() const
{
	BW_GUARD;

	Vector3 pos = objectTransform()[3];

	Matrix s;

	float scale = ( Moo::rc().invView()[2].dotProduct( pos ) -
		Moo::rc().invView()[2].dotProduct( Moo::rc().invView()[3] ) );
	if (scale > 0.05)
		scale /= 25.f;
	else
		scale = 0.05f / 25.f;
	s.setScale( scale, scale, scale );

	//now, spin to face the camera
	Vector3 forward = Moo::rc().invView()[2];
	Vector3 up = Moo::rc().invView()[1];
	Matrix m;
	m.lookAt( pos, forward, up );
	m.invert();
	m.preMultiply( s );

	return m;
}

BW_END_NAMESPACE
// angle_gizmo.cpp
