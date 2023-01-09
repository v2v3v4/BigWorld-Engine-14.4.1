#include "pch.hpp"
#include "py_attachment.hpp"
#include "duplo/pymodelnode.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyAttachment
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyAttachment )

PY_BEGIN_METHODS( PyAttachment )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAttachment )
	/*~ attribute PyAttachment.attached
	 *
	 *	This attribute indicates whether or not this attachment is already
	 *	attached to something.  It is 1 if the attachment is attached, 0
	 *	otherwise.  
	 *
	 *	This attribute should be checked before trying to attach to something, 
	 *  because you can't attach to something if it is already attached to
	 *	something else.  First you must detach the PyAttachment from whatever
	 *	it is attached to.  If this is an entity, you can set the entity's model
	 *	attribute to None.  If it is attached to a PyModelNode, you can call
	 *	the detach function. 
	 *
	 *	@type	Read-Only Integer
	 */
	PY_ATTRIBUTE( attached )
	/*~ attribute PyAttachment.inWorld
	 *
	 *	This attribute is used to determine whether the attachment has been
	 *	added to the world.  It is read only, and is set to 1 if the Attachment
	 *	is in the world, 0 otherwise.  
	 *
	 *	@type	Read-Only Integer
	 */
	PY_ATTRIBUTE( inWorld )
	/*~ attribute PyAttachment.matrix
	 *
	 *	This matrix is the transformation between the frame of reference of the
	 *	object the PyAttachment is attached to, and the PyAttachment's root
	 *	node.
	 *
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( matrix )

	// assign this attribute BEFORE attaching this PyAttachment (i.e. via BigWorld.addModel)
	PY_ATTRIBUTE( castsShadow )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyAttachment )



/**
 *	Constructor
 */
PyAttachment::PyAttachment( PyTypeObject * pType ) :
	PyObjectPlusWithWeakReference( pType ),
	pOwnWorld_( NULL ),
	attached_( false ),
	inWorld_( false ),
	castsShadow_(true)
{
	BW_GUARD;
	// matrix liaison when not in world
	pOwnWorld_ = new ModelMatrixLiaison();
}

/**
 *	Destructor
 */
PyAttachment::~PyAttachment()
{
	BW_GUARD;

	MF_ASSERT_DEV( !attached_ );
	MF_ASSERT_DEV( !inWorld_ );

	bw_safe_delete(pOwnWorld_);
}


/**
 *	Basic attach method
 */
bool PyAttachment::attach( MatrixLiaison * pOwnWorld )
{
	BW_GUARD;
	if (attached_)
	{
		return false;
	}

	SCOPED_DISABLE_FRAME_BEHIND_WARNING;
	pOwnWorld->setMatrix( pOwnWorld_->getMatrix() );

	// clear matrix liaison for when not in world
	bw_safe_delete(pOwnWorld_);

	attached_ = true;
	pOwnWorld_ = pOwnWorld;
	return true;
}

/**
 *	Basic detach method
 */
void PyAttachment::detach()
{
	BW_GUARD;
	if (!attached_) 
	{
		return;
	}
	
	attached_ = false;

	MatrixLiaison* m = new ModelMatrixLiaison();

	SCOPED_DISABLE_FRAME_BEHIND_WARNING;
	m->setMatrix( pOwnWorld_->getMatrix() );

	pOwnWorld_ = m;
}


/**
 *	Basic enterWorld method
 */
void PyAttachment::enterWorld()
{
	inWorld_ = true;
}

/**
 *	Basic leaveWorld method
 */
void PyAttachment::leaveWorld()
{
	inWorld_ = false;
}

/**
 *	Helper class to access the matrix at which attachments are drawn.
 */
class PyAttachmentMatrixProvider : public MatrixProvider
{
public:
	PyAttachmentMatrixProvider( PyAttachmentPtr pAttachment ) :
		MatrixProvider( false, &s_type_ ),
		pAttachment_( pAttachment )
	{ }

private:
	virtual void matrix( Matrix & m ) const
	{
		m = pAttachment_->worldTransform();
	}

	PyAttachmentPtr pAttachment_;
};


/**
 *	Get a MatrixProvider for this attachment
 */
MatrixProvider * PyAttachment::matrix()
{
	BW_GUARD;
	return new PyAttachmentMatrixProvider( 
		PyAttachmentPtr( this ) );
}


/**
 *	Method to find out if owner is in the world
 */
static bool isOwnerInWorld( PyObject * pOwner )
{
	BW_GUARD;
	PyObject * pResult = PyObject_GetAttrString( pOwner, "inWorld" );
	bool result = false;
	if (Script::setData( pResult, result ) != 0)
	{
		PyErr_Clear();
	}
	return result;
}


/// Take a-hold of this model (put in world)
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
bool PySTLObjectAid::Holder< PyAttachments >::hold(
	PyAttachmentPtr & pAttachment, PyObject * pOwner )
{
	BW_GUARD;
	if (!pAttachment)
	{
		PyErr_SetString( PyExc_ValueError,
			"Attachment to add to vector cannot be None" );
		return false;
	}

	if (pAttachment->isAttached())
	{
		PyErr_SetString( PyExc_TypeError,
			"Attachment to add to vector cannot be attached elsewhere" );
		return false;
	}

	pAttachment->attach( MatrixProviderLiaison::Check( pOwner ) ?
		(MatrixProviderLiaison*)pOwner : NULL );
	if (isOwnerInWorld( pOwner )) pAttachment->enterWorld();

	return true;
}


// Let go of this model (remove from world)
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
void PySTLObjectAid::Holder< PyAttachments >::drop(
	PyAttachmentPtr & pAttachment, PyObject * pOwner )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pAttachment )
	{
		return;
	}

	if (isOwnerInWorld( pOwner )) pAttachment->leaveWorld();
	pAttachment->detach();
}



// -----------------------------------------------------------------------------
// Section: MatrixProviderLiaison
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( MatrixProviderLiaison )

PY_BEGIN_METHODS( MatrixProviderLiaison )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixProviderLiaison )
PY_END_ATTRIBUTES()

ModelMatrixLiaison::ModelMatrixLiaison()
{
	matrix_.setIdentity();
}

const Matrix & ModelMatrixLiaison::getMatrix() const
{
	return matrix_;
}

bool ModelMatrixLiaison::setMatrix( const Matrix & m )
{
	matrix_ = m;

	return true;
}

BW_END_NAMESPACE

// py_attachment.cpp
