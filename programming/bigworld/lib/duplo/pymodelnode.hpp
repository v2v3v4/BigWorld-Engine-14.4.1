#ifndef PYMODELNODE_HPP
#define PYMODELNODE_HPP

#include "py_attachment.hpp"
#include "pymodel.hpp"
#include "pyscript/script_math.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}
/*~ class BigWorld.PyModelNode
 *
 *	A PyModelNode is a MatrixProvider
 *	which is weighted with other nodes to calculate the transformation matrix
 *  for a particular piece of geometry.  In a biped for example, "biped Spine",
 *  "biped Spine1" and "biped Pelvis" are all nodes which are used to transform
 *  the torso geometry.
 *
 *	PyAttachments can be attached to PyModelNodes, which will store them in the
 *  attachments attribute.
 *
 *	PyModelNodes cannot be created dynamically, but are part of a PyModel, which
 *	is loaded from a .model file. They can be accessed using the node function,
 *	which obtains the PyModelNode of the specified name from a model.
 *
 *	A PyModelNode's matrix is only updated after the owning model's nodes are
 *	updated, which is at draw time.  This means that from script the very first
 *	time you create a PyModelNode reference it's internal transform will not
 *	yet be set.  You will have to wait until the owning model updates
 *	at least once before accessing the node's matrix.
 *
 *	For example:
 *	@{
 *	myNode = model.node( "biped Head" )
 *	@}
 *	obtains the head node of the model.
 */
/**
 *	This class is a node in a PyModel.
 *	The other purpose of this class is to save the node's transformation state
 *	for drawing later because Node data is stored in the NodeCatalogue.
 */
class PyModelNode : public MatrixProviderLiaison
{
	Py_Header( PyModelNode, MatrixProviderLiaison )

public:
	PyModelNode( PyModel * pOwner,
		Moo::NodePtr& pNode,
		MatrixProviderPtr local,
		PyTypeObject * pType = &s_type_ );
	~PyModelNode();

	void tick( float dTime );
	void move( float dTime );
	bool needsUpdate() const;
	bool isLodVisible() const;
#if ENABLE_TRANSFORM_VALIDATION
	bool isTransformFrameDirty( uint32 frameTimestamp ) const;
	bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const;
#endif // ENABLE_TRANSFORM_VALIDATION
	void updateAnimations( float lod );
	void draw( Moo::DrawContext& drawContext );

	void enterWorld();
	void leaveWorld();

	void worldBoundingBox( BoundingBox & bb, const Matrix& world );

	void tossed( bool outside );

	void latch( const Matrix& world );
	void detach();

	PyObject * pyGetHardPoint();
	int pySetHardPoint( PyObject * value );

	virtual void matrix( Matrix & m ) const;

	virtual const Matrix & getMatrix() const;
	virtual bool setMatrix( const Matrix & m );

	MatrixProviderPtr localTransform() const	{ return localTransform_; }
	void localTransform(MatrixProviderPtr l)	{ localTransform_ = l; }

	PY_RO_ATTRIBUTE_DECLARE( pOwner_ && pOwner_->isInWorld(), inWorld )
	PY_RW_ATTRIBUTE_DECLARE( attachmentsHolder_, attachments )
	PY_RO_ATTRIBUTE_DECLARE( pNode_->identifier(), name )
	PY_RO_ATTRIBUTE_DECLARE( localTransform_, local )

	bool attach( PyAttachmentPtr pAttachment );
	bool detach( PyAttachmentPtr pAttachment );
	bool hasAttachments() const					{ return !attachments_.empty(); }
	PY_AUTO_METHOD_DECLARE( RETOK, attach, NZARG( PyAttachmentPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETOK, detach, NZARG( PyAttachmentPtr, END ) )

	// Should we have the dictionary-like named attachments here?

	const PyAttachments & attachments() const  { return attachments_; }
	Moo::NodePtr		pNode()				{ return pNode_; }
	bool isLodded() const { return lodded_; }

private:

#if ENABLE_TRANSFORM_VALIDATION
	friend class ScopedDisableFrameBehindWarning;
	static bool disableFrameBehindWarning();
	static void disableFrameBehindWarning( bool value );

	static bool s_disableFrameBehindWarning_;
#endif // ENABLE_TRANSFORM_VALIDATION

	PyModel				* pOwner_;
	Moo::NodePtr		pNode_;

	PyAttachments						attachments_;
	PySTLSequenceHolder<PyAttachments>	attachmentsHolder_;

	PyAttachment		* pHardpointAttachment_;

	bool				lodded_;
	MatrixProviderPtr	localTransform_;
	Matrix				lastWorldTransform_;

#if ENABLE_TRANSFORM_VALIDATION
	uint32 frameTimestamp_;
	uint32 lastLatchTimestamp_;
#endif // ENABLE_TRANSFORM_VALIDATION
};


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	This is a workaround to disable warnings for systems which have always been
 *	accessing matrix positions from the previous frame.
 *
 *	Please use SCOPED_DISABLE_FRAME_BEHIND_WARNING to disable unwanted frame
 *	behind warnings or move the code to after updateAnimations has been called.
 */
class ScopedDisableFrameBehindWarning
{
public:
	ScopedDisableFrameBehindWarning()
	{
		PyModelNode::disableFrameBehindWarning( true );
	}

	~ScopedDisableFrameBehindWarning()
	{
		PyModelNode::disableFrameBehindWarning( false );
	}
};

#define SCOPED_DISABLE_FRAME_BEHIND_WARNING ScopedDisableFrameBehindWarning safb;

#else // ENABLE_TRANSFORM_VALIDATION

#define SCOPED_DISABLE_FRAME_BEHIND_WARNING

#endif // ENABLE_TRANSFORM_VALIDATION

BW_END_NAMESPACE

#endif // PYMODELNODE_HPP
