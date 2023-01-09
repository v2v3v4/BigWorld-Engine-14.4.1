#include "pch.hpp"
#include <algorithm>
#include "gizmo_manager.hpp"
#include "coord_mode_provider.hpp"
#include "current_general_properties.hpp"
#include "gizmo/tool_manager.hpp"
#include "moo/visual_manager.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"
#include "pyscript/pyobject_plus.hpp"

#ifndef CODE_INLINE
#include "gizmo_manager.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Display", 2 );

BW_BEGIN_NAMESPACE

//information shared by individual gizmos
bool g_showHitRegion = false;
Moo::Colour g_unlit( 0.75f, 0.75f, 0.75f, 0.75f );


/**
 *	This method returns a transform that is used by gizmos to
 *	draw.  It gives the gizmo a constant on-screen size, no matter
 *	how far away the object transform is.
 */
Matrix Gizmo::gizmoTransform() const
{
	BW_GUARD;

	Vector3 pos = objectTransform()[3];

	Matrix m = getCoordModifier();
	m[3].setZero();
	m[0].normalise();
	m[1].normalise();
	m[2].normalise();

	float scale = ( Moo::rc().invView()[2].dotProduct( pos ) -
		Moo::rc().invView()[2].dotProduct( Moo::rc().invView()[3] ) );
	if (scale > 0.05)
		scale /= 25.f;
	else
		scale = 0.05f / 25.f;

	Matrix scaleMat;
	scaleMat.setScale( scale, scale, scale );

	m.postMultiply( scaleMat );

	m[3] = pos;
	return m;
}


/**
 *  Get the matrix used to modifiy positions based upon the coordinate mode.
 *
 *  @returns        The matrix used to modify the positions.
 */
Matrix Gizmo::getCoordModifier() const
{
	BW_GUARD;

	Matrix coord;
	if 
    (
        CoordModeProvider::ins()->getCoordMode() 
        == 
        CoordModeProvider::COORDMODE_OBJECT
    )
	{
		return objectCoord();
	}
	else if
    ( 
        CoordModeProvider::ins()->getCoordMode() 
        == 
        CoordModeProvider::COORDMODE_VIEW 
    )
	{
		coord = Moo::rc().invView();
	}
	else
    {
		coord.setIdentity();
    }
	return coord;
}


/**
 *	Get the matrix used to modify positions when in object coordinate mode.
 */
/*virtual*/ Matrix Gizmo::objectCoord() const
{
	BW_GUARD;

	Matrix coord;
	if (CurrentPositionProperties::properties().size() == 1)
	{
		CurrentPositionProperties::properties()[0]->pMatrix()->getMatrix(coord);
	}
	else
	{
		coord.setIdentity();
	}
	return coord;
}


/**
 *	Stop applying current tool and push a new one.
 */
/*virtual*/ void Gizmo::pushTool( ToolPtr pTool )
{
	BW_GUARD;
	if (ToolManager::instance().isToolApplying())
	{
		ToolManager::instance().tool()->stopApplying( true );
	}
	ToolManager::instance().pushTool( pTool );
}


// -----------------------------------------------------------------------------
// Section: GizmoSet
// -----------------------------------------------------------------------------

GizmoSet::GizmoSet()
{
}

void GizmoSet::clear()
{
	BW_GUARD;

	gizmos_.clear();
}

void GizmoSet::add( GizmoPtr gizmo )
{
	BW_GUARD;

	gizmos_.push_back( gizmo );
}

const GizmoVector& GizmoSet::vector()
{
	return gizmos_;
}




// -----------------------------------------------------------------------------
// Section: GizmoManager
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GizmoManager::GizmoManager()
{
	MF_WATCH( "Render/Show Gizmo Hit Region", g_showHitRegion );
}


/**
 *	Destructor.
 */
GizmoManager::~GizmoManager()
{	
}


/**
 *	This method returns the single instance of the GizmoManager.
 */
GizmoManager & GizmoManager::instance()
{
	static GizmoManager s_instance;

	return s_instance;
}


/**
 *	This method draws all cached gizmos.
 */
void GizmoManager::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	GizmoVector::iterator begin = gizmos_.begin();
	GizmoVector::iterator end = gizmos_.end();
	bool force = !!forcedGizmoSet_;

	if (force)
	{
		begin = const_cast<GizmoVector&>( forcedGizmoSet_->vector() ).begin();
		end = const_cast<GizmoVector&>( forcedGizmoSet_->vector() ).end();
	}

	for ( GizmoVector::iterator it = begin; it != end; ++it )
	{
		(*it)->drawZBufferedStuff( force );
	}

	Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_ZBUFFER/* | D3DCLEAR_STENCIL*/, 0, 1, 0 );

	for ( GizmoVector::iterator it = begin; it != end; ++it )
	{
		(*it)->draw( drawContext, force );
	}
}

/**
 * This method updates the gizmo manager's internals.
 * @param worldRay the ray from the camera to the mouse
 * @return true if the ray intersects something
 */
bool GizmoManager::update( const Vector3& worldRay )
{
	BW_GUARD;

	lastWorldRay_ = worldRay;
	lastWorldOrigin_ = Moo::rc().invView()[3];
	intersectedGizmo_ = NULL;
	float distance = Moo::rc().camera().farPlane() * 2;

	bool force = !!forcedGizmoSet_;
	GizmoVector::iterator it = gizmos_.begin();
	GizmoVector::iterator end = gizmos_.end();

	if (force)
	{
		it = const_cast<GizmoVector&>( forcedGizmoSet_->vector() ).begin();
		end = const_cast<GizmoVector&>( forcedGizmoSet_->vector() ).end();
	}

	while (it != end)
	{
		GizmoPtr pGizmo = *it++;
		if (pGizmo->intersects( Moo::rc().invView()[3], worldRay, distance,
																	force ))
		{
			intersectedGizmo_ = pGizmo;
		}
	}

	return intersectedGizmo_.hasObject();
}

/**
 * This method should be called when the user clicks on a gizmo
 */
bool GizmoManager::click()
{
	BW_GUARD;

	if (intersectedGizmo_.hasObject())
	{
		intersectedGizmo_->click(lastWorldOrigin_, lastWorldRay_);
		return true;
	}
	return false;
}

/**
 * This method should be called when the user rolls over a gizmo
 */
bool GizmoManager::rollOver()
{
	BW_GUARD;

	if (intersectedGizmo_.hasObject())
	{
		intersectedGizmo_->rollOver(lastWorldOrigin_, lastWorldRay_);
		return true;
	}
	return false;
}

void GizmoManager::addGizmo( GizmoPtr pGizmo )
{
	BW_GUARD;

	if (pGizmo)
	{
		// check it doesn't already exist
		GizmoVector::iterator it = std::find( gizmos_.begin(), gizmos_.end(), pGizmo );
		if (it==gizmos_.end())
			gizmos_.push_back( pGizmo );
	}
}

void GizmoManager::removeGizmo( GizmoPtr pGizmo )
{
	BW_GUARD;

	GizmoVector::iterator it = std::find( gizmos_.begin(), gizmos_.end(), pGizmo );
	if (it!=gizmos_.end())
		gizmos_.erase( it );
	if( intersectedGizmo_ == pGizmo )
		intersectedGizmo_ = NULL;
}

void GizmoManager::removeAllGizmo( )
{
	BW_GUARD;

	gizmos_.clear();
}

void GizmoManager::forceGizmoSet( GizmoSetPtr gizmoSet )
{
	BW_GUARD;

	forcedGizmoSet_ = gizmoSet;
}

GizmoSetPtr GizmoManager::forceGizmoSet()
{
	return forcedGizmoSet_;
}

const Vector3& GizmoManager::getLastCameraPosition()
{
	return lastWorldOrigin_;
}

PY_MODULE_STATIC_METHOD( GizmoManager, gizmoUpdate, WorldEditor )
PY_MODULE_STATIC_METHOD( GizmoManager, gizmoClick, WorldEditor )

/*~ function WorldEditor.gizmoUpdate
 *	@components{ worldeditor }
 *	
 *	This method updates the gizmo manager's internals.
 *	
 *	@param worldRay The ray from the camera to the mouse position.
 * 
 *	@return Returns True if the ray intersects something, False otherwise.
 */
PyObject* GizmoManager::py_gizmoUpdate( PyObject* args )
{
	BW_GUARD;

	Vector3 ray;
	if (PyTuple_Size(args) != 1) 
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.gizmoUpdate expects a Vector3" );
		return NULL;
	}
	if (Script::setData( PyTuple_GetItem(args, 0 ), ray, "WorldEditor.gizmoUpdate argument" ) != 0 )
	{
		return NULL;
	}

	bool result = GizmoManager::instance().update( ray );
	if (result)
		GizmoManager::instance().rollOver();

	return Script::getData(result);
}

/*~ function WorldEditor.gizmoClick
 *	@components{ worldeditor }
 *
 *	This function tells the gizmo that the user has clicked it.
 */
PyObject* GizmoManager::py_gizmoClick( PyObject* args )
{
	BW_GUARD;

	if (PyTuple_Size(args) != 0) 
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.gizmoClick takes no arguments" );
		return NULL;
	}
	if (!GizmoManager::instance().click())
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.gizmoClick no gizmo to click" );
		return NULL;
	}
	Py_RETURN_NONE;
}



BW_END_NAMESPACE
// gizmo_manager.cpp
