#ifndef PY_SPLODGE_HPP
#define PY_SPLODGE_HPP


#include "py_attachment.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.PySplodge
 *
 *  A PyAttachment which draws a "splodge" shadow on the ground
 *  below the point where it is attached. The shape of this shadow
 *  moves in relation to the position of the sun, and is only visible
 *  outside. The actual shadow is drawn using the material specified
 *  in resource.xml/environment/splodgeMaterial
 *
 *	A new PySplodge is created using BigWorld.Splodge function.
 */
/**
 *	This class is an attachment that draws a splodge on the ground
 *	below the point where it is attached.
 */
class PySplodge : public PyAttachment
{
	Py_Header( PySplodge, PyAttachment )

public:
	PySplodge( Vector3 bbSize, PyTypeObject * pType = &s_type_ );

	virtual void tick( float dTime ) { }

	virtual bool needsUpdate() const { return true; }
	virtual bool isLodVisible() const;
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod );
	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );

	virtual void tossed( bool outside );

	PY_RW_ATTRIBUTE_REF_DECLARE( bbSize_, size )
	PY_RW_ATTRIBUTE_DECLARE( maxLod_, maxLod )

	static PySplodge * New( Vector3 bbSize );
	PY_AUTO_FACTORY_DECLARE( PySplodge, OPTARG( Vector3, Vector3(0,0,0), END ) )

private:
	Vector3			bbSize_;
	float			lod_;
	float			maxLod_;
	bool			outsideNow_;

public:
	static bool		s_ignoreSplodge_;
	static uint32	s_splodgeIntensity_;
};

BW_END_NAMESPACE


#endif // PY_SPLODGE_HPP
