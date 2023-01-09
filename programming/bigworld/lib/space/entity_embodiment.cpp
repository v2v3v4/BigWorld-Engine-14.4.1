#include "pch.hpp"

#include "entity_embodiment.hpp"

#include <space/client_space.hpp>
#include <space/space_manager.hpp>

namespace BW
{

IEntityEmbodiment::IEntityEmbodiment( const ScriptObject& object ) :
	object_( object )
{

}


IEntityEmbodiment::~IEntityEmbodiment()
{

}


void IEntityEmbodiment::enterSpace( ClientSpacePtr pSpace, bool transient )
{
	space_ = pSpace;
	doEnterSpace( pSpace, transient );
}


void IEntityEmbodiment::leaveSpace( bool transient )
{
	doLeaveSpace( transient );
	space_ = NULL;
}

void IEntityEmbodiment::move( float dTime )
{
	return doMove( dTime );
}


void IEntityEmbodiment::worldTransform( const Matrix & transform )
{
	return doWorldTransform( transform );
}


const Matrix & IEntityEmbodiment::worldTransform() const
{
	return doWorldTransform();
}


const AABB & IEntityEmbodiment::localBoundingBox() const
{
	return doLocalBoundingBox();
}


void IEntityEmbodiment::draw( Moo::DrawContext& drawContext )
{
	return doDraw( drawContext );
}


bool IEntityEmbodiment::isOutside() const
{
	return doIsOutside();
}


bool IEntityEmbodiment::isRegionLoaded( Vector3 testPos, float radius ) 
	const
{
	return doIsRegionLoaded( testPos, radius );
}


const ScriptObject& IEntityEmbodiment::scriptObject()
{
	return object_;
}


const ClientSpacePtr& IEntityEmbodiment::space() const
{
	return space_;
}


/// Hack method to pass a space pointer through python
static bool getSpace( PyObject * pSpaceInformer, ClientSpace * & pCS )
{
	BW_GUARD;
	PyObject * pCSInt = PyObject_GetAttrString(
		pSpaceInformer, ".space" );

	bool good = true;
	if (pCSInt == NULL || Script::setData( pCSInt, (int&)pCS ) != 0)
	{
		PyErr_SetString( PyExc_SystemError,
			"Owner of embodiment list won't tell its space!" );
		good = false;
	}

	Py_XDECREF( pCSInt );

	return good;
}

/// Release method releases the reference originally taken by setData
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
void PySTLObjectAid::Releaser< IEntityEmbodimentPtr >::release(
	IEntityEmbodimentPtr & pCA )
{
	BW_GUARD;
	//DEBUG_MSG( "Deleting embodiment at 0x%08X\n", &*pCA );
	pCA = NULL;
}


/// Take a-hold of this model (put in world)
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
bool PySTLObjectAid::Holder< EntityEmbodiments >::hold(
	IEntityEmbodimentPtr & pCA, PyObject * pOwner )
{
	BW_GUARD;
	//DEBUG_MSG( "Holder<ChunkEmbodiments>::hold: "
	//	"Calling hold for chunk attachment at 0x%08X\n", &*pCA );

	if (!pCA)
	{
		PyErr_SetString( PyExc_ValueError,
			"Model to add to entity list cannot be None" );
		return false;
	}

	ClientSpace * pSpace = NULL;
	if (!getSpace( pOwner, pSpace ))
	{
		return false;
	}

	if (pSpace != NULL)
	{
		pCA->enterSpace( pSpace );
	}

	return true;
}


// Let go of this model (remove from world)
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
void PySTLObjectAid::Holder< EntityEmbodiments >::drop(
	IEntityEmbodimentPtr & pCA, PyObject * pOwner )
{
	BW_GUARD;
	//DEBUG_MSG( "Calling drop for model at 0x%08X\n", &*pCA );

	MF_ASSERT_DEV( pCA );

	ClientSpace * pSpace = NULL;
	if (!getSpace( pOwner, pSpace ))
	{
		PyErr_Clear();
	}
	if (pSpace != NULL && pCA.exists())
	{
		pCA->leaveSpace();
	}
}


/// IEntityEmbodiment converter
PyObject * Script::getData( const IEntityEmbodimentPtr pCA )
{
	BW_GUARD;
	if (pCA)
	{
		PyObject * pPyObject = &*pCA->scriptObject();
		Py_INCREF( pPyObject );
		return pPyObject;
	}

	Py_RETURN_NONE;
}


/// IEntityEmbodiment converter
int Script::setData( PyObject * pObj, IEntityEmbodimentPtr & pCA,
	const char * varName )
{
	BW_GUARD;
	IEntityEmbodimentPtr pNew = NULL;

	// look at the object
	if (pObj != Py_None)
	{
		pNew = SpaceManager::instance().createEntityEmbodiment( 
			ScriptObject( pObj ) );
		
		if (!pNew)
		{
			// Only raise exception if the factory didn't already.
			if (!PyErr_Occurred())
			{
				PyErr_Format( PyExc_TypeError, "%s must be set to an object "
					"that can become an Embodiment", varName );
			}
			return -1;
		}
	}

	// now replace the pointer
	pCA = pNew;

	return 0;
}

} // namespace BW