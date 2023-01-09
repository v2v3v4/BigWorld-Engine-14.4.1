#ifndef BOX_ATTACHMENT_HPP
#define BOX_ATTACHMENT_HPP

#include "py_attachment.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements a PyAttachment with hit-testing
 *	capabilities.  Use this to attach to dynamic skeletal
 *	objects for collision tests.
 */
class BoxAttachment : public PyAttachment
{
	Py_Header( BoxAttachment, PyAttachment )

public:
	BoxAttachment( PyTypeObject * pType = &s_type_ );
	~BoxAttachment();

	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( minBounds_, minBounds )
	PY_RW_ATTRIBUTE_DECLARE( maxBounds_, maxBounds )

	PyObject * pyGet_name();
	int pySet_name( PyObject * pValue );

	PY_RW_ATTRIBUTE_DECLARE( hit_, hit )
	PY_RO_ATTRIBUTE_DECLARE( worldTransform_, worldTransform )

	void name( const BW::string& name );
	const BW::string& name() const;

	virtual void tick( float dTime );

	virtual bool needsUpdate() const { return false; }
	virtual bool isLodVisible() const { return true; }
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod ) { /*do nothing*/ }

	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );
	bool collide( const Vector3& start, const Vector3& direction, float& distance );

	void	save( DataSectionPtr pSection );
	void	load( DataSectionPtr pSection );

private:
	BoxAttachment( const BoxAttachment& );
	BoxAttachment& operator=( const BoxAttachment& );

	Vector3		minBounds_;
	Vector3		maxBounds_;
	Matrix		worldTransform_;
	uint16		nameIndex_;
	bool		hit_;
};

PY_SCRIPT_CONVERTERS_DECLARE( BoxAttachment )

BW_END_NAMESPACE

#endif // BOX_ATTACHMENT_HPP
