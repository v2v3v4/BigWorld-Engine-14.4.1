#include "pch.hpp"
#include "pymodelnode.hpp"

#include "model/model.hpp"
#include "model/super_model.hpp"
#include "moo/render_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyModelNode
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyModelNode )

PY_BEGIN_METHODS( PyModelNode )
	PY_METHOD( attach )
	PY_METHOD( detach )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyModelNode )
	/*~ attribute PyModelNode.inWorld
	 *
	 *	This attribute is used to determine whether the parent model has been
	 *	added to the world.  It is read only, and is set to 1 if the Model is
	 *	in the world, 0 otherwise.  In order to add a model to the world, you
	 *	can attach it to another model, or set the model property of an entity
	 *	to this Model.
	 *
	 *	@type	Read-Only Integer
	 */
	PY_ATTRIBUTE( inWorld )
	/*~ attribute PyModelNode.attachments
	 *
	 *	This attribute is the list of all PyAttachments which are attached to
	 *	this node.  PyAttachments can be added and removed from the list using
	 *	the attach and detach functions.
	 *
	 *	@type	List of PyAttachments
	 */
	PY_ATTRIBUTE( attachments )
	/*~ attribute PyModelNode.name
	 *
	 *	This attribute is the name of the node
	 *
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( name )
	/*~ attribute PyModelNode.local
	 *
	 *	This attribute is the local transform for the node.  The local transform
	 *	is pre-multiplied to the node matrix and affects every attached object.
	 *
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( local )
PY_END_ATTRIBUTES()


#if ENABLE_TRANSFORM_VALIDATION
/*static*/ bool PyModelNode::s_disableFrameBehindWarning_ = false;


/**
 *	Check if frame behind warnings are disabled.
 */
/*static*/ bool PyModelNode::disableFrameBehindWarning()
{
	BW_GUARD;
	return PyModelNode::s_disableFrameBehindWarning_;
}


/**
 *	Set frame behind warnings enabled/disabled.
 */
/*static*/ void PyModelNode::disableFrameBehindWarning( bool value )
{
	BW_GUARD;
	PyModelNode::s_disableFrameBehindWarning_ = value;
}
#endif // ENABLE_TRANSFORM_VALIDATION


/**
 *	Constructor
 */
PyModelNode::PyModelNode( PyModel * pOwner,
		Moo::NodePtr& pNode,
		MatrixProviderPtr l,
		PyTypeObject * pType ):
	MatrixProviderLiaison( false, pType ),
	localTransform_( l ),
	pOwner_( pOwner ),
	pNode_( pNode ),
	attachmentsHolder_( attachments_, this, true ),
	pHardpointAttachment_( NULL ),
	lodded_( true ),
	lastWorldTransform_( Matrix::identity )
#if ENABLE_TRANSFORM_VALIDATION
	, frameTimestamp_( 0 )
	, lastLatchTimestamp_( 0 )
#endif // ENABLE_TRANSFORM_VALIDATION
{
}

/**
 *	Destructor
 */
PyModelNode::~PyModelNode()
{
	BW_GUARD;
	MF_ASSERT( pOwner_ == NULL )
}


/**
 *	Tick method
 */
void PyModelNode::tick( float dTime )
{
	BW_GUARD;
	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->tick( dTime );
	}
}


/**
 *	Check if this node needs its animations updated this frame.
 *	Checks if attachments need their animations updated this frame.
 *	@return true if this node needs its animations updated,
 *		false if it does not.
 */
bool PyModelNode::needsUpdate() const
{
	BW_GUARD;

	// Do not check if lodded here
	// Always try to call update to re-calculate if we are lodded

#if ENABLE_TRANSFORM_VALIDATION
	if (frameTimestamp_ != Moo::rc().frameTimestamp())
	{
		return true;
	}
#endif // ENABLE_TRANSFORM_VALIDATION

	for (PyAttachments::const_iterator it = attachments_.begin();
		it != attachments_.end();
		++it)
	{
		if ((*it)->needsUpdate())
		{
			return true;
		}
	}

	return false;
}


/**
 *	Check if this if visible or lodded out.
 *	@return true if this object is visible, false if lodded out.
 */
bool PyModelNode::isLodVisible() const
{
	BW_GUARD;

	// Check this node
	if (lodded_)
	{
		return false;
	}

	// Check attachments
	for (PyAttachments::const_iterator it = attachments_.begin();
		it != attachments_.end();
		++it)
	{
		if ((*it)->isLodVisible())
		{
			return true;
		}
	}

	return false;
}


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	Check if this node's animations have been updated this frame.
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this node needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool PyModelNode::isTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;

	// Do not check if lodded here
	// Always try to call update to re-calculate if we are lodded

	if (frameTimestamp_ != frameTimestamp)
	{
		return true;
	}

	for (PyAttachments::const_iterator it = attachments_.begin();
		it != attachments_.end();
		++it)
	{
		if ((*it)->isTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
	}

	return false;
}


/**
 *	Check if this node's animations have been updated this frame.
 *
 *	Use this check when checking for if we need to update.
 *	ie. want to update lods.
 *	
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this node needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool PyModelNode::isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;

	if (this->isLodded())
	{
		return false;
	}

	if (frameTimestamp_ != frameTimestamp)
	{
		return true;
	}

	for (PyAttachments::const_iterator it = attachments_.begin();
		it != attachments_.end();
		++it)
	{
		if ((*it)->isVisibleTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
	}

	return false;
}
#endif // ENABLE_TRANSFORM_VALIDATION


/**
 *	Re-calculate LOD and apply animations to this node's attachments for this
 *	frame.
 *	@param lod the lod to use.
 */
void PyModelNode::updateAnimations( float lod )
{
	BW_GUARD;

	if (!this->needsUpdate())
	{
		// Make sure this isn't lodded out or it will never lod back in
		MF_ASSERT( !this->isLodded() );
		return;
	}

	TRANSFORM_ASSERT( this->isTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );

	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		++it)
	{
		(*it)->updateAnimations( lastWorldTransform_, lod );
	}

#if ENABLE_TRANSFORM_VALIDATION
	frameTimestamp_ = Moo::rc().frameTimestamp();
#endif // ENABLE_TRANSFORM_VALIDATION
	TRANSFORM_ASSERT( !this->isVisibleTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );
}


/**
 *	Draw method
 */
void PyModelNode::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	if (lodded_) return;

	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->draw( drawContext, lastWorldTransform_ );
	}
}

/**
 *	Our model has entered the world, so tell all attachments
 */
void PyModelNode::enterWorld()
{
	BW_GUARD;
	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->enterWorld();
	}
}

/**
 *	Our model has left the world, so tell all attachments
 */
void PyModelNode::leaveWorld()
{
	BW_GUARD;
	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->leaveWorld();
	}
}


/**
 *	Our model has been moved, so tell all attachments
 */
void PyModelNode::move( float dTime )
{
	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->move( dTime );
	}
}


/**
 *	We want to add up everyone's bounding boxes
 */
void PyModelNode::worldBoundingBox( BoundingBox & bb, const Matrix& world )
{
	BW_GUARD;
	if (attachments_.empty()) return;

	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->worldBoundingBox( bb, lastWorldTransform_ );
	}
}

/**
 *	Our model has been tossed, so tell all attachments
 */
void PyModelNode::tossed( bool outside )
{
	BW_GUARD;
	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->tossed( outside );
	}
}


/**
 *	Save the current value of our node's world transform.
 *	We need to save it as Node data is shared by the NodeCatalogue.
 */
void PyModelNode::latch( const Matrix& world )
{
	BW_GUARD;
	if (pNode_->blend( Model::blendCookie() ) <= 0.f)
	{
		lodded_ = true;
		lastWorldTransform_ = world;
#if ENABLE_TRANSFORM_VALIDATION
		lastLatchTimestamp_ = Moo::rc().frameTimestamp();
#endif // ENABLE_TRANSFORM_VALIDATION
	}
	else
	{
		lodded_ = false;
		lastWorldTransform_ = pNode_->worldTransform();
#if ENABLE_TRANSFORM_VALIDATION
		lastLatchTimestamp_ = Moo::rc().frameTimestamp();
#endif // ENABLE_TRANSFORM_VALIDATION
	}
	if (localTransform_.exists())
	{
		Matrix l;
		localTransform_->matrix(l);
		lastWorldTransform_.preMultiply(l);
	}
}

/**
 *	Detach since our model has been destroyed
 */
void PyModelNode::detach()
{
	BW_GUARD;
	for (PyAttachments::iterator it = attachments_.begin();
		it != attachments_.end();
		it++)
	{
		(*it)->detach();
	}

	pOwner_ = NULL;
}



/**
 *	This allows scripts to get the attachments attached to this node,
 *	interpreted as if it were a hardpoint.
 */
PyObject * PyModelNode::pyGetHardPoint()
{
	BW_GUARD;
	for (uint i = 0; i < attachments_.size(); i++)
	{
		PyAttachment * pAttachment = attachments_[i].get();

		// Note: The pHardpointAttachment_ pointer will be (knowingly) left
		// dangling if the attachment is removed manually from the attachments
		// list, i.e. if pySetHardPoint was not used. So the cached value here
		// can never be accessed, only compared. And yes, it is possible to
		// remove the hardpoint attachment, reattach it in a different way
		// (or destroy it and a new object gets the same pointer) but those
		// are the (user-imposed) hazards of mixing node attachment interfaces.
		if (pAttachment == pHardpointAttachment_)
		{
			Py_INCREF( pAttachment );
			return pAttachment;
		}
	}

	Py_RETURN_NONE;
}


/**
 *	This allows scripts to set the attachments attached to this node,
 *	interpreted as if it were a hardpoint.
 */
int PyModelNode::pySetHardPoint( PyObject * value )
{
	BW_GUARD;
	// ok, now make sure that value is an attachment
	if (value == NULL || (!PyAttachment::Check( value ) && value != Py_None) )
	{
		PyErr_Format( PyExc_TypeError, "Model.%s "
			"must be set to an Attachment", pNode_->identifier().c_str() );
		return -1;
	}

	// if we are attaching a model that's not blank,
	//  make sure it has a corresponding hard point
	PyModel * pSubModel = NULL;
	if (PyModel::Check( value ) && ((PyModel*)value)->pSuperModel() != NULL)
	{
		pSubModel = ((PyModel*)value);

		if (!pSubModel->pSuperModel()->findNode( pNode_->identifier(),
			NULL, true ))
		{
			PyErr_Format( PyExc_ValueError, "Model.%s can only be set to a "
				"non-blank Model if it has a corresponding hardpoint",
				pNode_->identifier().c_str() );
			return -1;
		}
	}

	// ok then, erase the existing hardpoint, if we still have it
	for (uint i = 0; i < attachments_.size(); i++)
	{
		if (attachments_[i].get() == pHardpointAttachment_)
		{
			// unless we're attaching the same one!
			if ((PyAttachment*)value == pHardpointAttachment_)
				return 0;	// 'value' OK to be Py_None above

			attachmentsHolder_.erase( i, 1 );
			// cannot have duplicates in list (would be 'already attached')
			break;
		}
	}
	pHardpointAttachment_ = NULL;

	// and if there's a new one
	if (value != Py_None)
	{
		PyAttachment * pAttachment = (PyAttachment*)value;
		pHardpointAttachment_ = pAttachment;

		// insert the new one
		attachmentsHolder_.insertRange( 0, 1 );
		if (attachmentsHolder_.insert( pAttachment ) != 0)
		{
			// get out now if it failed
			attachmentsHolder_.cancel();
			return -1;
		}

		// reroute the root to be the corresponding hardpoint (if applicable)
		if (pSubModel != NULL)
		{
			if (!pSubModel->origin( pNode_->identifier() ))
			{
				WARNING_MSG( "PyModelNode::pySetHardPoint: "
					"origin() unexpectedly failed. Safe, but bad.\n" );
				// get out now if it failed (it shouldn't - we already checked)
				attachmentsHolder_.cancel();
				return -1;
			}
		}
	}

	// and finally commit to those changes!
	attachmentsHolder_.commit();

	return 0;
}


/**
 *	MatrixProvider matrix method.
 *	Accesses the matrix that was saved using PyModelNode::latch(),
 *	during animation update.
 */
void PyModelNode::matrix( Matrix & m ) const
{
	BW_GUARD;

	m = lastWorldTransform_;

	// Use the SCOPED_DISABLE_FRAME_BEHIND_WARNING macro to disable this warning
#if ENABLE_TRANSFORM_VALIDATION
	if (!PyModelNode::disableFrameBehindWarning() &&
		Moo::rc().frameTimestamp() != lastLatchTimestamp_)
	{
		WARNING_MSG( "PyModelNode::matrix: "
			"node's position has been accessed %d frames behind\n",
			(Moo::rc().frameTimestamp() - lastLatchTimestamp_) );
	}
#endif // ENABLE_TRANSFORM_VALIDATION
}


/**
 *	MatrixLiaison matrix method.
 *	Accesses the matrix that was saved using PyModelNode::latch(),
 *	during animation update.
 */
const Matrix & PyModelNode::getMatrix() const
{
	BW_GUARD;

	// Use the SCOPED_DISABLE_FRAME_BEHIND_WARNING macro to disable this warning
#if ENABLE_TRANSFORM_VALIDATION
	if (!PyModelNode::disableFrameBehindWarning() &&
		Moo::rc().frameTimestamp() != lastLatchTimestamp_)
	{
		WARNING_MSG( "PyModelNode::getMatrix: "
			"node's position has been accessed %d frames behind\n",
			(Moo::rc().frameTimestamp() - lastLatchTimestamp_) );
	}
#endif // ENABLE_TRANSFORM_VALIDATION

	return lastWorldTransform_;
}


/**
 *	MatrixLiaison matrix method.
 *	Does nothing.
 */
bool PyModelNode::setMatrix( const Matrix & m )
{
	return true;
}


/*~ function PyModelNode.attach
 *
 *	This function takes a PyAttachment and attaches it to the node.  The
 *  attachment must not already be attached to something. Attaching it causes
 *  the attachment to appear with its root node with the same position and
 *	rotation as this node. It also sets the attachments inWorld attribute to
 *  the inWorld Attribute of this PyModelNode.
 *
 *	Examples of things which can be attached are models and particle systems.
 *
 *	@param	attachment	the PyAttachment to be attached to this node.
 *
 */
/**
 *	Attach called from python
 */
bool PyModelNode::attach( PyAttachmentPtr pAttachment )
{
	BW_GUARD;
	attachmentsHolder_.insertRange( static_cast<int>(attachments_.size()), 1 );
	if (attachmentsHolder_.insert( pAttachment.get() ) != 0)
	{
		// PySTLSequenceHolder insert will raise an error via Script::setData
		attachmentsHolder_.cancel();
		return false;
	}

	attachmentsHolder_.commit();
	return true;
}

/*~ function PyModelNode.detach
 *
 *	This function takes a PyAttachment, and tries to detach it from this node.
 *	This will fail and error if the attachment wasn't attached to this node
 *  when the call was made.
 *
 *  Raises a ValueError if the given attachment is not attached to this node.
 *
 *	@param	attachment	the PyAttachment to be detached from this node.
 */
/**
 *	Detach called from python
 */
bool PyModelNode::detach( PyAttachmentPtr pAttachment )
{
	BW_GUARD;
	PyAttachments::iterator found = std::find(
		attachments_.begin(), attachments_.end(), pAttachment );
	if (found != attachments_.end())
	{
		int index = static_cast<int>(found-attachments_.begin());
		attachmentsHolder_.erase( index, index+1 );
		attachmentsHolder_.commit();
		return true;
	}

	PyErr_SetString( PyExc_ValueError, "PyModelNode.detach(): "
		"The given Attachment is not attached here." );
	return false;
}

BW_END_NAMESPACE

// pymodelnode.cpp
