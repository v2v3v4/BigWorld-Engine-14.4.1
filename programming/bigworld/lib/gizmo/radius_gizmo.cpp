#include "pch.hpp"
#include "math/colour.hpp"
#include "radius_gizmo.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "cstdmf/debug.hpp"
#include "dynamic_float_device.hpp"
#include "tool.hpp"
#include "tool_manager.hpp"
#include "general_properties.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/geometrics.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/visual_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Gizmo", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_radiusGizmoVisual( "editor/radiusGizmo" );
static AutoConfigString s_gizmoVisualSmall( "editor/radiusGizmoSmall" );

extern Moo::Colour g_unlit;
extern bool g_showHitRegion;

class RadiusShapePart : public ShapePart
{
public:
	RadiusShapePart( const Moo::Colour& col, int axis ) 
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


// -----------------------------------------------------------------------------
// Section: RadiusGizmo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
RadiusGizmo::RadiusGizmo(
		FloatProxyPtr pFloat,
		MatrixProxyPtr pCenter,
		const BW::string& name,
		uint32 colour,
		float editableRadius,
		int enablerModifier,
		float adjFactor,
		bool drawZBuffered,
		const Matrix * discMatrix,
		MatrixProxyPtr visualOffsetMatrix,
		ShowSphere showSphere,
		LabelFormatter<float>* formatter ) :
	active_( false ),
	currentPart_( NULL ),
	pFloat_( pFloat ),
	pCenter_( pCenter ),
	name_( name ),
	gizmoColour_( colour ),
	inited_( false ),
	drawMesh_( NULL ),
	lightColour_( 0, 0, 0, 0 ),
	editableRadius_( editableRadius ),
	drawZBuffered_( drawZBuffered ),
	enablerModifier_( enablerModifier ),
	discMatrix_( discMatrix ),
	visualOffsetMatrix_( visualOffsetMatrix ),
	showSphere_( showSphere ),
	formatter_( formatter ),
	adjFactor_(adjFactor)
{
}


void RadiusGizmo::init()
{
	BW_GUARD;

	if (!inited_)
	{
		if (editableRadius_ > 4.f)
		{
			if ( !s_radiusGizmoVisual.value().empty() )
			{
				drawMesh_ = Moo::VisualManager::instance()->get( s_radiusGizmoVisual );			
			}
		}	
		else
		{
			if ( !s_gizmoVisualSmall.value().empty() )
			{
				drawMesh_ = Moo::VisualManager::instance()->get( s_gizmoVisualSmall );			
			}
		}

		selectionMesh_.transform( Matrix::identity );
		selectionMesh_.addDisc( Vector3( 0, 0, 0 ),
			editableRadius_, editableRadius_ + 0.5f, gizmoColour_,
			new RadiusShapePart( Moo::Colour( 1, 0, 0, 0 ), 0 ) );
		inited_ = true;
	}
}


/**
 *	Destructor.
 */
RadiusGizmo::~RadiusGizmo()
{
}

void RadiusGizmo::drawRadius( bool force )
{
	BW_GUARD;

	if 
	( 
		showSphere_ == SHOW_SPHERE_NEVER 
		|| 
		(
			showSphere_ == SHOW_SPHERE_MODIFIER 
			&&
			!force 
			&&
			enablerModifier_ != ALWAYS_ENABLED
			&& 
			((InputDevices::modifiers() & enablerModifier_) == 0 ) 
		)
	)
	{
		return;
	}
	Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
	Geometrics::instance().wireSphere( objectTransform().applyToOrigin(), pFloat_->get(), gizmoColour_ );
	Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
}

void RadiusGizmo::drawZBufferedStuff( bool force )
{
	BW_GUARD;

	if (drawZBuffered_)
	{
		drawRadius( force );
	}
}

bool RadiusGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	active_ = false;
	if (!drawZBuffered_)
	{
		drawRadius( force );
	}

	if 
	( 
		!force 
		&& 
		enablerModifier_ != ALWAYS_ENABLED
		&&
		(InputDevices::modifiers() & enablerModifier_) == 0 
	)
	{
		return false;
	}
	active_ = true;

	init();

	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	pDev->SetTransform( D3DTS_VIEW, &rc.view() );
	pDev->SetTransform( D3DTS_PROJECTION, &rc.projection() );
	Moo::rc().setPixelShader(NULL);

	if (drawMesh_)
	{
		// set a temporary sunlight to filter the axes colour.
		Moo::SunLight oldSun = rc.effectVisualContext().sunLight();
		Moo::SunLight gizmoSun = oldSun;
		// this is camera facing, so force light to come from camera.
		// this is because we don't have any alpha effects that aren't lit.
		gizmoSun.m_dir = -Moo::rc().invView()[2];
		gizmoSun.m_ambient = lightColour_;
		gizmoSun.m_color = lightColour_;
		rc.effectVisualContext().sunLight(gizmoSun);

		rc.push();
		float extraScale = editableRadius_ / 8.f;	//mesh made for an 8 radius gizmo
		Matrix gt( gizmoTransform() );
		Matrix sc;
		sc.setScale( extraScale, extraScale, extraScale );
		gt.preMultiply(sc);
		rc.world( gt );
		drawMesh_->draw( drawContext );
		rc.pop();

		rc.effectVisualContext().sunLight(oldSun);
	}

	if (!drawMesh_ || g_showHitRegion)
	{
		rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
		Moo::Material::setVertexColour();
		rc.setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		//rc.setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		rc.setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		//rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		//rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
		rc.setRenderState( D3DRS_LIGHTING, FALSE );
		rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
		rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		uint32 tfactor = lightColour_;
		rc.setRenderState( D3DRS_TEXTUREFACTOR, tfactor );
		
		typedef Moo::VertexXYZND VertexType;
		int vertexSize = sizeof(VertexType);

		Moo::rc().setPixelShader(NULL);
		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( VertexType::fvf() );

		Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
		Moo::IndicesReference ind = dib.lock( (UINT)selectionMesh_.indices().size() );
		if (ind.size())
		{
			ind.fill( &selectionMesh_.indices().front(), (uint32)selectionMesh_.indices().size() );

			dib.unlock();
			uint32 ibLockIndex = dib.lockIndex();

			Moo::DynamicVertexBufferBase2 & dvb = 
				Moo::DynamicVertexBufferBase2::instance( vertexSize );

			void * verts = dvb.lock( 
				static_cast<uint32>(selectionMesh_.verts().size()) );

			if (verts)
			{
				memcpy( verts, &selectionMesh_.verts().front(), 
					vertexSize * selectionMesh_.verts().size() );

				dvb.unlock();
				uint32 vbLockIndex = dvb.lockIndex();
				dvb.set();
				
				dib.indexBuffer().set();
			
				//draw editable part
				{
						Matrix gizmo = gizmoTransform();
						pDev->SetTransform( D3DTS_WORLD, &gizmo );

					rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, vbLockIndex, 0, (UINT)selectionMesh_.verts().size(),
						ibLockIndex, (UINT)selectionMesh_.indices().size() / 3 );
				}
			}
		}	
	}

	return true;
}

bool RadiusGizmo::intersects( const Vector3& origin, const Vector3& direction,
														float& t, bool force )
{
	BW_GUARD;

	if ( !active_ )
	{
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

	currentPart_ = (RadiusShapePart*)selectionMesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );

	t /= l;

	return currentPart_ != NULL;
}

void RadiusGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	if (visualOffsetMatrix_)	// it is not using the properties to move
		pCenter_->recordState();

	// handle the click
	if (currentPart_ != NULL) 
	{
		ToolPtr radiusTool( new Tool(
			new AdaptivePlaneToolLocator( this->objectTransform().applyToOrigin() ),
			ToolViewPtr( new FloatVisualiser( pCenter_, pFloat_, gizmoColour_, name_, *formatter_ ), true ),
			ToolFunctorPtr( new DynamicFloatDevice( pCenter_, pFloat_, adjFactor_ ), true )
			), true );
		this->pushTool( radiusTool );
	}
}

void RadiusGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	// roll it over.
	if (currentPart_ != NULL)
	{
		lightColour_ = Moo::Colour( 1,1,1,1 );
	}
	else
	{
		lightColour_ = g_unlit;
	}
}

Matrix RadiusGizmo::objectTransform() const
{
	BW_GUARD;

	Matrix m;
	pCenter_->getMatrix( m );

	if (visualOffsetMatrix_)
	{
		// move the model
		Matrix mat;
		visualOffsetMatrix_->getMatrix(mat);
		m.postTranslateBy(mat.applyToOrigin());
	}

	return m;
}


//Overridden from Gizmo.  Spins to face the camera.
Matrix RadiusGizmo::gizmoTransform() const
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
	if (discMatrix_)
	{
		m = *discMatrix_;
	}
	else
	{
        m.lookAt( pos, forward, up );
		m.invert();
	}
	m.preMultiply( s );

	return m;
}

void RadiusGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	visualOffsetMatrix_ = matrix;
}

BW_END_NAMESPACE
// radius_gizmo.cpp
