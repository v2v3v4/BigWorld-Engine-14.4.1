#include "pch.hpp"
#include "position_gizmo.hpp"
#include "tool.hpp"
#include "general_properties.hpp"
#include "current_general_properties.hpp"
#include "appmgr/options.hpp"
#include "gizmo/matrix_mover.hpp"
#include "gizmo/matrix_positioner.hpp"
#include "input/input.hpp"
#include "moo/visual_manager.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/effect_visual_context.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"

BW_BEGIN_NAMESPACE

extern Moo::Colour g_unlit;
static AutoConfigString s_positionGizmoVisual( "editor/positionGizmo" );
static AutoConfigString s_gizmoTrLockVisual( "editor/positionGizmoTrLock" );
static AutoConfigString s_gizmoObLockVisual( "editor/positionGizmoObLock" );
static AutoConfigString s_pointerModel( "editor/pointerModel" );

extern bool g_showHitRegion;


// -----------------------------------------------------------------------------
// Section: PositionGizmo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PositionGizmo::PositionGizmo( int disablerModifiers, MatrixProxyPtr matrixProxy, MatrixProxyPtr visualOffsetMatrix, float radius, bool isPlanar ) 
:	active_( false ),
	inited_( false ),
	matrixProxy_(matrixProxy),
	visualOffsetMatrix_(visualOffsetMatrix),
	currentPart_( NULL ),	
	lightColour_( 0, 0, 0, 0 ),
	disablerModifiers_( disablerModifiers ),
	radius_( radius ),
	isPlanar_( isPlanar ),
	pModel_( NULL )
{
	BW_GUARD;

	pDrawMesh_ = drawMesh_[0] = drawMesh_[1] = drawMesh_[2] = NULL;
}


void PositionGizmo::init()
{
	BW_GUARD;

	if (!inited_)
	{
		if ( !s_positionGizmoVisual.value().empty() )
		{
			drawMesh_[0] = Moo::VisualManager::instance()->get( s_positionGizmoVisual );
			ResourceCache::instance().addResource( drawMesh_[0] );
		}
		if ( !s_gizmoObLockVisual.value().empty() )
		{
			drawMesh_[1] = Moo::VisualManager::instance()->get( s_gizmoObLockVisual );
			ResourceCache::instance().addResource( drawMesh_[1] );
		}
		if ( !s_gizmoTrLockVisual.value().empty() )
		{
			drawMesh_[2] = Moo::VisualManager::instance()->get( s_gizmoTrLockVisual );
			ResourceCache::instance().addResource( drawMesh_[2] );
		}

		rebuildMesh( true );
		inited_ = true;
	}
}


void PositionGizmo::rebuildMesh( bool force )
{
	BW_GUARD;

	if( !force && snapMode_ == SnapProvider::instance()->snapMode() &&
		objectNum_ == CurrentPositionProperties::properties().size() )
		return;
	if( lastTool_ && lastTool_ == ToolManager::instance().tool() )
		return;

	snapMode_ = SnapProvider::instance()->snapMode();
	objectNum_ = (uint)CurrentPositionProperties::properties().size();
	lastTool_ = NULL;

	if( snapToTerrainEnabled() || isPlanar_ )
		pDrawMesh_ = drawMesh_[2];
	else if( !snapToObstableEnabled() )
		pDrawMesh_ = drawMesh_[0];
	else
		pDrawMesh_ = drawMesh_[1];

	selectionMesh_.clear();

	Matrix m;
	m.setIdentity();
	selectionMesh_.transform( m );

	float length = 3.f;
	float sphereRadius = length / 3.f;

	if (pDrawMesh_ != NULL)
	{
		static float s_sphereRadius = 0.45f;
		/*static bool s_wAdded = false;
		if (!s_wAdded)
		{
			s_wAdded = true;
			MF_WATCH( "Sphere Radius", s_sphereRadius );
		}*/
		sphereRadius = s_sphereRadius;
	}

	if( snapToObstableEnabled() )
		selectionMesh_.addSphere( Vector3( 0, 0, 0 ), sphereRadius, 0x7fffffff, new PositionShapePart( Moo::Colour( 1, 1, 1, 1 ) )  );
	else
		sphereRadius = 0.0;

	selectionMesh_.addCylinder( Vector3( 0, 0, length ), radius_, -length + sphereRadius, false, true, 0xffff0000, new PositionShapePart( Moo::Colour( 1, 0, 0, 0 ), Vector3( 0, 0, 1 ) ) );
	selectionMesh_.addCone( Vector3( 0, 0, length ), radius_*2.f, 1, true, 0xffff0000, new PositionShapePart( Moo::Colour( 1, 0, 0, 0 ), Vector3( 0, 0, 1 ) ) );
	m.setRotateY( DEG_TO_RAD( 90 ) );
	selectionMesh_.transform( m );
	selectionMesh_.addCylinder( Vector3( 0, 0, length ), radius_, -length + sphereRadius, false, true, 0xff00ff00, new PositionShapePart( Moo::Colour( 0, 1, 0, 0 ), Vector3( 1, 0, 0 ) ) );
	selectionMesh_.addCone( Vector3( 0, 0, length ), radius_*2.f, 1, true, 0xff00ff00, new PositionShapePart( Moo::Colour( 0, 1, 0, 0 ), Vector3( 1, 0, 0 ) ) );
	m.setRotateX( DEG_TO_RAD( -90 ) );
	selectionMesh_.transform( m );
	if ( !snapToTerrainEnabled() && !isPlanar_)
	{
		selectionMesh_.addCylinder( Vector3( 0, 0, length ), radius_, -length + sphereRadius, false, true, 0xff0000ff, new PositionShapePart( Moo::Colour( 0, 0, 1, 0 ), Vector3( 0, 1, 0 ) ) );
		selectionMesh_.addCone( Vector3( 0, 0, length ), radius_*2.f, 1, true, 0xff0000ff, new PositionShapePart( Moo::Colour( 0, 0, 1, 0 ), Vector3( 0, 1, 0 ) ) );
	}

	bool s_watcherAdded = false;	

	float offset = length / 6.f;
	float boxSize = length / 12.f;
	float boxHeight = length / 12.f;
	if (pDrawMesh_ != NULL)
	{
		static float s_boxSize = 0.6f;
		static float s_boxHeight = 0.01f;
		static float s_offset = 0.f;
		static float s_length = 0.f;
		/*static bool s_added = false;
		if (!s_added)
		{
			s_added = true;
			MF_WATCH( "Show Hit Regions", g_showHitRegion );			 
			MF_WATCH( "Box Size", s_boxSize );
			MF_WATCH( "Box Height", s_boxHeight );
			MF_WATCH( "Offset", s_offset );
			MF_WATCH( "Length", s_length );
		}*/
		//increase hit region to cover mesh.
		boxHeight = s_boxHeight;
		boxSize = s_boxSize;
		offset = s_offset;
		length = s_length;
	}
	float pos1 = length + offset;
	float pos2 = length + offset + boxSize;
	float pos3 = length + offset + boxSize + boxSize;
	Vector3 min1( pos1, -boxHeight/2.f, pos2 );
	Vector3 max1( pos3, boxHeight/2.f, pos3 );

	Vector3 min2( pos2, -boxHeight/2.f, pos1 );
	Vector3 max2( pos3, boxHeight/2.f, pos2 );

	selectionMesh_.transform( Matrix::identity );
	selectionMesh_.addBox( min1, max1, 0xffffff00, new PositionShapePart( Moo::Colour( 1, 1, 0, 0 ), 1 ) );
	selectionMesh_.addBox( min2, max2, 0xffffff00, new PositionShapePart( Moo::Colour( 1, 1, 0, 0 ), 1 ) );

	if ( !snapToTerrainEnabled() && !isPlanar_ )
	{
		m.setRotateZ( DEG_TO_RAD( 90 ) );
		selectionMesh_.transform( m );
		selectionMesh_.addBox( min1, max1, 0xffff00ff, new PositionShapePart( Moo::Colour( 1, 0, 1, 0 ), 0 ) );
		selectionMesh_.addBox( min2, max2, 0xffff00ff, new PositionShapePart( Moo::Colour( 1, 0, 1, 0 ), 0 ) );

		m.setRotateX( DEG_TO_RAD( -90 ) );
		selectionMesh_.transform( m );
		selectionMesh_.addBox( min1, max1, 0xff00ffff, new PositionShapePart( Moo::Colour( 0, 1, 1, 0 ), 2 ) );
		selectionMesh_.addBox( min2, max2, 0xff00ffff, new PositionShapePart( Moo::Colour( 0, 1, 1, 0 ), 2 ) );
	}

	if( snapToObstableEnabled() )
	{
		BW::vector<BW::string>	modelNames;
		modelNames.push_back( s_pointerModel.value() );
		pModel_ = new SuperModel( modelNames );
	}
	else
		pModel_ = NULL;
}

/**
 *	Destructor.
 */
PositionGizmo::~PositionGizmo()
{
	BW_GUARD;

	bw_safe_delete(pModel_);// eventually we've got sth. to do here
}

bool PositionGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	active_ = false;
	if ( !force && (InputDevices::modifiers() & disablerModifiers_) != 0 )
		return false;
	active_ = true;

	init();

	rebuildMesh( false );
	if ( pModel_ && currentPart_ && currentPart_->isFree()
		&& snapToObstableEnabled() &&
		lastTool_ && lastTool_ == ToolManager::instance().tool() )
	{
		return true;
	}

	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	if (pDrawMesh_)
	{
		// set a temporary sunlight to filter the axes colour.
		Moo::SunLight oldSun = rc.effectVisualContext().sunLight();
		Moo::SunLight gizmoSun = oldSun;
		gizmoSun.m_ambient = lightColour_;
		gizmoSun.m_color = lightColour_;
		rc.effectVisualContext().sunLight(gizmoSun);
		rc.effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_FRAME );
		rc.setPixelShader( NULL );		

		rc.push();
		rc.world( gizmoTransform() );
		pDrawMesh_->draw( drawContext );
		rc.pop();

		const Moo::DrawContext::ChannelMask gizmoChannelMask = Moo::DrawContext::TRANSPARENT_CHANNEL_MASK;
		drawContext.end( gizmoChannelMask);
		drawContext.flush( gizmoChannelMask );
		drawContext.begin( gizmoChannelMask );

		rc.effectVisualContext().sunLight(oldSun);
		rc.effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_FRAME );
	}
	
	if (!pDrawMesh_ || g_showHitRegion)
	{
		rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
		Moo::Material::setVertexColour();

		rc.setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		rc.setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
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
		pDev->SetTransform( D3DTS_PROJECTION, &rc.projection() );
		rc.setPixelShader( NULL );
		rc.setVertexShader( NULL );
		rc.setFVF( Moo::VertexXYZND::fvf() );
	
		selectionMesh_.draw(rc);
	}

	return true;
}

bool PositionGizmo::intersects( const Vector3& origin,
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
	ld /= l;

	currentPart_ = (PositionShapePart*)selectionMesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );

	t /= l;

	return currentPart_ != NULL;
}

void PositionGizmo::click( const Vector3& origin, const Vector3& direction )
{
	BW_GUARD;

	// handle the click
	if (matrixProxy_)	// i.e. not using the common properties
		matrixProxy_->recordState();

	if (currentPart_ != NULL) 
	{
		ToolFunctorPtr toolFunctor = NULL;
		if (matrixProxy_)
			toolFunctor = ToolFunctorPtr( new MatrixPositioner( matrixProxy_ ), true );
		else
		{
			if( SnapProvider::instance()->snapMode() == SnapProvider::SNAPMODE_OBSTACLE )
			{
				if( currentPart_->isFree() && CurrentPositionProperties::properties().size() == 1 )
					toolFunctor = ToolFunctorPtr( new MatrixMover( matrixProxy_, true, true ), true );
				else
					toolFunctor = ToolFunctorPtr( new MatrixMover( matrixProxy_, false, false ), true );
			}
			else
				toolFunctor = ToolFunctorPtr( new MatrixMover( matrixProxy_, true, false ), true );
		}

		if (currentPart_->isPlane() && isPlanar_ &&
			ToolManager::instance().tool() &&
			ToolManager::instance().tool()->locator())
		{
			ToolPtr moveTool( new Tool( ToolManager::instance().tool()->locator(),
				NULL,
				toolFunctor
				), true );
			this->pushTool( moveTool );
		}
		else if( currentPart_->isFree() )
		{
			if( snapToObstableEnabled() )
			{
				Vector3 normal = getCoordModifier().applyVector( currentPart_->plane().normal() );
				normal.normalise();

				PlaneEq	peq( this->objectTransform().applyToOrigin(),normal );

				ToolPtr moveTool( new Tool( ToolLocatorPtr( new PlaneToolLocator( &peq ), true ),
					NULL,
					toolFunctor
					), true );
				this->pushTool( moveTool );
			}
			else
			{
				PlaneEq	peq( this->objectTransform().applyToOrigin(),
					Moo::rc().invView().applyToOrigin() - this->objectTransform().applyToOrigin() );

				ToolPtr moveTool( new Tool(
					ToolLocatorPtr( new PlaneToolLocator( &peq ), true ),
					NULL,
					toolFunctor
					), true );
				this->pushTool( moveTool );
			}
		}
		else
		if (currentPart_->isPlane())
		{
			Vector3 normal = getCoordModifier().applyVector( currentPart_->plane().normal() );
			normal.normalise();

			PlaneEq	peq( this->objectTransform().applyToOrigin(), normal );
			
			ToolPtr moveTool( new Tool(
				ToolLocatorPtr( new PlaneToolLocator( &peq ), true ),
				NULL,
				toolFunctor
				), true );
			this->pushTool( moveTool );
		}
		else
		{
			Vector3 dir = getCoordModifier().applyVector( currentPart_->direction() );
			dir.normalise();

			ToolPtr moveTool( new Tool(
				ToolLocatorPtr( new LineToolLocator( 
					this->objectTransform().applyToOrigin(), dir ), true ),
				NULL,
				toolFunctor
				), true );
			this->pushTool( moveTool );
		}
	}
}

void PositionGizmo::rollOver( const Vector3& origin, const Vector3& direction )
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

Matrix PositionGizmo::objectTransform() const
{
	BW_GUARD;

	Matrix m;
	
	if (matrixProxy_)
	{
		matrixProxy_->getMatrix(m);
	}
	else
	{
		m.setTranslate( CurrentPositionProperties::averageOrigin() );
	}

	if (visualOffsetMatrix_)
	{
		// move the model
		Matrix mat;
		visualOffsetMatrix_->getMatrix(mat);
		m.postTranslateBy(mat.applyToOrigin());
	}

	return m;
}


void PositionGizmo::setVisualOffsetMatrixProxy( MatrixProxyPtr matrix )
{
	visualOffsetMatrix_ = matrix;
}

bool PositionGizmo::snapToObstableEnabled() const
{
	BW_GUARD;

	return SnapProvider::instance()->snapMode() == SnapProvider::SNAPMODE_OBSTACLE &&
		CurrentPositionProperties::properties().size() == 1;
}

bool PositionGizmo::snapToTerrainEnabled() const
{
	BW_GUARD;

	return SnapProvider::instance()->snapMode() == SnapProvider::SNAPMODE_TERRAIN;
}

void PositionGizmo::pushTool( ToolPtr pTool )
{
	BW_GUARD;
	lastTool_ = pTool;
	Gizmo::pushTool( pTool );
}

BW_END_NAMESPACE

// position_gizmo.cpp
