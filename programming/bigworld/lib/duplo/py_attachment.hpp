#ifndef PY_ATTACHMENT_HPP
#define PY_ATTACHMENT_HPP

#include "math/boundbox.hpp"
#include "math/matrix_liason.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/stl_to_py.hpp"
#include "moo/texture_usage.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
class ModelTextureUsageGroup;
class DrawContext;
}

/*~ class BigWorld.PyAttachment
 *
 *	Any object type which can be attached to something else inherits from
 *  PyAttachment.  Examples of PyAttachment are PyModel, ParticleSystem and
 *  PySplodge.  A PyAttachment can be attached to various things, among them
 *  PyModelNode@@s and Entity@@s.
 *
 *	For example, to attach a model to an entity:
 *	@{
 *	e.model = myModel # model is a subclass of PyAttachment
 *	@}
 *
 *	To attach a particle system to the right hand of a character model:
 *	@{
 *	# myPS is a particle system, which inherits from PyAttachment
 *	character.node( "biped R Hand" ).attach( myPS )
 *	@}
 */
/**
 *	Base class for all pythonised things that can be attached
 *	somewhere in the world.
 */
class PyAttachment : public PyObjectPlusWithWeakReference
{
	Py_Header( PyAttachment, PyObjectPlusWithWeakReference )

public:
	PyAttachment( PyTypeObject * pType = &s_type_ );
	~PyAttachment();

	virtual void tick( float dTime ) = 0;
	virtual bool needsUpdate() const = 0;
	virtual bool isLodVisible() const = 0;
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const = 0;
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const = 0;
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod ) = 0;
	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform ) = 0;

	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false )
	{ bb.addBounds( Vector3::zero() ); }	// local space


	virtual void localVisibilityBox( BoundingBox & vbb, bool skinny = false )
	{ vbb.addBounds( Vector3::zero() ); }	// local space

	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false )
	{};

	virtual void worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny = false )
	{};

	virtual void tossed( bool outside ) { }

	virtual bool attach( MatrixLiaison * pOwnWorld = NULL );
	virtual void detach();

	virtual void enterWorld();
	virtual void leaveWorld();

	virtual void move( float dTime ) { }		// top-level attachment only

	bool isAttached() const		{ return attached_; }
	bool isInWorld() const		{ return inWorld_; }

	bool castsShadow() const { return castsShadow_; }
	MatrixProvider * matrix();

	PY_RO_ATTRIBUTE_DECLARE( attached_, attached )
	PY_RO_ATTRIBUTE_DECLARE( inWorld_, inWorld )
	PY_RW_ATTRIBUTE_DECLARE( castsShadow_, castsShadow )
	PyObject * pyGet_matrix() { return this->matrix(); }
	PY_RO_ATTRIBUTE_SET( matrix )

	const Matrix & worldTransform() const
		{ return pOwnWorld_ ? pOwnWorld_->getMatrix() : Matrix::identity; }
	void worldTransform( const Matrix & worldTransform )
		{ if (pOwnWorld_) pOwnWorld_->setMatrix( worldTransform ); }

	MatrixLiaison* matrixLiaison() { return pOwnWorld_; }
	virtual const class PyModel * getPyModel() const { return NULL; };

	virtual void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup ) const {}
	virtual void textureUsageWorldBox( BoundingBox & bb, const Matrix & world ) const {}

protected:
	MatrixLiaison	* pOwnWorld_;
	bool			attached_;
	bool			inWorld_;
	bool			castsShadow_;
};

typedef ScriptObjectPtr<PyAttachment> PyAttachmentPtr;

PY_SCRIPT_CONVERTERS_DECLARE( PyAttachment )

typedef BW::vector<PyAttachmentPtr> PyAttachments;


/*
 *	Helper stuff for dealing with attachments
 */
namespace PySTLObjectAid
{
	/// Holder is special for our model list
	template <> struct Holder< PyAttachments >
	{
		static bool hold( PyAttachmentPtr & it, PyObject * pOwner );
		static void drop( PyAttachmentPtr & it, PyObject * pOwner );
	};
};


/**
 *	This class is a matrix provider and a liaison.
 *
 *	It only exists so that the attachments holder can determine
 *	whether or not its owner is a MatrixLiaison.
 */
class MatrixProviderLiaison : public MatrixProvider, public MatrixLiaison
{
	Py_Header( MatrixProviderLiaison, MatrixProvider )

public:
	MatrixProviderLiaison( bool autoTick, PyTypeObject * pType = &s_type_ ) :
		MatrixProvider( autoTick, pType ) { }
};

class ModelMatrixLiaison : public MatrixLiaison
{
public:
	ModelMatrixLiaison();

	virtual const Matrix & getMatrix() const;
	virtual bool setMatrix( const Matrix & m );

protected:
	Matrix matrix_;
};

BW_END_NAMESPACE

#endif // PY_ATTACHMENT_HPP
