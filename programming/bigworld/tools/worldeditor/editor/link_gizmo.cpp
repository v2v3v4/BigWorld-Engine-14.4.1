#include "pch.hpp"

#include "link_gizmo.hpp"

#include "cstdmf/debug.hpp"
#include "math/colour.hpp"
#include "gizmo/current_general_properties.hpp"
#include "gizmo/matrix_mover.hpp"
#include "gizmo/tool.hpp"
#include "gizmo/tool_manager.hpp"
#include "gizmo/coord_mode_provider.hpp"
#include "moo/effect_visual_context.hpp"
#include "resmgr/auto_config.hpp"
#include <limits>
#include "moo/visual_manager.hpp"
#include "worldeditor/editor/link_functor.hpp"
#include "worldeditor/editor/link_view.hpp"

DECLARE_DEBUG_COMPONENT2( "gizmo", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_glVisual( "editor/graphLinkGizmo" );
static AutoConfigString s_gaVisual( "editor/graphAddGizmo" );

extern bool g_showHitRegion;


// This is used in the mouse-collision test to designate what the mouse was
// actually over.
struct LinkGizmoColourPart : public ShapePart
{
    enum Type
    {
        LINK_LEFT_TYPE,
        LINK_RIGHT_TYPE,
        ADD_UPPER_TYPE,
        ADD_LOWER_TYPE
    };

    Moo::Colour     colour_;
    Type            type_;

    LinkGizmoColourPart(Moo::Colour c, Type type) : colour_(c), type_(type) {}
};


//
// Positions, coordinates and colours used to draw the gizmo.  These are
// publically available because other pieces of code may need some of them too.
//

/*static*/ Vector3  LinkGizmo::rightLinkCenter  = Vector3(+1.8f, 0.0f, 0.0f);
/*static*/ Vector3  LinkGizmo::leftLinkCenter   = Vector3(-1.8f, 0.0f, 0.0f);
/*static*/ float    LinkGizmo::linkRadius       = 0.5f;
/*static*/ DWORD    LinkGizmo::linkColour       = D3DCOLOR_RGBA(0, 0, 255, 128);

/*static*/ Vector3  LinkGizmo::centerAddUL      = Vector3(-0.0f, -0.0f, -1.5f);
/*static*/ Vector3  LinkGizmo::centerAddLR      = Vector3(-0.0f, -0.0f, +1.5f);
/*static*/ float    LinkGizmo::heightAdd        = 0.5f;
/*static*/ float    LinkGizmo::widthAdd         = 0.3f;
/*static*/ float    LinkGizmo::breadthAdd       = 0.7f;
/*static*/ DWORD    LinkGizmo::addColour        = D3DCOLOR_RGBA(0, 255, 0, 128);

/*static*/ DWORD    LinkGizmo::unlitColour      = D3DCOLOR_RGBA(128, 128, 128, 128);


/**
 *  LinkGizmo constructor.
 *
 *  @param linkProxy_       The proxy to consult for linking.
 *  @param center           The center of the object being linked.
 *  @param enableModifier   Keys used to temporarily disable the gizmo.
 */
LinkGizmo::LinkGizmo
(
    LinkProxyPtr        linkProxy,
    MatrixProxyPtr      center, 
    uint32              enablerModifier /*= MODIFIER_ALT*/
) : 
    linkProxy_(linkProxy),
    center_(center),
	active_(false),
	inited_(false),
	enablerModifier_(enablerModifier),
    hilightColour_(0, 0, 0, 0),
    linkDrawMesh_(NULL),
    addDrawMesh_(NULL),
    currentPart_(NULL)
{
}


/**
 *  LinkGizmo destructor.
 */
LinkGizmo::~LinkGizmo()
{
}


/**
 *  Changes the proxies.
 */
void LinkGizmo::update( LinkProxyPtr linkProxy, MatrixProxyPtr center )
{
	BW_GUARD;

    linkProxy_ = linkProxy;
    center_ = center;
}


void LinkGizmo::init()
{
	BW_GUARD;

	if (!inited_)
	{
		if ( !s_glVisual.value().empty() )
		{
			linkDrawMesh_ = Moo::VisualManager::instance()->get( s_glVisual );
		}
		if ( !s_gaVisual.value().empty() )
		{
			addDrawMesh_ = Moo::VisualManager::instance()->get( s_gaVisual );
		}
		//arbitrary rule - you must have both meshes, or none.
		if (!addDrawMesh_)
			linkDrawMesh_ = NULL;

		rebuildMeshes(true);
		inited_ = true;
	}
}


/**
 *  This is called to draw the gizmo.
 *
 *  @param force        Force drawing?
 */
/*virtual*/ bool LinkGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	active_ = false;
	if 
	(
		!force 
		&&
		enablerModifier_ != ALWAYS_ENABLED
		&&
		(InputDevices::modifiers() & enablerModifier_) != 0
	)
	{
        return false;
	}
	active_ = true;

	init();

    Moo::RenderContext  *rc     = &Moo::rc();
	DX::Device          *device = rc->device();

	rc->setPixelShader(NULL);
	rc->setVertexShader(NULL);

	if (linkDrawMesh_)
	{
		Moo::LightContainerPtr pOldLC = rc->lightContainer();
		Moo::LightContainerPtr pLC = new Moo::LightContainer;		
		pLC->ambientColour( hilightColour_ );
		rc->lightContainer( pLC );
		rc->setPixelShader( NULL );		

		rc->push();		
		rc->world( gizmoTransform() );
		if ((linkProxy_->linkType() & LinkProxy::LT_LINK)!= 0)
			linkDrawMesh_->draw( drawContext );
		if ((linkProxy_->linkType() & LinkProxy::LT_ADD) != 0)
			addDrawMesh_ ->draw( drawContext );
		rc->pop();

		rc->lightContainer( pOldLC );
		drawContext.end( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
		drawContext.flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
		drawContext.begin( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
	}

	if (!linkDrawMesh_ || g_showHitRegion)
	{
		rc->setRenderState(D3DRS_ALPHABLENDENABLE, TRUE                 );
		rc->setRenderState(D3DRS_SRCBLEND        , D3DBLEND_SRCALPHA    );
		rc->setRenderState(D3DRS_DESTBLEND       , D3DBLEND_INVSRCALPHA );
		rc->setRenderState(D3DRS_LIGHTING        , FALSE                );

		rc->setTextureStageState(0, D3DTSS_COLOROP  , D3DTOP_MODULATE);
		rc->setTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE  );
		rc->setTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR  );
		rc->setTextureStageState(0, D3DTSS_ALPHAOP  , D3DTOP_DISABLE );
		rc->setTextureStageState(1, D3DTSS_COLOROP  , D3DTOP_DISABLE );

		Matrix transform = gizmoTransform();
		device->SetTransform(D3DTS_WORLD     , &transform       );
		device->SetTransform(D3DTS_VIEW      , &rc->view()      );
		device->SetTransform(D3DTS_PROJECTION, &rc->projection());

		uint32 tfactor = hilightColour_;
		rc->setRenderState(D3DRS_TEXTUREFACTOR, tfactor);		
		rc->setFVF(Moo::VertexXYZND::fvf());

		if ((linkProxy_->linkType() & LinkProxy::LT_LINK) != 0)
			linkSelectionMesh_.draw(*rc);
		if ((linkProxy_->linkType() & LinkProxy::LT_ADD) != 0)
			addSelectionMesh_.draw(*rc);
	}

    return true;
}


/**
 *  This is called to test intersection with the gizmo.
 *
 *  @param origin       The origin of the cast ray.
 *  @param direction    The direction of the case ray.
 *  @param t            The t-value of intersection (if there is an 
 *                      intersection).
 *  @param force        Wheather or not the gizmo is being force-drawn.
 *  @returns            True if there is an intersection of the ray with the
 *                      gizmo.
 */
/*virtual*/ bool 
LinkGizmo::intersects( Vector3 const &origin, Vector3 const &direction, 
														float &t, bool force )
{
	BW_GUARD;

	if ( !active_ )
	{
		currentPart_ = NULL;
		return false;
	}

	init();

    Matrix m = gizmoTransform();
    m.invert();

    Vector3 originX = m.applyPoint(origin);
    Vector3 dirX    = m.applyVector(direction);

    float length = dirX.length();
    t *= length;
    dirX /= length;

    bool hit = false;
    t = std::numeric_limits<float>::max();
	hilightColour_ = Moo::Colour(unlitColour);
    currentPart_ = NULL;

    if ((linkProxy_->linkType() & LinkProxy::LT_LINK) != 0)
    {
        float lt = t;
        LinkGizmoColourPart *linkIntersect =
            (LinkGizmoColourPart *)
            linkSelectionMesh_.intersects
            (
                originX,
                m.applyVector(direction),
                &lt
            );
        if (linkIntersect != NULL)
        {
            hit = true;
            float thist = lt/length;
            if (thist < t)
            {
                t = thist;
                currentPart_ = linkIntersect;
                currentPos_ = origin + t*direction;
            }
        }
    }

    if ((linkProxy_->linkType() & LinkProxy::LT_ADD) != 0)
    {
        float st = t;
        LinkGizmoColourPart *addIntersect =
            (LinkGizmoColourPart *)
            addSelectionMesh_.intersects
            (
                originX,
                m.applyVector(direction),
                &st
            );
        if (addIntersect != NULL)
        {
            hit = true;
            float thist = st/length;
            if (thist < t)
            {
                t = thist;
                currentPart_ = addIntersect;
                currentPos_ = origin + t*direction;
            }
        }
    }

    return hit;
}


/**
 *  This is called when the user clicks the gizmo.
 *
 *  @param origin       The origin of the cast ray, used to select the gizmo.
 *  @param direction    The direction of the cast ray, used to select the 
 *                      gizmo.
 */
void LinkGizmo::click(Vector3 const &origin, Vector3 const &direction)
{
	BW_GUARD;

    if (currentPart_ == NULL)
        return;

    if 
    (
        currentPart_->type_ == LinkGizmoColourPart::ADD_UPPER_TYPE
        ||
        currentPart_->type_ == LinkGizmoColourPart::ADD_LOWER_TYPE
    )
    {
        clickAdd
        (
            origin, 
            direction,
            currentPart_->type_ == LinkGizmoColourPart::ADD_UPPER_TYPE
        );
    }
    else
    {
        clickLink
        (
            origin, 
            direction, 
            currentPart_->type_ == LinkGizmoColourPart::LINK_LEFT_TYPE
        );
    }
}


/**
 *  This is called when the user hovers the mouse over the gizmo.
 *
 *  @param origin       The origin of the cast ray, used to hover over the 
 *                      gizmo.
 *  @param direction    The direction of the cast ray, used to hover over the 
 *                      gizmo.
 */
void 
LinkGizmo::rollOver
(
    Vector3         const &/*origin*/, 
    Vector3         const &/*direction*/
)
{
	BW_GUARD;

    if (currentPart_ != NULL)
    {
        hilightColour_ = currentPart_->colour_;
    }
    else
    {
        hilightColour_ = Moo::Colour(unlitColour);
    }
}


/**
 *  This is called to (re)build the gizmo's meshes.
 *
 *  @param force        Force a rebuild.
 */
void LinkGizmo::rebuildMeshes(bool force)
{
	BW_GUARD;

    if (!force)
        return;

	Matrix rotY;
	rotY.setRotateY( DEG_TO_RAD(45.f) );

    linkSelectionMesh_.clear();
    linkSelectionMesh_.transform(rotY);
    linkSelectionMesh_.addSphere
    (
        rightLinkCenter, 
        linkRadius, 
        linkColour,
        new LinkGizmoColourPart
        (
            Moo::Colour(linkColour),
            LinkGizmoColourPart::LINK_RIGHT_TYPE
        )
    );
    linkSelectionMesh_.addSphere
    (
        leftLinkCenter, 
        linkRadius, 
        linkColour,
        new LinkGizmoColourPart
        (
            Moo::Colour(linkColour),
            LinkGizmoColourPart::LINK_LEFT_TYPE
        )
    );

    addSelectionMesh_.clear();
    addSelectionMesh_.transform(rotY);
    addSelectionMesh_.addBox
    (
        centerAddUL + Vector3(-0.5f*widthAdd  , -0.5f*heightAdd, -0.5f*breadthAdd), 
        centerAddUL + Vector3(+0.5f*widthAdd  , +0.5f*heightAdd, +0.5f*breadthAdd),
        addColour,                            
        new LinkGizmoColourPart               
        (                                     
            Moo::Colour(addColour),           
            LinkGizmoColourPart::ADD_UPPER_TYPE     
        )                                     
    );                                        
    addSelectionMesh_.addBox                           
    (                                         
        centerAddUL + Vector3(-0.5f*breadthAdd, -0.5f*heightAdd, -0.5f*widthAdd), 
        centerAddUL + Vector3(+0.5f*breadthAdd, +0.5f*heightAdd, +0.5f*widthAdd),
        addColour,                            
        new LinkGizmoColourPart               
        (                                     
            Moo::Colour(addColour),           
            LinkGizmoColourPart::ADD_UPPER_TYPE     
        )                                     
    );                                        
    addSelectionMesh_.addBox                           
    (                                         
        centerAddLR + Vector3(-0.5f*widthAdd  , -0.5f*heightAdd, -0.5f*breadthAdd), 
        centerAddLR + Vector3(+0.5f*widthAdd  , +0.5f*heightAdd, +0.5f*breadthAdd),
        addColour,
        new LinkGizmoColourPart
        (
            Moo::Colour(addColour),
            LinkGizmoColourPart::ADD_LOWER_TYPE
        )
    );
    addSelectionMesh_.addBox
    (
        centerAddLR + Vector3(-0.5f*breadthAdd, -0.5f*heightAdd, -0.5f*widthAdd), 
        centerAddLR + Vector3(+0.5f*breadthAdd, +0.5f*heightAdd, +0.5f*widthAdd),
        addColour,
        new LinkGizmoColourPart
        (
            Moo::Colour(addColour),
            LinkGizmoColourPart::ADD_LOWER_TYPE
        )
    );
}


/**
 *  This returns the gizmo's position and orientation.
 *
 *  @returns            The gizmo's position and orientation.
 */
Matrix LinkGizmo::gizmoTransform() const
{
	BW_GUARD;

	Vector3 pos = objectTransform()[3];

	Matrix m;
    CurrentPositionProperties::properties()[0]->pMatrix()->getMatrix(m);
	m[3].setZero();
	m[0].normalise();
	m[1].normalise();
	m[2].normalise();

    Moo::RenderContext *rc = &Moo::rc();

	float scale = 
        (rc->invView()[2].dotProduct(pos) - rc->invView()[2].dotProduct(rc->invView()[3]));
	if (scale > 0.05)
		scale /= 25.f;
	else
		scale = 0.05f/25.f;

	Matrix scaleMat;
	scaleMat.setScale(scale, scale, scale);

	m.postMultiply(scaleMat);

	m[3] = pos;

    return m;
}


/**
 *  This returns the position/orientation of the object being linked.
 *
 *  @returns            The position/orientation of the linked item.
 */
Matrix LinkGizmo::objectTransform() const
{
	BW_GUARD;

	Matrix m;
	
	if (center_)
	{
		center_->getMatrix(m);
	}
	else
	{
		m.setTranslate(CurrentPositionProperties::averageOrigin());
	}

    return m;
}


/**
 *  This is called when the user clicks on the 'add' part of the gizmo.  It 
 *  calls the linkProxy_ to create a copy of the linked item and to link the
 *  new item and the old one.  The linkProxy_ should also set the selection to
 *  the new item and return a MatrixProxy for it so the position of the new
 *  item can be modified.
 *
 *  @param origin       The origin of the cast ray, used to select the gizmo.
 *  @param direction    The direction of the cast ray, used to select the 
 *                      gizmo.
 *  @param upper        True if the upper add node was clicked, false if it was
 *                      the lower.
 */
void 
LinkGizmo::clickAdd
(
    Vector3         const &/*origin*/, 
    Vector3         const &direction,
    bool            /*upper*/
)
{
	BW_GUARD;

    // The link proxy can set WorldEditor's selection, which in turn can delete
    // this object underneath us because of the change of tool.  To prevent 
    // premature deletion we increase the reference count by one at the start
    // of this function and decrease it by one at the end.  Hence, if there is
    // a deletion, it occurs at the end of the function.
    incRef();

    // Create a copy of the linked item, link it to the current item, select
    // the new item and get the position proxy for it:
    MatrixProxyPtr newObjPosProxy = linkProxy_->createCopyForLink();

    // Translate the new object to the selection point:
    Matrix xform;
    newObjPosProxy->getMatrix(xform);
    xform.setTranslate(currentPos_);
    Matrix worldToLocal;
    newObjPosProxy->getMatrixContextInverse(worldToLocal);
    xform.postMultiply(worldToLocal);
    newObjPosProxy->setMatrix(xform);
    newObjPosProxy->commitState(false, false);

    // Start moving the new object:
	// Do not discard changes from moving the link as there is no way to undo
	// adding it and we don't want it to snap back on top of the first one if
	// the user cancels movement 
    ToolLocatorPtr locator = linkProxy_->createLocator();
    ToolFunctorPtr functor = 
	ToolFunctorPtr(new MatrixMover(newObjPosProxy, true, true, false), true);

	ToolPtr moveTool(new Tool(locator, NULL, functor), true);

	// Make this the currently active tool before updating
	this->pushTool( moveTool );

    // Update the tool immediately.  If we don't do this then the mouse can
    // move between now and when the first move occurs.  If this happens then
    // the node gets dragged around with a noticeable offset.
    locator->calculatePosition(direction, *moveTool);
    functor->update(0.0f, *moveTool);

    decRef();
}


/**
 *  This is called when the user clicks on the 'link' part of the gizmo.  It 
 *  creates a linking tool that can be moved around to link to this object.
 *
 *  @param origin       The origin of the cast ray, used to select the gizmo.
 *  @param direction    The direction of the cast ray, used to select the 
 *                      gizmo.
 *  @param left         True if the user clicked on the 'left' link part of the
 *                      gizmo.
 */
void 
LinkGizmo::clickLink
(
    Vector3         const &/*origin*/, 
    Vector3         const &/*direction*/,
    bool            /*left*/
)
{
	BW_GUARD;

	ToolPtr linkTool
        ( 
            new Tool
            (
                linkProxy_->createLocator(),
				ToolViewPtr(new LinkView(linkProxy_, gizmoTransform()), true),
				ToolFunctorPtr(new LinkFunctor(linkProxy_), true)
			), 
            true 
        );
	this->pushTool(linkTool);
}
BW_END_NAMESPACE

