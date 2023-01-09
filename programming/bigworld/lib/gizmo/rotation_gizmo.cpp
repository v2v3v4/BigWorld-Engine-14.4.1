#include "pch.hpp"
#include "rotation_gizmo.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk.hpp"
#include "tool.hpp"
#include "tool_manager.hpp"
#include "general_properties.hpp"
#include "current_general_properties.hpp"
#include "matrix_rotator.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/effect_visual_context.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "moo/visual_manager.hpp"

BW_BEGIN_NAMESPACE

static AutoConfigString s_rotationGizmoVisual( "editor/rotationGizmo" );

extern Moo::Colour g_unlit;
extern bool g_showHitRegion;

class RotationShapePart : public ShapePart
{
public:
	RotationShapePart( const Moo::Colour& col, int axis ) 
	:	colour_( col )
	{
		BW_GUARD;

		Vector3 normal( 0, 0, 0 );
		normal[axis] = 1.f;
		planeEq_ = PlaneEq( normal, 0 );
	}

	const Moo::Colour& colour() const { return colour_; }
	const PlaneEq & plane() const{ return planeEq_; }

private:
	Moo::Colour	colour_;
	PlaneEq		planeEq_;
};

Matrix normaliseMatrix( const Matrix& m )
{
	Matrix m2 = m;
	m2[0].normalise();
	m2[1].normalise();
	m2[2].normalise();
	return m2;
}


// -----------------------------------------------------------------------------
// Section: RotationGizmo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
RotationGizmo::RotationGizmo( MatrixProxyPtr pMatrix, int enablerModifier, int disablerModifier ) :
	active_( false ),
	inited_( false ),
	currentPart_( NULL ),
	pMatrix_( pMatrix ),
	drawMesh_( NULL ),
	lightColour_( 0, 0, 0, 0 ),
	enablerModifier_( enablerModifier ),
	disablerModifier_( disablerModifier )
{
}

void RotationGizmo::init()
{
	BW_GUARD;

	if (!inited_)
	{
		if ( !s_rotationGizmoVisual.value().empty() )
		{
			drawMesh_ = Moo::VisualManager::instance()->get( s_rotationGizmoVisual );
			ResourceCache::instance().addResource( drawMesh_ );
		}	

		float inner = 6.f;
		float outer = 6.5f;
		Matrix m;
		m.setIdentity();
		selectionMesh_.transform( m );
		selectionMesh_.addDisc( Vector3( 0, 0, 0 ), inner, outer, 0xff0000, new RotationShapePart( Moo::Colour( 1, 0, 0, 0 ), 2 ) );

		m.setRotateY( DEG_TO_RAD( 90 ) );
		selectionMesh_.transform( m );
		selectionMesh_.addDisc( Vector3( 0, 0, 0 ), inner, outer, 0x00ff00, new RotationShapePart( Moo::Colour( 0, 1, 0, 0 ), 0 ) );

		m.setRotateX( DEG_TO_RAD( 90 ) );
		selectionMesh_.transform( m );
		selectionMesh_.addDisc( Vector3( 0, 0, 0 ), inner, outer, 0x0000ff, new RotationShapePart( Moo::Colour( 0, 0, 1, 0 ), 1 ) );

		inited_ = true;
	}
}


/**
 *	Destructor.
 */
RotationGizmo::~RotationGizmo()
{

}

bool RotationGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	active_ = false;
	if 
	( 
		!force 
		&& 
		enablerModifier_ != ALWAYS_ENABLED
		&&
		( 
			(InputDevices::modifiers() & enablerModifier_) == 0 
			||
			(InputDevices::modifiers() & disablerModifier_) != 0 
		) 
	)
	{
		return false;
	}
	active_ = true;

	init();

	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	rc.setPixelShader(NULL);

	if (drawMesh_)
	{
		Moo::SunLight oldSun = rc.effectVisualContext().sunLight();
		Moo::SunLight gizmoSun = oldSun;
		gizmoSun.m_ambient = lightColour_;
		gizmoSun.m_color = lightColour_;
		rc.effectVisualContext().sunLight( gizmoSun );
		rc.effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_FRAME );
		rc.setPixelShader( NULL );		

		rc.push();
		rc.world( gizmoTransform() );
		drawMesh_->draw( drawContext );
		rc.pop();

		const Moo::DrawContext::ChannelMask gizmoChannelMask = Moo::DrawContext::TRANSPARENT_CHANNEL_MASK;
		drawContext.end( gizmoChannelMask);
		drawContext.flush( gizmoChannelMask );
		drawContext.begin( gizmoChannelMask );

		rc.effectVisualContext().sunLight( oldSun );
		rc.effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_FRAME );
	}
	
	if (!drawMesh_ || g_showHitRegion)
	{
		rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
		Moo::Material::setVertexColour();
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
		pDev->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
		pDev->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );		
		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( Moo::VertexXYZND::fvf() );

		Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
		Moo::IndicesReference ind = dib.lock( 
			static_cast<uint32>(selectionMesh_.indices().size()) );
		if (ind.size())
		{
			ind.fill( &selectionMesh_.indices().front(), 
				static_cast<uint32>(selectionMesh_.indices().size()) );
			dib.unlock();

			typedef Moo::VertexXYZND VertexType;
			int vertexSize = sizeof(VertexType);

			Moo::DynamicVertexBuffer & dvb = 
				Moo::DynamicVertexBuffer::instance( vertexSize );
			void * verts = dvb.lock( 
				static_cast<uint32>(selectionMesh_.verts().size()) );	
			if (verts)
			{
				memcpy( verts, &selectionMesh_.verts().front(), 
					vertexSize * selectionMesh_.verts().size() );
				dvb.unlock();

				dvb.set();
				dib.indexBuffer().set();

				rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, dvb.lockIndex(), 0, 
					static_cast<UINT>(selectionMesh_.verts().size()),
					dib.lockIndex(), 
					static_cast<UINT>(selectionMesh_.indices().size() / 3) );
			}
		}
	}

	return true;
}

bool RotationGizmo::intersects( const Vector3& origin,
							const Vector3& direction, float& t, bool force )
{
	BW_GUARD;

	if ( !active_ )
	{
		currentPart_ = NULL;
		return false;
	}

	init();

	lightColour_ = g_unlit;

	Matrix m = gizmoTransform();
	m.invert();

	Vector3 lo = m.applyPoint( origin );
	Vector3 ld = m.applyVector( direction );
	float l = ld.length();
	t *= l;
	ld /= l;;

	currentPart_ = (RotationShapePart*)selectionMesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );

	t /= l;

/*	currentPart_ = (RotationShapePart*)selectionMesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );*/

	return currentPart_ != NULL;
}

void RotationGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	// handle the click
	if (currentPart_ != NULL) 
	{
		PlaneEq	peq( Vector3( 0.f, 0.f, 0.f ), 0.f );
		peq = currentPart_->plane();
		peq = PlaneEq( this->objectTransform().applyToOrigin(), getCoordModifier().applyVector( peq.normal() ) );
		ToolPtr moveTool( new Tool(
			ToolLocatorPtr( new PlaneToolLocator( &peq ), true ),
			/*pView*/NULL,
			ToolFunctorPtr( new MatrixRotator( pMatrix_ ), true )
			), true );
		this->pushTool( moveTool );
	}
}

void RotationGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	// roll it over.
	if (currentPart_ != NULL)
	{
		lightColour_ = currentPart_->colour();
	}
	else
	{
		lightColour_ = g_unlit;
	}
}

Matrix RotationGizmo::objectTransform() const
{
	BW_GUARD;

	Matrix m;
	pMatrix_->getMatrix( m );
	m.translation(CurrentRotationProperties::averageOrigin());
	return m;
}


Matrix RotationGizmo::objectCoord() const
{
	BW_GUARD;

	Matrix coord;
	if (CurrentRotationProperties::properties().size() == 1)
	{
		CurrentRotationProperties::properties()[0]->pMatrix()->getMatrix(coord);
	}
	else
	{
		coord.setIdentity();
	}
	return coord;
}

Matrix RotationGizmo::getCoordModifier() const
{
	BW_GUARD;

	Matrix coord;
	if( CoordModeProvider::ins()->getCoordMode() == CoordModeProvider::COORDMODE_OBJECT )
	{
		return objectCoord();
	}
	else if( CoordModeProvider::ins()->getCoordMode() == CoordModeProvider::COORDMODE_VIEW )
	{
		coord = Moo::rc().invView();
	}
	else
	{
		coord.setIdentity();
	}
	return coord;
}


BW_END_NAMESPACE
// rotation_gizmo.cpp
