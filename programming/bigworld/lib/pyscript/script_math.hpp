/**
 * 	@file
 *
 * 	This file defines the PyVector class.
 *
 * 	@ingroup script
 */

#ifndef SCRIPT_MATH_HPP
#define SCRIPT_MATH_HPP


#include "pyobject_plus.hpp"
#include "script.hpp"
#include "math/matrix.hpp"
#include "math/planeeq.hpp"


BW_BEGIN_NAMESPACE

/*~ module Math
 *  @components{ all }
 */


// Reference this variable to link in the Math classes.
extern int Math_token;

/* These macros allow a VALID_ANGLE_ARG to appear in a PY_AUTO declaration.
 * They are defined here rather than globally in script.hpp because they use
 * the definition of "valid angle" from isValidAngle, which is statically
 * declared in src/lib/math/matrix.cpp, and so we don't want to leak that
 * definition to the rest of the world.
 *
 * See script.hpp for how the rest of the PY_AUTO stuff works.
 */


/// macros for arg count up to optional args
#define PYAUTO_OPTARGC_PARSE_VALID_ANGLE_ARG(T,R) (PYAUTO_OPTARGC_DO,ARG,U,R)
#define PYAUTO_OPTARGC_PARSE_OPTVALID_ANGLE_ARG(T,DEF,R) (PYAUTO_OPTARGC_DO,END,U,U)

#define PYAUTO_OPTARGC_PARSE_VALID_ANGLE_VECTOR3_ARG(T,R) (PYAUTO_OPTARGC_DO,ARG,U,R)
#define PYAUTO_OPTARGC_PARSE_OPTVALID_ANGLE_VECTOR3_ARG(T,DEF,R) (PYAUTO_OPTARGC_DO,END,U,U)

/// macros for arg count of all args
#define PYAUTO_ALLARGC_PARSE_VALID_ANGLE_ARG(T,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_OPTVALID_ANGLE_ARG(T,DEF,R) (PYAUTO_ALLARGC_DO,ARG,U,R)

#define PYAUTO_ALLARGC_PARSE_VALID_ANGLE_VECTOR3_ARG(T,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_OPTVALID_ANGLE_VECTOR3_ARG(T,DEF,R) (PYAUTO_ALLARGC_DO,ARG,U,R)

/// macros for the types of arguments
#define PYAUTO_ARGTYPES_PARSE_VALID_ANGLE_ARG(T,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_OPTVALID_ANGLE_ARG(T,DEF,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)

#define PYAUTO_ARGTYPES_PARSE_VALID_ANGLE_VECTOR3_ARG(T,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_OPTVALID_ANGLE_VECTOR3_ARG(T,DEF,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)

/// macros for writing the actual argument parsing code
#define PYAUTO_WRITE_PARSE_VALID_ANGLE_ARG(T,R) (PYAUTO_WRITE_DO,VALID_ANGLE_ARG,T,R)
#define PYAUTO_WRITE_PARSE_OPTVALID_ANGLE_ARG(T,DEF,R) (PYAUTO_WRITE_DO,OVALID_ANGLE_ARG,(T,DEF),R)

#define PYAUTO_WRITE_PARSE_VALID_ANGLE_VECTOR3_ARG(T,R) (PYAUTO_WRITE_DO,VALID_ANGLE_VECTOR3_ARG,T,R)
#define PYAUTO_WRITE_PARSE_OPTVALID_ANGLE_VECTOR3_ARG(T,DEF,R) (PYAUTO_WRITE_DO,OVALID_ANGLE_VECTOR3_ARG,(T,DEF),R)

// This definition is stolen from math/matrix.cpp
#define PYAUTO_WRITE_VALID_ANGLE(N,T)								\
	if (arg##N < -100.f || arg##N > 100.f)							\
	{																\
		PyErr_Format( PyExc_TypeError,								\
			"() argument " #N " must be a valid angle" );			\
		return NULL;												\
	}																\

#define PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,T,ELEMENT)				\
	if (arg##N[ELEMENT] < -100.f || arg##N[ELEMENT] > 100.f)		\
	{																\
		PyErr_Format( PyExc_TypeError,								\
			"() argument " #N " element " #ELEMENT " must be a "	\
				"valid angle" );									\
		return NULL;												\
	}																\

#define PYAUTO_WRITE_DO_VALID_ANGLE_ARG(N,A,R)						\
	PYAUTO_WRITE_STD(N,A)											\
	PYAUTO_WRITE_VALID_ANGLE(N,A)									\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_OVALID_ANGLE_ARG(N,A,R)						\
	PYAUTO_WRITE_OPT(N,PAIR_1 A, PAIR_2 A)							\
	PYAUTO_WRITE_VALID_ANGLE(N,PAIR_1 A)							\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_VALID_ANGLE_VECTOR3_ARG(N,A,R)				\
	PYAUTO_WRITE_STD(N,A)											\
	PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,A,0)							\
	PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,A,1)							\
	PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,A,2)							\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_OVALID_ANGLE_VECTOR3_ARG(N,A,R)				\
	PYAUTO_WRITE_OPT(N,PAIR_1 A, PAIR_2 A)							\
	PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,PAIR_1 A,0)					\
	PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,PAIR_1 A,1)					\
	PYAUTO_WRITE_VALID_ANGLE_VECTOR3(N,PAIR_1 A,2)					\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

/// macros for calling the function with the parsed arguments
#define PYAUTO_CALLFN_PARSE_VALID_ANGLE_ARG(T,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_OPTVALID_ANGLE_ARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG,U,R)

#define PYAUTO_CALLFN_PARSE_VALID_ANGLE_VECTOR3_ARG(T,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_OPTVALID_ANGLE_VECTOR3_ARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG,U,R)

#define PYAUTO_CALLFN_PARSE1_VALID_ANGLE_ARG(T,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_OPTVALID_ANGLE_ARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG1,U,R)

#define PYAUTO_CALLFN_PARSE1_VALID_ANGLE_VECTOR3_ARG(T,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_OPTVALID_ANGLE_VECTOR3_ARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG1,U,R)


/**
 *	This helper class has virtual functions to provide a matrix. This
 *	assists many classes in getting out matrices from a variety of
 *	different objects.
 */
class MatrixProvider : public PyObjectPlus
{
	Py_Header( MatrixProvider, PyObjectPlus )

public:
	MatrixProvider( bool autoTick, PyTypeObject * pType );
	virtual ~MatrixProvider();

	virtual void tick( float /*dTime*/ ) { }
	virtual void matrix( Matrix & m ) const = 0;

	virtual void getYawPitch( float& yaw, float& pitch) const
	{
		Matrix m;
		this->matrix(m);
		yaw = m.yaw();
		pitch = m.pitch();
	}

private:
	bool autoTick_;
};

typedef SmartPointer<MatrixProvider> MatrixProviderPtr;

PY_SCRIPT_CONVERTERS_DECLARE( MatrixProvider )

/**
 *	This class allows scripts to create and manipulate their own matrices
 */
class PyMatrix : public MatrixProvider, public Matrix
{
	Py_Header( PyMatrix, MatrixProvider )


public:
	PyMatrix( PyTypeObject * pType = &s_type_ ) :
		MatrixProvider( false, pType ), Matrix( Matrix::identity )	{ }
	void set( const Matrix & m )			{ *static_cast<Matrix*>(this) = m; }
	virtual void matrix( Matrix & m ) const	{ m = *this; }

	// Need to overload this so that we can set the Python exception state.
	bool invert()
	{
		if (!this->Matrix::invert())
		{
			PyErr_SetString( PyExc_ValueError, "Matrix could not be inverted" );
			return false;
		}

		return true;
	}

	void set( MatrixProviderPtr mpp )
		{ Matrix m; mpp->matrix( m ); this->set( m ); }
	PY_AUTO_METHOD_DECLARE( RETVOID, set, NZARG( MatrixProviderPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, setZero, END )
	PY_AUTO_METHOD_DECLARE( RETVOID, setIdentity, END )
	PY_AUTO_METHOD_DECLARE( RETVOID, setScale, ARG( Vector3, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, setTranslate, ARG( Vector3, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, setRotateX, VALID_ANGLE_ARG( float, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, setRotateY, VALID_ANGLE_ARG( float, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, setRotateZ, VALID_ANGLE_ARG( float, END ) )
	void setRotateYPR( const Vector3 & ypr )
		{ this->setRotate( ypr[0], ypr[1], ypr[2] ); }
	PY_AUTO_METHOD_DECLARE( RETVOID, setRotateYPR, VALID_ANGLE_VECTOR3_ARG( Vector3, END ) )
	void preMultiply( MatrixProviderPtr mpp )
		{ Matrix m; mpp->matrix( m ); this->Matrix::preMultiply( m ); }
	PY_AUTO_METHOD_DECLARE( RETVOID, preMultiply,
		NZARG( MatrixProviderPtr, END ) )
	void postMultiply( MatrixProviderPtr mpp )
		{ Matrix m; mpp->matrix( m ); this->Matrix::postMultiply( m ); }
	PY_AUTO_METHOD_DECLARE( RETVOID, postMultiply,
		NZARG( MatrixProviderPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETOK, invert, END )
	PY_AUTO_METHOD_DECLARE( RETVOID, lookAt, ARG( Vector3,
		ARG( Vector3, ARG( Vector3, END ) ) ) )

	void setElement( int col, int row, float value) { (*this)[col&3][row&3] = value; }
	PY_AUTO_METHOD_DECLARE( RETVOID, setElement, ARG( uint, ARG( uint, ARG( float, END ) ) ) )

	float get( int col, int row ) const { return (*this)[col&3][row&3]; }
	PY_AUTO_METHOD_DECLARE( RETDATA, get, ARG( uint, ARG( uint, END ) ) )
	PY_RO_ATTRIBUTE_DECLARE( getDeterminant(), determinant )

	PY_AUTO_METHOD_DECLARE( RETDATA, applyPoint, ARG( Vector3, END ) )
	Vector4 applyV4Point( const Vector4 & p )
		{ Vector4 ret; this->applyPoint( ret, p ); return ret; }
	PY_AUTO_METHOD_DECLARE( RETDATA, applyV4Point, ARG( Vector4, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, applyVector, ARG( Vector3, END ) )

	const Vector3 & applyToAxis( uint axis )
		{ return this->applyToUnitAxisVector( axis & 3 ); }
	PY_AUTO_METHOD_DECLARE( RETDATA, applyToAxis, ARG( uint, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, applyToOrigin, END )

	PY_RO_ATTRIBUTE_DECLARE( isMirrored(), isMirrored )

	PY_AUTO_METHOD_DECLARE( RETVOID, orthogonalProjection,
		ARG( float, ARG( float, ARG( float, ARG( float, END ) ) ) ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, perspectiveProjection,
		ARG( float, ARG( float, ARG( float, ARG( float, END ) ) ) ) )

	const Vector3 & translation() const
		{ return this->Matrix::applyToOrigin(); }
	void translation( const Vector3 & v )
		{ this->Matrix::translation( v ); }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, translation, translation )

	PY_RO_ATTRIBUTE_DECLARE( yaw(), yaw )
	PY_RO_ATTRIBUTE_DECLARE( pitch(), pitch )
	PY_RO_ATTRIBUTE_DECLARE( roll(), roll )

	static PyObject * _pyCall( PyObject * self, PyObject * args, PyObject * kw)
		{ return _py_get( self, args, kw ); }

	PY_METHOD_DECLARE( py___getstate__ )
	PY_METHOD_DECLARE( py___setstate__ )

	PY_FACTORY_DECLARE()
};


/**
 *	This class implements a Vector class for use in Python
 *
 * 	@ingroup script
 */
template <class V> class PyVector : public PyObjectPlus
{
	Py_Header( PyVector< V >, PyObjectPlus )

public:
	typedef V Member;

	PyVector( bool isReadOnly, PyTypeObject * pType ) :
		PyObjectPlus( pType ), isReadOnly_( isReadOnly ) {}
	virtual ~PyVector() { };

	virtual V getVector() const = 0;
	virtual bool setVector( const V & v ) = 0;

	bool isReadOnly() const		{ return isReadOnly_; }
	virtual bool isReference() const	{ return false; }

	PyObject * pyGet_x();
	int pySet_x( PyObject * value );

	PyObject * pyGet_y();
	int pySet_y( PyObject * value );

	PyObject * pyGet_z();
	int pySet_z( PyObject * value );

	PyObject * pyGet_w();
	int pySet_w( PyObject * value );

// 	PY_FACTORY_DECLARE()
	PyObject * pyNew( PyObject * args );

	// PyObjectPlus overrides
	ScriptObject		pyGetAttribute( const ScriptString & attrObj );

	PY_UNARY_FUNC_METHOD( pyStr )

	// Used for as_number
	// PY_BINARY_FUNC_METHOD( py_add )
	// PY_BINARY_FUNC_METHOD( py_subtract )
	// PY_BINARY_FUNC_METHOD( py_multiply )
	static PyObject * _py_add( PyObject * a, PyObject * b );
	static PyObject * _py_subtract( PyObject * a, PyObject * b );
	static PyObject * _py_multiply( PyObject * a, PyObject * b );
	static PyObject * _py_divide( PyObject * a, PyObject * b );

	PY_UNARY_FUNC_METHOD( py_negative )
	PY_UNARY_FUNC_METHOD( py_positive )

	PY_INQUIRY_METHOD( py_nonzero )

	PY_BINARY_FUNC_METHOD( py_inplace_add )
	PY_BINARY_FUNC_METHOD( py_inplace_subtract )
	PY_BINARY_FUNC_METHOD( py_inplace_multiply )
	PY_BINARY_FUNC_METHOD( py_inplace_divide )

	// Used for as_sequence
	PY_SIZE_INQUIRY_METHOD( py_sq_length )
	PY_INTARG_FUNC_METHOD( py_sq_item )
	PY_INTINTARG_FUNC_METHOD( py_sq_slice )
	PY_INTOBJARG_PROC_METHOD( py_sq_ass_item )

	// Script methods
	PY_METHOD_DECLARE( py_set )
	PY_METHOD_DECLARE( py_scale )
	PY_METHOD_DECLARE( py_dot )
	PY_METHOD_DECLARE( py_normalise )

	PY_METHOD_DECLARE( py_tuple )
	PY_METHOD_DECLARE( py_list )

	PY_METHOD_DECLARE( py_cross2D )

	PY_METHOD_DECLARE( py_distTo )
	PY_METHOD_DECLARE( py_distSqrTo )

	PY_METHOD_DECLARE( py_flatDistTo )
	PY_METHOD_DECLARE( py_flatDistSqrTo )
	PY_METHOD_DECLARE( py_setPitchYaw )

	// PY_METHOD_DECLARE( py___reduce__ )
	PY_METHOD_DECLARE( py___getstate__ )
	PY_METHOD_DECLARE( py___setstate__ )

	PY_RO_ATTRIBUTE_DECLARE( this->getVector().length(), length )
	PY_RO_ATTRIBUTE_DECLARE( this->getVector().lengthSquared(), lengthSquared )

	PY_RO_ATTRIBUTE_DECLARE( this->isReadOnly(), isReadOnly )
	PY_RO_ATTRIBUTE_DECLARE( this->isReference(), isReference )

	PyObject * pyGet_yaw();
	PyObject * pyGet_pitch();
	PY_RO_ATTRIBUTE_SET( yaw )
	PY_RO_ATTRIBUTE_SET( pitch )

	static PyObject * _pyNew( PyTypeObject * pType,
			PyObject * args, PyObject * kwargs );

private:
	PyVector( const PyVector & toCopy );
	PyVector & operator=( const PyVector & toCopy );

	bool safeSetVector( const V & v );

	bool isReadOnly_;

	static const int NUMELTS;
};


/**
 *	This class implements a PyVector that has the vector attribute as a member.
 */
template <class V>
class PyVectorCopy : public PyVector< V >
{
public:
	PyVectorCopy( bool isReadOnly = false,
			PyTypeObject * pType = &PyVector<V>::s_type_ ) :
		PyVector<V>( isReadOnly, pType ), v_( V::zero() ) {}

	PyVectorCopy( const V & v, bool isReadOnly = false,
			PyTypeObject * pType = &PyVector<V>::s_type_ ) :
		PyVector<V>( isReadOnly, pType ), v_( v ) {}

	virtual V getVector() const
	{
		return v_;
	}

	virtual bool setVector( const V & v )
	{
		v_ = v;
		return true;
	}

private:
	V v_;
};


/**
 *	This class implements a PyVector that takes its value from a member of
 *	another Python object. This is useful for exposing a Vector2, Vector3 or
 *	Vector4 member C++ class and allowing the underlying member to be modified
 *	when the Python Vector is modified.
 */
template <class V>
class PyVectorRef : public PyVector< V >
{
public:
	PyVectorRef( PyObject * pOwner, V * pV, bool isReadOnly = false,
			PyTypeObject * pType = &PyVector<V>::s_type_ ) :
		PyVector<V>( isReadOnly, pType ), pOwner_( pOwner ), pV_( pV ) {}

	virtual V getVector() const
	{
		return *pV_;
	}

	virtual bool setVector( const V & v )
	{
		*pV_ = v;

		return true;
	}

	virtual bool isReference() const	{ return true; }

private:
	PyObjectPtr pOwner_;
	V * pV_;
};


/**
 *	This class extends PyVector4 is add colour attributes that are aliases for
 *	the PyVector4 attributes.
 */
class PyColour : public PyVector< Vector4 >
{
	Py_Header( PyColour, PyVector< Vector4 > )

public:
	PyColour( bool isReadOnly = false, PyTypeObject * pType = &s_type_ ) :
		PyVector< Vector4 >( isReadOnly, pType ) {}
};


//-----------------------------------------------------------------------------
// Section: Vector4Provider
//-----------------------------------------------------------------------------

/**
 *	This abstract class provides a Vector4
 */
class Vector4Provider : public PyObjectPlusWithWeakReference
{
	Py_Header( Vector4Provider, PyObjectPlusWithWeakReference )
public:
	Vector4Provider( bool autoTick, PyTypeObject * pType );
	virtual ~Vector4Provider();

	PyObject * pyGet_value();
	PY_RO_ATTRIBUTE_SET( value );

	virtual void tick( float /*dTime*/ ) { }
	virtual void output( Vector4 & val ) = 0;

	static PyObjectPtr coerce( PyObject * pObject );

private:
	bool autoTick_;
};

typedef SmartPointer<Vector4Provider> Vector4ProviderPtr;
typedef std::pair<float,Vector4ProviderPtr> Vector4Animation_Keyframe;
namespace Script
{
	int setData( PyObject * pObject, Vector4Animation_Keyframe & rpVal,
		const char * varName = "" );
	PyObject * getData( const Vector4Animation_Keyframe & pVal );
}

PY_SCRIPT_CONVERTERS_DECLARE( Vector4Provider )


/**
 *	This is the basic concrete Vector4 provider.
 */
class Vector4Basic : public Vector4Provider
{
	Py_Header( Vector4Basic, Vector4Provider )
public:
	Vector4Basic( const Vector4 & val = Vector4(0,0,0,0), PyTypeObject * pType = &s_type_ );

	PY_RW_ATTRIBUTE_DECLARE( value_, value )

	virtual void output( Vector4 & val ) { val = value_; }
	void set( const Vector4 & val )		{ value_ = val; }
	Vector4* pValue()					{ return &value_; }
private:
	Vector4 value_;
};

typedef SmartPointer<Vector4Basic> Vector4BasicPtr;
PY_SCRIPT_CONVERTERS_DECLARE( Vector4Basic )


/**
 *	This class allows scripts to create and manipulate a plane
 */
class PyPlane : public PyObjectPlus, PlaneEq
{
	Py_Header( PyPlane, PyObjectPlus)

public:
	PyPlane( PyTypeObject * pType = &s_type_ ) :
		PyObjectPlus( pType )	{ }

	PY_AUTO_METHOD_DECLARE( RETVOID, init, ARG(Vector3, ARG(Vector3, ARG(Vector3, END ))) )

	PY_AUTO_METHOD_DECLARE( RETDATA, intersectRay, ARG( Vector3, ARG(Vector3, END )) )

	PY_FACTORY_DECLARE()
};


/**
 *	This class holds onto all Vector4/Matrix Providers
 *	and ticks them.
 */
class ProviderStore
{
public:
	static void tick( float dTime );
	static void add( MatrixProvider* );
	static void add( Vector4Provider* );
	static void del( MatrixProvider* );
	static void del( Vector4Provider* );
private:
	typedef BW::vector< Vector4Provider*> Vector4Providers;
	static Vector4Providers v4s_;
	typedef BW::vector< MatrixProvider* > MatrixProviders;
	static MatrixProviders ms_;
};

BW_END_NAMESPACE

#endif // SCRIPT_MATH_HPP
