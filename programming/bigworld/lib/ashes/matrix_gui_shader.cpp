#include "pch.hpp"
#include "matrix_gui_shader.hpp"

#include "simple_gui_component.hpp"
#include "moo/render_context.hpp"	// for frameTimestamp


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: WorldToClipMP
// -----------------------------------------------------------------------------


/**
 *	This class is mainly for the matrix GUI shader, and provides a clip-space
 *	position given a world-space matrix provider.
 */
class WorldToClipMP : public MatrixProvider
{
	Py_Header( WorldToClipMP, MatrixProvider )

public:
	WorldToClipMP( PyTypeObject * pType = &s_type_ ) : MatrixProvider( false, &s_type_ )
		{ }

	virtual void matrix( Matrix & m ) const;

	PY_RW_ATTRIBUTE_DECLARE( target_, target )

	static WorldToClipMP * New()	{ return new WorldToClipMP(); }
	PY_AUTO_FACTORY_DECLARE( WorldToClipMP, END )

private:
	MatrixProviderPtr	target_;
};

/*~	class GUI.WorldToClipMP
 *	@components{ client, tools }
 *
 *	This utility class is a MatrixProvider that provides a 
 *	clip-space position given a world-space matrix provider.
 *	It is primarily used by the MatrixGUIShader.
 */
PY_TYPEOBJECT( WorldToClipMP )

PY_BEGIN_METHODS( WorldToClipMP )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( WorldToClipMP )

	/*~	attribute WorldToClipMP.target
	 *	@components{ client, tools }
	 *
	 *	@type MatrixProviderPtr	The target MatrixProvider is the matrix that is used 
	 *							to provide the clip-space for this WorldToClipMP.
	 */
	PY_ATTRIBUTE( target )

PY_END_ATTRIBUTES()

/*~	function GUI.WorldToClipMP
 *	@components{ client, tools }
 *
 *	Used primarily with the MatrixGUIShader, this returns a new WorldToClip MatrixProider object.
 */
PY_FACTORY( WorldToClipMP, GUI )


/**
 *	WorldToClipMP matrix method
 */
void WorldToClipMP::matrix( Matrix & m ) const
{
	BW_GUARD;
	if (target_)
	{
		Vector4 clipPos;
		Matrix tgt;
		target_->matrix( tgt );
		Moo::rc().viewProjection().applyPoint( clipPos, tgt.applyToOrigin() );
		if ( !almostZero( clipPos.w, 0.0001f ) )
		{
			float oow = 1.f / fabsf( clipPos.w );
			m.setTranslate( Vector3( clipPos.x * oow, clipPos.y * oow, 0.f ) );
		}
		else
		{
			//position off the screen
			m.setTranslate( Vector3( -100000.f,-100000.f, 0.f ) );
		}

	}
	else
	{
		m = Matrix::identity;
	}

}


// -----------------------------------------------------------------------------
// Section: MatrixGUIShader
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( MatrixGUIShader )

PY_BEGIN_METHODS( MatrixGUIShader )
	PY_METHOD( reset )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixGUIShader )
	/*~ attribute MatrixGUIShader.target
	 *	@components{ client, tools }
	 *
	 *	This attribute specifies the MatrixProvider that is used to transform
	 *	the components that this is attached to.  The transformation is applied
	 *	within clip space ( (-1,-1) is the bottom left of the screen, (1,1) is
	 *	the top right).
	 *
	 *	If blend is 0 (false) then any changes to the target MatrixProvider
	 *	are reflected instantaneously in the transformation of the components.  
	 *
	 *	If blend is non-zero (true), then the the components will move from
	 *	their current transformations to the transform specified by the target
	 *	in eta seconds.
	 *
	 *	It defaults to None.
	 *
	 *	@type		MatrixProvider
	 */
	PY_ATTRIBUTE( target )
	/*~ attribute MatrixGUIShader.eta
	 *	@components{ client, tools }
	 *
	 *	This attribute specifies how many seconds it will take to blend from
	 *	the current transformation of the components to the target transform.
	 *	This is only used if the blend attribute is non-zero.
	 *
	 *	It defaults to 0.0.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( eta )
	/*~ attribute MatrixGUIShader.blend
	 *	@components{ client, tools }
	 *
	 *	This attribute specifies whether to blend from the current transform to
	 *	that specified by the target MatrixProvider.  If this attribute is
	 *	non-zero (true) then blending is performed, over eta seconds.  If this
	 *	attribute is zero (false) then the target transform is just applied
	 *	instantly.
	 *
	 *	It defaults to 1 (true).
	 *
	 *	@type	integer (treated as boolean)
	 */
	PY_ATTRIBUTE( blend )
PY_END_ATTRIBUTES()

/*~ function GUI.MatrixShader
 *	@components{ client, tools }
 *
 *	This function creates a new MatrixGUIShader.  This can be used to transform
 *	the position of one or more GUI Components within clipping space.  
 *
 *	@param	target (optional)	The initial target MatrixProvider.
 *
 *	@return				a new MatrixGUIShader.
 */
PY_FACTORY_NAMED( MatrixGUIShader, "MatrixShader", GUI )

SHADER_FACTORY( MatrixGUIShader )


/**
 *	Constructors.
 */
MatrixGUIShader::MatrixGUIShader( PyTypeObject * pType ) :
	GUIShader( pType ),
	current_( Matrix::identity ),
	target_( NULL ),
	eta_( 0.f ),
	blend_( true ),
	lastUpdated_( -1 )
{
	BW_GUARD;
}


MatrixGUIShader::MatrixGUIShader( MatrixProviderPtr pTrn, PyTypeObject * pType ):
	GUIShader( pType ),
	current_( Matrix::identity ),
	target_( pTrn ),
	eta_( 0.f ),
	blend_( false ),
	lastUpdated_( -1 )
{
	BW_GUARD;	
}

/**
 *	Destructor.
 */
MatrixGUIShader::~MatrixGUIShader()
{
	BW_GUARD;	
}

/**
 *	Python factory method
 */
PyObject * MatrixGUIShader::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "GUI.MatrixShader()"
			" takes no arguments" );
		return NULL;
	}

	return new MatrixGUIShader();
}


/**
 *	Apply ourselves to the given component
 */
bool MatrixGUIShader::processComponent( SimpleGUIComponent& component,
	float dTime )
{
	BW_GUARD;
	Matrix transformer;

	// change the matrix if desired
	// (and if we haven't yet done so this frame)
	uint32 frameNow = Moo::rc().frameTimestamp();
	if (target_)
	{
		if ( blend_ )
		{		
			if ( lastUpdated_ != frameNow )
			{
				Matrix tgtVal;
				target_->matrix( tgtVal );

				BlendTransform tgtBlend( tgtVal );

				if (eta_ > 0.f)
				{
					current_.blend( dTime / eta_, tgtBlend );
					eta_ = max( eta_ - dTime, 0.f );
				}
				else
				{
					current_ = tgtBlend;
				}
			}

			lastUpdated_ = frameNow;

			// set up the matrix to apply
			current_.output( transformer );
		}
		else
		{
			target_->matrix( transformer );
		}

		// and apply it to the runTimeTransform.
		Matrix m( component.runTimeTransform() );
		m.preMultiply( transformer );
		component.runTimeTransform( m );
	}

	// easy!
	return false;	//don't need to update children - runTimeTransform already persists
}


/*~ function MatrixGUIShader.reset
 *	@components{ client, tools }
 *
 *	This method sets the current matrix to the target matrix, immediately.
 */
/**
 *	This method sets the current matrix to the target matrix, immediately.
 */
void MatrixGUIShader::reset()
{
	BW_GUARD;
	eta_ = 0.f;
	Matrix tgtVal;
	target_->matrix( tgtVal );
	BlendTransform tgtBlend( tgtVal );
	current_ = tgtBlend;
	lastUpdated_ = Moo::rc().frameTimestamp();
}


/**
 *	Load method
 */
bool MatrixGUIShader::load( DataSectionPtr pSect )
{
	BW_GUARD;
	if (!GUIShader::load( pSect )) return false;

	DataSectionPtr tsect = pSect->openSection( "target" );
	if (!tsect)
	{
		target_ = NULL;
	}
	else
	{
		Matrix tgtVal = Matrix::identity;
		if (target_) target_->matrix( tgtVal );
		tgtVal = tsect->asMatrix34( tgtVal );

		PyMatrix * pyM = new PyMatrix();
		pyM->set( tgtVal );
		target_ = pyM;
		Py_DECREF( pyM );
	}

	eta_ = pSect->readFloat( "eta", eta_ );
	blend_ = pSect->readBool( "blend", blend_ );

	return true;
}

/**
 *	Save method
 */
void MatrixGUIShader::save( DataSectionPtr pSect )
{
	BW_GUARD;
	GUIShader::save( pSect );

	if (target_)
	{
		Matrix tgtVal;
		target_->matrix( tgtVal );
		pSect->writeMatrix34( "target", tgtVal );
	}

	pSect->writeFloat( "eta", eta_ );
	pSect->writeBool( "blend", blend_ );
}

BW_END_NAMESPACE

// matrix_gui_shader.cpp
