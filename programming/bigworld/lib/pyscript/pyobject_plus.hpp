#ifndef PYOBJECT_PLUS_HPP
#define PYOBJECT_PLUS_HPP

#include "Python.h"


#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include "compatibility.hpp"
#include "pyobject_base.hpp"
#include "pyobject_pointer.hpp"
#include "py_factory_method_link.hpp"

#include "script/script_object.hpp"


/// Deprecated, use PyTypeObject instead
#define PyTypePlus PyTypeObject


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Macros
// -----------------------------------------------------------------------------

/**
 *	General helper macros
 */

/// This method performs a fast string comparison.
inline bool streq( const char * strA, const char * strB )
{
	// Do the initial character comparision first for speed.
	return *strA == *strB && strcmp( strA, strB ) == 0;
}

/// Deprecated, use Py_RETURN_NONE instead
/// This macro returns the Python Py_None object.
#define Py_Return			{ Py_INCREF( Py_None ); return Py_None; }

/// This macro prints the current Python error if there is one.
#define PY_ERROR_CHECK()													\
{																			\
 	if (PyErr_Occurred())													\
 	{																		\
		ERROR_MSG( "%s(%d): Script Error\n", __FILE__, __LINE__ );			\
		PyErr_Print();														\
	}																		\
}



/**
 *	PyObjectPlus-derived class declaration macros
 */

/// This must be the first line of each PyObjectPlus-derived class.
#define Py_Header( CLASS, SUPER_CLASS )										\
	public:																	\
		static void _tp_dealloc( PyObject * pObj )							\
		{																	\
			static_cast< CLASS * >( pObj )->pyDel();						\
			delete static_cast< CLASS * >( pObj );							\
		}																	\
																			\
		static PyObject * _tp_getattro( PyObject * pObj,					\
				PyObject * name )											\
		{																	\
			return static_cast<CLASS*>(pObj)->pyGetAttribute(				\
				ScriptString( name, 										\
					ScriptObject::FROM_BORROWED_REFERENCE ) ).newRef(); 	\
		}																	\
																			\
		static int _tp_setattro( PyObject * pObj,							\
			PyObject * name,												\
			PyObject * value )												\
		{																	\
			ScriptString attr( name, 										\
				ScriptObject::FROM_BORROWED_REFERENCE );					\
																			\
			ScriptObject newValue( value,									\
				ScriptObject::FROM_BORROWED_REFERENCE );					\
			return (value != NULL) ?										\
				(static_cast<CLASS*>(pObj)->pySetAttribute( attr, newValue )\
				 	? 0 : -1) :												\
				(static_cast<CLASS*>(pObj)->pyDelAttribute( attr )			\
					? 0 : -1);												\
		}																	\
																			\
		static PyObject * _tp_repr( PyObject * pObj )						\
		{																	\
			return static_cast<CLASS *>(pObj)->pyRepr();					\
		}																	\
																			\
	Py_InternalHeader( CLASS, SUPER_CLASS )									\

/// This can be used for classes that do not derive from PyObjectPlus
/// @see PY_FAKE_PYOBJECTPLUS_BASE_DECLARE
#define Py_FakeHeader( CLASS, SUPER_CLASS )									\
	public:																	\
		const char * typeName() const { return #CLASS; }					\
																			\
	Py_InternalHeader( CLASS, SUPER_CLASS )									\

/// This internal macro does common header stuff for real and fake
/// PyObjectPlus classes
#define Py_InternalHeader( CLASS, SUPER_CLASS )								\
	public:																	\
		static PyTypeObject				s_type_;							\
																			\
		static bool Check( PyObject * pObject )								\
		{																	\
			if (pObject)													\
				return PyObject_TypeCheck( pObject, &s_type_ );				\
			return false;													\
		}																	\
		static bool Check( const ScriptObject & pObject )					\
		{																	\
			return CLASS::Check( pObject.get() );							\
		}																	\
																			\
		typedef SUPER_CLASS Super;											\
																			\
																			\
	Py_CommonHeader( CLASS )

/// Deprecated, use Py_Header instead of Py_InstanceHeader
#define Py_InstanceHeader( CLASS ) Py_Header( CLASS, PyObjectPlus )


// This macro is used by both of the macros above
#define Py_CommonHeader( CLASS )											\
	private:																\
		typedef CLASS This;													\
																			\
	public:																	\
		static PyMethodDef * s_getMethodDefs();								\
		static PyGetSetDef * s_getAttributeDefs();							\
																			\
		/* TODO: Replace __members__ and __methods__ with __dir__ */		\
		PyObject * pyGet___members__()										\
		{																	\
			ScriptList pList = ScriptList::create();						\
			this->pyAdditionalMembers( pList );								\
			return pList.newRef();											\
		}																	\
		PY_RO_ATTRIBUTE_SET( __members__ )									\
																			\
		PyObject * pyGet___methods__()										\
		{																	\
			ScriptList pList = ScriptList::create();						\
			this->pyAdditionalMethods( pList );								\
			return pList.newRef();											\
		}																	\
		PY_RO_ATTRIBUTE_SET( __methods__ )									\
																			\
	private:																\


/// This macro declares a method taking one object argument.
#define PY_UNARY_FUNC_METHOD( METHOD_NAME )									\
	PyObject * METHOD_NAME();												\
	static PyObject * _##METHOD_NAME( PyObject * self )						\
	{																		\
		return ((This*)self)->METHOD_NAME();								\
	}


/// This macro declares a method taking two object arguments.
#define PY_BINARY_FUNC_METHOD( METHOD_NAME )								\
	PyObject * METHOD_NAME( PyObject * a );									\
	static PyObject * _##METHOD_NAME( PyObject * self, PyObject * a )		\
	{																		\
		return ((This*)self)->METHOD_NAME( a );								\
	}


/// This macro declares a method taking three object arguments.
#define PY_TERNARY_FUNC_METHOD( METHOD_NAME )								\
	PyObject * METHOD_NAME( PyObject * a, PyObject * b );					\
	static PyObject * _##METHOD_NAME( PyObject * self, PyObject * a, PyObject * b ) \
	{																		\
		return ((This*)self)->METHOD_NAME( a, b );							\
	}


/// This macro declares a size inquiry method (taking no arguments).
#define PY_SIZE_INQUIRY_METHOD( METHOD_NAME )								\
	Py_ssize_t METHOD_NAME();												\
	static Py_ssize_t _##METHOD_NAME( PyObject * self )						\
	{																		\
		return ((This*)self)->METHOD_NAME();								\
	}

/// This macro declares a method taking no arguments.
#define PY_INQUIRY_METHOD( METHOD_NAME )									\
	int METHOD_NAME();														\
	static int _##METHOD_NAME( PyObject * self )							\
	{																		\
		return ((This*)self)->METHOD_NAME();								\
	}

/// This macro declares a coercion method.
#define PY_COERCION_METHOD( METHOD_NAME )									\
	static int METHOD_NAME( PyObject *& self, PyObject *& a );				\
	static int _##METHOD_NAME( PyObject ** self, PyObject ** a )			\
	{																		\
		return METHOD_NAME( *self, *a );									\
	}


/// This macro declares a method taking one int argument.
#define PY_INTARG_FUNC_METHOD( METHOD_NAME )								\
	PyObject * METHOD_NAME( Py_ssize_t a );									\
	static PyObject * _##METHOD_NAME( PyObject * self, Py_ssize_t a )		\
	{																		\
		return ((This*)self)->METHOD_NAME( a );								\
	}


/// This macro declares a method taking two int arguments.
#define PY_INTINTARG_FUNC_METHOD( METHOD_NAME )								\
	PyObject * METHOD_NAME( Py_ssize_t a, Py_ssize_t b );					\
	static PyObject * _##METHOD_NAME( PyObject * self, 						\
		Py_ssize_t a, Py_ssize_t b )										\
	{																		\
		return ((This*)self)->METHOD_NAME( a, b );							\
	}


/// This macro declares a method taking an int and an object.
#define PY_INTOBJARG_PROC_METHOD( METHOD_NAME )								\
	int METHOD_NAME( Py_ssize_t a, PyObject * b );							\
	static int _##METHOD_NAME( PyObject * self,								\
			Py_ssize_t a, PyObject * b )									\
	{																		\
		return ((This*)self)->METHOD_NAME( a, b );							\
	}


/// This macro declares a method taking two ints and an object.
#define PY_INTINTOBJARG_PROC_METHOD( METHOD_NAME )							\
	int METHOD_NAME( Py_ssize_t a, Py_ssize_t b, PyObject * c );			\
	static int _##METHOD_NAME( PyObject * self,								\
				Py_ssize_t a, Py_ssize_t b, PyObject * c )					\
	{																		\
		return ((This*)self)->METHOD_NAME( a, b, c );						\
	}


/// This macro declares a method taking one object argument (don't ask).
#define PY_OBJOBJ_PROC_METHOD( METHOD_NAME )								\
	int METHOD_NAME( PyObject * a );										\
	static int _##METHOD_NAME( PyObject * self, PyObject * a )				\
	{																		\
		return ((This*)self)->METHOD_NAME( a );								\
	}


/* More macros that may need to be declared... (from object.h)
typedef int (*getreadbufferproc)(PyObject *, int, void **);
typedef int (*getwritebufferproc)(PyObject *, int, void **);
typedef int (*getsegcountproc)(PyObject *, int *);
typedef int (*getcharbufferproc)(PyObject *, int, const char **);
typedef int (*objobjproc)(PyObject *, PyObject *);
typedef int (*visitproc)(PyObject *, void *);
typedef int (*traverseproc)(PyObject *, visitproc, void *);
*/




/// This macro declares a standard python method.
#define PY_METHOD_DECLARE( METHOD_NAME )									\
	PyObject * METHOD_NAME( PyObject * args );								\
																			\
	static PyObject * _##METHOD_NAME( PyObject * self,						\
		PyObject * args, PyObject * /*kwargs*/ )							\
	{																		\
		This * pSelf = static_cast< This * >( This::getSelf( self ) );		\
		return pSelf ? pSelf->METHOD_NAME( args ) : NULL;					\
	}																		\
																			\

/// This macro declares an auto-parsed standard python method
#define PY_AUTO_METHOD_DECLARE( RET, NAME, ARGS )							\
	static PyObject * _py_##NAME(											\
		PyObject * self, PyObject * args, PyObject * /*kwargs*/ )			\
	{																		\
		This * pThis = static_cast< This * >( This::getSelf( self ) );		\
		if (!pThis)															\
		{																	\
			return NULL;													\
		}																	\
		PY_AUTO_DEFINE_INT( RET, NAME, pThis->NAME, ARGS )					\
	}																		\

/// This macro declares a python method that uses keywords.
#define PY_KEYWORD_METHOD_DECLARE( METHOD_NAME )							\
	PyObject * METHOD_NAME( PyObject * args, PyObject * kwargs );			\
																			\
	static PyObject * _##METHOD_NAME( PyObject * self,						\
		PyObject * args, PyObject * kwargs )								\
	{																		\
		This * pSelf = static_cast< This * >( This::getSelf( self ) );		\
		return pSelf ? pSelf->METHOD_NAME( args, kwargs ) : NULL;			\
	}																		\
																			\

/// This macro declares a static python method
#define PY_STATIC_METHOD_DECLARE( NAME )									\
	static PyObject * NAME( PyObject * pArgs );								\
																			\
	static PyObject * _##NAME( PyObject *,									\
		PyObject * args, PyObject * )										\
	{																		\
		return This::NAME( args );											\
	}																		\

/// This macro declares an auto-parsed static python method
#define PY_AUTO_STATIC_METHOD_DECLARE( RET, NAME, ARGS )					\
	static PyObject * _py_##NAME(											\
		PyObject * self, PyObject * args, PyObject * kwargs )				\
	{																		\
		PY_AUTO_DEFINE_INT( RET, NAME, This::NAME, ARGS )					\
	}																		\


/// This macro declares the python factory method (pyNew) for this type
#define PY_FACTORY_DECLARE()												\
	static PyObject * pyNew( PyObject * pArgs );							\
																			\
	static PyObject * _pyNew( PyObject *, PyObject * args, PyObject * )		\
	{																		\
		return This::pyNew( args );											\
	}																		\
	PY_FACTORY_METHOD_LINK_DECLARE()										\

/// This macro declares the python factory method (New) as auto-parsed
#define PY_AUTO_FACTORY_DECLARE( CLASS_NAME, ARGS )							\
	static PyObject * _pyNew( PyObject *, PyObject * args )					\
	{																		\
		PY_AUTO_DEFINE_INT( RETOWN, CLASS_NAME, This::New, ARGS )			\
	}																		\
	PY_FACTORY_METHOD_LINK_DECLARE()										\


/// This macro declares the python factory method for the given class
/// It calls the constructor with auto-parsed arguments
#define PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( CLASS_NAME, ARGS )				\
	static PyObject * _pyNew( PyObject *, PyObject * args )					\
	{																		\
		PY_AUTO_DEFINE_INT( RETOWN, CLASS_NAME, new This, ARGS )			\
	}																		\
	PY_FACTORY_METHOD_LINK_DECLARE()										\

/// This macro is used internally by factory declaration macros
#define PY_FACTORY_METHOD_LINK_DECLARE()									\
	static PyFactoryMethodLink s_link_pyNew;								\


/// This macro declares 'setstate' and 'getstate' methods for simple pickling
#define PY_GETSETSTATE_METHODS_DECLARE()									\
	PY_METHOD_DECLARE( py___getstate__ )									\
	PY_METHOD_DECLARE( py___setstate__ )									\

/// This macro declares a 'reduce' method used for nontrivial pickling
/// The pyPickleReduce method should return a tuple of picklable arguments
/// to be passed to the construction function when the object is unpickled.
#define PY_PICKLING_METHOD_DECLARE( CONS_NAME )								\
	PyObject * pyPickleReduce();											\
																			\
	static PyObject * _py___reduce_ex__( PyObject * self, PyObject * )		\
	{																		\
		PyObject * pConsArgs = ((This*)self)->pyPickleReduce();				\
		return Script::buildReduceResult( #CONS_NAME, pConsArgs );			\
	}																		\
																			\



/// This macro declares a python attribute that is implemented elsewhere
#define PY_DEFERRED_ATTRIBUTE_DECLARE( NAME )								\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME();								\
	int PY_ATTR_SCOPE pySet_##NAME( PyObject * value );						\

/// This macro declares a class member that is read-write accessible from
///	python as an attribute
#define PY_RW_ATTRIBUTE_DECLARE( MEMBER, NAME )								\
	PY_READABLE_ATTRIBUTE_GET( MEMBER, NAME )								\
	PY_WRITABLE_ATTRIBUTE_SET( MEMBER, NAME )								\

/// This macro declares a class member that is read-write accessible from
///	python as an attribute
#define PY_RW_ATTRIBUTE_REF_DECLARE( MEMBER, NAME )							\
	PY_READABLE_ATTRIBUTE_REF_GET( MEMBER, NAME )							\
	PY_WRITABLE_ATTRIBUTE_SET( MEMBER, NAME )								\

/// This macro declares a class member that is read-only accessible from
///	python as an attribute
#define PY_RO_ATTRIBUTE_DECLARE( MEMBER, NAME )								\
	PY_RO_ATTRIBUTE_GET( MEMBER, NAME )										\
	PY_RO_ATTRIBUTE_SET( NAME )												\

/// This macro declares a class member that is write-only accessible from
///	python as an attribute
#define PY_WO_ATTRIBUTE_DECLARE( MEMBER, NAME )								\
	PY_WO_ATTRIBUTE_GET( NAME )												\
	PY_WRITABLE_ATTRIBUTE_SET( MEMBER, NAME )								\

/// This macro declares a class function that is write-only accessible from
///	python as an attribute
#define PY_WO_ATTRIBUTE_SETTER_DECLARE( TYPE, MEMBER, NAME )				\
	PY_WO_ATTRIBUTE_GET( NAME )												\
	PY_WRITABLE_ATTRIBUTE_SETTER( TYPE, MEMBER, NAME )						\

/// This macro defines the function that gets a readable attribute
#define PY_READABLE_ATTRIBUTE_GET( MEMBER, NAME )							\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
		{ return Script::getData( MEMBER ); }								\

/// This macro defines the function that gets a readable attribute
#define PY_RO_ATTRIBUTE_GET( MEMBER, NAME )									\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
		{ return Script::getReadOnlyData( MEMBER ); }						\

/// This macro defines the function that gets a readable attribute
#define PY_READABLE_ATTRIBUTE_REF_GET( MEMBER, NAME )						\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
		{ return Script::getDataRef( this, &MEMBER ); }						\

/// This macro defines the error function that is called when an attempt
///	is made to set a read-only attribute
#define PY_RO_ATTRIBUTE_SET( NAME )											\
	int PY_ATTR_SCOPE pySet_##NAME( PyObject * /*value*/ )					\
	{																		\
		PyErr_Format( PyExc_TypeError,										\
			"Sorry, the attribute " #NAME " in %s is read-only",			\
			this->typeName() );												\
		return -1;															\
	}																		\

/// This macro defines the error function that is called when an attempt
///	is made to get a write-only attribute
#define PY_WO_ATTRIBUTE_GET( NAME )											\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
	{																		\
		PyErr_Format( PyExc_TypeError,										\
			"Sorry, the attribute " #NAME " in %s is write-only",			\
			this->typeName() );												\
		return NULL;														\
	}																		\

/// This macro defines the function that sets a writable attribute
#define PY_WRITABLE_ATTRIBUTE_SET( MEMBER, NAME )							\
	int PY_ATTR_SCOPE pySet_##NAME( PyObject * value )						\
		{ return Script::setData( value, MEMBER, #NAME ); }					\

/// This macro defines the function that calls a setter function
#define PY_WRITABLE_ATTRIBUTE_SETTER( TYPE, SETTER, NAME )					\
	int PY_ATTR_SCOPE pySet_##NAME( PyObject * value )						\
		{																	\
			TYPE convertedValue;											\
			Script::setData( value, convertedValue, #NAME );				\
			return SETTER( convertedValue );								\
		}																	\


/// This macro declares a class member that is read-write accessible from
///	python as an attribute, and accessed using accessors
#define PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( TYPE, MEMBERFN, NAME )			\
	PY_READABLE_ATTRIBUTE_GET( this->MEMBERFN(), NAME )						\
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( TYPE, MEMBERFN, NAME )				\


/// This macro defines the function that sets an attribute through an accessor
#define PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( TYPE, MEMBERFN, NAME )			\
	int PY_ATTR_SCOPE pySet_##NAME( PyObject * value )						\
	{																		\
		typedef TYPE LocalTypeDef;	/* for calling constructor */			\
		TYPE newVal = LocalTypeDef();										\
		int ret = Script::setData( value, newVal, #NAME );					\
		if (ret == 0) this->MEMBERFN( newVal );								\
		return ret;															\
	}																		\


/// This macro can be defined to a class name to use the attribute
/// declaration macros below as method definitions
#define PY_ATTR_SCOPE


/**
 *	PyObjectPlus-derived class static data instantiation macros
 */

/// This high-level macro defines a simple type object
#define PY_TYPEOBJECT 				PY_GENERAL_TYPEOBJECT

#define PY_TYPEOBJECT_WITH_DOC 		PY_GENERAL_TYPEOBJECT_WITH_DOC

/// This high-level macro defines a type object with sequence methods
#define PY_TYPEOBJECT_WITH_SEQUENCE( THIS_CLASS, SEQ )						\
	PY_TYPEOBJECT_WITH_SEQUENCE_WITH_DOC( THIS_CLASS, SEQ, 0 )

#define PY_TYPEOBJECT_WITH_SEQUENCE_WITH_DOC( THIS_CLASS, SEQ, DOC )		\
	PY_TYPEOBJECT_SPECIALISE_SEQ( THIS_CLASS, SEQ )							\
	PY_TYPEOBJECT_SPECIALISE_DOC( THIS_CLASS, DOC )							\
	PY_GENERAL_TYPEOBJECT( THIS_CLASS )

/// This high-level macro defines a type object with mapping methods
#define PY_TYPEOBJECT_WITH_MAPPING( THIS_CLASS, MAP )						\
	PY_TYPEOBJECT_WITH_MAPPING_WITH_DOC( THIS_CLASS, MAP, 0 )

#define PY_TYPEOBJECT_WITH_MAPPING_WITH_DOC( THIS_CLASS, MAP, DOC )			\
	PY_TYPEOBJECT_SPECIALISE_DOC( THIS_CLASS, DOC )							\
	PY_TYPEOBJECT_SPECIALISE_MAP( THIS_CLASS, MAP )							\
	PY_GENERAL_TYPEOBJECT( THIS_CLASS )

/// This high-level macro defines a type object with a call method
#define PY_TYPEOBJECT_WITH_CALL( THIS_CLASS )								\
	PY_TYPEOBJECT_WITH_CALL_WITH_DOC( THIS_CLASS, 0 )

#define PY_TYPEOBJECT_WITH_CALL_WITH_DOC( THIS_CLASS, DOC )					\
	PY_TYPEOBJECT_SPECIALISE_DOC( THIS_CLASS, DOC )							\
	PY_TYPEOBJECT_SPECIALISE_CALL( THIS_CLASS, &THIS_CLASS::_pyCall )		\
	PY_GENERAL_TYPEOBJECT( THIS_CLASS )

/// This macro defines a type object that supports the iterator protocol
#define PY_TYPEOBJECT_WITH_ITER( THIS_CLASS, GETITER, ITERNEXT )			\
	PY_TYPEOBJECT_WITH_ITER_WITH_DOC( THIS_CLASS, GETITER, ITERNEXT, 0 )

#define PY_TYPEOBJECT_WITH_ITER_WITH_DOC( THIS_CLASS, GETITER, ITERNEXT, 	\
		DOC )																\
	PY_TYPEOBJECT_SPECIALISE_DOC( THIS_CLASS, DOC )							\
	PY_TYPEOBJECT_SPECIALISE_ITER( THIS_CLASS, GETITER, ITERNEXT )			\
	PY_GENERAL_TYPEOBJECT( THIS_CLASS )

/// This high-level macro defines a type object which is weak-referencable,
/// for use with the PY_WEAK_REFERENCABLE class definition macro
#define PY_TYPEOBJECT_WITH_WEAKREF( THIS_CLASS )							\
	PY_TYPEOBJECT_WITH_WEAKREF_WITH_DOC( THIS_CLASS, 0 )

#define PY_TYPEOBJECT_WITH_WEAKREF_WITH_DOC( THIS_CLASS, DOC )				\
	PY_TYPEOBJECT_SPECIALISE_DOC( THIS_CLASS, DOC )							\
	PY_TYPEOBJECT_SPECIALISE_WEAKREF( THIS_CLASS, _py_pWeakRefList )		\
	PY_GENERAL_TYPEOBJECT( THIS_CLASS )

/// Low-level macros for specialising PyObjectTypeUtil functions.

#define PY_TYPEOBJECT_SPECIALISE_BASIC_SIZE( THIS_CLASS, BASIC_SIZE)		\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		basicSize, int, BASIC_SIZE )

#define PY_TYPEOBJECT_SPECIALISE_ITER( THIS_CLASS, GETITER, ITERNEXT )		\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		getIterFunction, getiterfunc, GETITER )								\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		iterNextFunction, iternextfunc, ITERNEXT )

#define PY_TYPEOBJECT_SPECIALISE_CMP( THIS_CLASS, CMP )						\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		compareFunction, cmpfunc, CMP )

#define PY_TYPEOBJECT_SPECIALISE_REPR_AND_STR( THIS_CLASS, REPR, STR )		\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		reprFunction, reprfunc, REPR )										\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		strFunction, reprfunc, STR )

#define PY_TYPEOBJECT_SPECIALISE_NUM( THIS_CLASS, NUM )						\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, 							\
		asNumber, PyNumberMethods *, NUM )

#define PY_TYPEOBJECT_SPECIALISE_SEQ( THIS_CLASS, SEQ )						\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, 							\
		asSequence, PySequenceMethods *, SEQ )

#define PY_TYPEOBJECT_SPECIALISE_MAP( THIS_CLASS, MAP )						\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, 							\
		asMapping, PyMappingMethods *, MAP )

#define PY_TYPEOBJECT_SPECIALISE_CALL( THIS_CLASS, CALL )					\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, 							\
		callFunction, ternaryfunc, CALL )

// Python's internal weakref code finds the PyObject * to store its weakref
// list in by adding this value to the PyObject * it has, if the value's not 0.
#define PY_TYPEOBJECT_SPECIALISE_WEAKREF( THIS_CLASS, WEAKREFLIST )			\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, 							\
		weakListOffset, Py_ssize_t,											\
		bw_offsetof_from( THIS_CLASS, WEAKREFLIST, PyObject ) )

#define PY_TYPEOBJECT_SPECIALISE_FLAGS( THIS_CLASS, FLAGS )					\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS,							\
		flags, long, FLAGS )

#define PY_TYPEOBJECT_SPECIALISE_DOC( THIS_CLASS, DOC )						\
	PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, doc, const char *, DOC )

#define PY_TYPEOBJECT_SPECIALISE_SIMPLE( THIS_CLASS, TEMPLATE_FUNC,  		\
		RET_TYPE, RET_VALUE )												\
	namespace PyTypeObjectUtil 												\
	{																		\
		template<>															\
		RET_TYPE TEMPLATE_FUNC< THIS_CLASS >()								\
		{																	\
			return RET_VALUE; 												\
		}																	\
	} // end namespace PyTypeObjectUtil



/// This low-level macro defines the PyTypeObject for this class.
#define PY_GENERAL_TYPEOBJECT( THIS_CLASS )									\
	PY_GENERAL_TYPEOBJECT_WITH_BASE( THIS_CLASS, &Super::s_type_ ) 			\

/// This low-level macro defines the PyTypeObject for this class, with the
/// given docstring.
#define PY_GENERAL_TYPEOBJECT_WITH_DOC( THIS_CLASS, DOC )					\
	PY_GENERAL_TYPEOBJECT_WITH_BASE_WITH_NAME( THIS_CLASS, &Super::s_type_, \
		#THIS_CLASS )

/// This low-level macro defines the PyTypeObject for this class, with the
/// given base class.
#define PY_GENERAL_TYPEOBJECT_WITH_BASE( THIS_CLASS, BASE )					\
	PY_GENERAL_TYPEOBJECT_WITH_BASE_WITH_NAME( THIS_CLASS, BASE, 			\
		#THIS_CLASS )

/// This low-level macro defines the PyTypeObject for this class, with the
///	given base class and the / given docstring.
#define PY_GENERAL_TYPEOBJECT_WITH_BASE_WITH_NAME( THIS_CLASS, BASE, NAME )	\
																			\
	PyTypeObject THIS_CLASS::s_type_ =										\
	{																		\
		PyObject_HEAD_INIT(&PyType_Type)									\
		0,									/* ob_size */					\
		const_cast< char * >( NAME ),		/* tp_name */					\
		PyTypeObjectUtil::basicSize< THIS_CLASS >(),						\
											/* tp_basicsize */				\
		0,									/* tp_itemsize */				\
																			\
		/* methods */														\
		THIS_CLASS::_tp_dealloc,			/* tp_dealloc */				\
		0,									/* tp_print */					\
		0,									/* tp_getattr */				\
		0,									/* tp_setattr */				\
		PyTypeObjectUtil::compareFunction< THIS_CLASS >(),					\
											/* tp_compare */				\
		PyTypeObjectUtil::reprFunction< THIS_CLASS >(),						\
											/* tp_repr */					\
		PyTypeObjectUtil::asNumber< THIS_CLASS >(),							\
											/* tp_as_number */				\
		PyTypeObjectUtil::asSequence< THIS_CLASS >(),						\
											/* tp_as_sequence */			\
		PyTypeObjectUtil::asMapping< THIS_CLASS >(),						\
											/* tp_as_mapping */				\
		0,									/* tp_hash */					\
		PyTypeObjectUtil::callFunction< THIS_CLASS >(),						\
											/* tp_call */					\
		PyTypeObjectUtil::strFunction< THIS_CLASS >(),						\
											/* tp_str */					\
		THIS_CLASS::_tp_getattro,			/* tp_getattro */				\
		THIS_CLASS::_tp_setattro,			/* tp_setattro */				\
		0,									/* tp_as_buffer */				\
		PyTypeObjectUtil::flags< THIS_CLASS >(),							\
											/* tp_flags */					\
		const_cast< char * >(												\
				PY_GET_DOC( PyTypeObjectUtil::doc< THIS_CLASS >() ) ),		\
											/* tp_doc */					\
		0,									/* tp_traverse */				\
		0,									/* tp_clear */					\
		0,									/* tp_richcompare */			\
		PyTypeObjectUtil::weakListOffset< THIS_CLASS >(),					\
											/* tp_weaklistoffset */			\
		PyTypeObjectUtil::getIterFunction< THIS_CLASS >(),					\
											/* tp_iter */					\
		PyTypeObjectUtil::iterNextFunction< THIS_CLASS >(),					\
											/* tp_iternext */				\
		THIS_CLASS::s_getMethodDefs(),		/* tp_methods */				\
		0,									/* tp_members */				\
		THIS_CLASS::s_getAttributeDefs(),	/* tp_getset */					\
		BASE,								/* tp_base */					\
		0,									/* tp_dict */					\
		0,									/* tp_descr_get */				\
		0,									/* tp_descr_set */				\
		0,									/* tp_dictoffset */				\
		0,									/* tp_init */					\
		0,									/* tp_alloc */					\
		(newfunc)THIS_CLASS::_pyNew,		/* tp_new */					\
		0,									/* tp_free */					\
		0,									/* tp_is_gc */					\
		0,									/* tp_bases */					\
		0,									/* tp_mro */					\
		0,									/* tp_cache */					\
		0,									/* tp_subclasses */				\
		0,									/* tp_weaklist */				\
		0,									/* tp_del */					\
		0,									/* tp_version_tag */			\
	};


namespace PyTypeObjectUtil
{

template< typename T >
int basicSize()
{
	return sizeof( T );
}

template< typename T >
cmpfunc compareFunction()
{
	return 0;
}

template< typename T >
reprfunc reprFunction()
{
	return &T::_tp_repr;
}

template< typename T >
PyNumberMethods * asNumber()
{
	return 0;
}

template< typename T >
PySequenceMethods * asSequence()
{
	return 0;
}

template< typename T >
PyMappingMethods * asMapping()
{
	return 0;
}

template< typename T >
ternaryfunc callFunction()
{
	return 0;
}

template< typename T >
reprfunc strFunction()
{
	return 0;
}

template< typename T >
long flags()
{
	return Py_TPFLAGS_DEFAULT;
}

template< typename T >
const char * doc()
{
	return 0;
}

template< typename T >
getiterfunc getIterFunction()
{
	return 0;
}

template< typename T >
iternextfunc iterNextFunction()
{
	return 0;
}

template< typename T >
inline Py_ssize_t weakListOffset()
{
	return 0;
}

} // end namespace PyTypeObjectUtil


/// This macro initiates the method definitions for the given class
#define PY_BEGIN_METHODS( THIS_CLASS )										\
	/* static */ PyMethodDef * THIS_CLASS::s_getMethodDefs()				\
	{																		\
		static PyMethodDef s_methods[] =									\
		{																	\

/// This macro defines a Python method
#define PY_METHOD( NAME )													\
			{ #NAME, (PyCFunction)&_py_##NAME, METH_VARARGS|METH_KEYWORDS, NULL },		\

/// This macro defines a Python method
#define PY_METHOD_WITH_DOC( NAME, DOC_STRING )													\
			{ #NAME, (PyCFunction)&_py_##NAME, METH_VARARGS|METH_KEYWORDS, PY_GET_DOC( DOC_STRING ) },		\

/// This macro defines an alias for a Python method
#define PY_METHOD_ALIAS( NAME, ALIAS )										\
			{ #ALIAS, (PyCFunction)&_py_##NAME, METH_VARARGS|METH_KEYWORDS, NULL },		\

/// This macro concludes the method definitions
#define PY_END_METHODS()													\
			{ NULL, NULL, 0, NULL }											\
		};																	\
		return s_methods;													\
	}																		\

#define PY_ATTRIBUTE_CREATE_WRAPPER( NAME, ATTRIB_NAME )					\
	class PyAttrib_##NAME													\
	{																		\
	public:																	\
		static PyObject * get( PyObject * self, void * closure )			\
		{																	\
			This * selfObj = static_cast< This * >( This::getSelf( self ) );\
			return selfObj ? selfObj->pyGet_##ATTRIB_NAME() : NULL;			\
		}																	\
																			\
		static int set( PyObject * self, PyObject * value, void * closure )	\
		{																	\
			This * selfObj = static_cast< This * >( This::getSelf( self ) );\
			return selfObj ? selfObj->pySet_##ATTRIB_NAME( value ) : -1;	\
		}																	\
	};																		\

/// This macro initiates the attribute definitions for the given class.
#define PY_BEGIN_ATTRIBUTES( THIS_CLASS )									\
	/* static */ PyGetSetDef * THIS_CLASS::s_getAttributeDefs()				\
	{																		\
		static BW::vector<PyGetSetDef> s_attributes;						\
		if (!s_attributes.empty())											\
		{																	\
			return &s_attributes[0];										\
		}																	\
		PY_ATTRIBUTE( __members__ )											\
		PY_ATTRIBUTE( __methods__ )											\

/// This macro defines a Python attribute
#define PY_ATTRIBUTE( NAME )												\
		PY_ATTRIBUTE_CREATE_WRAPPER( NAME, NAME )							\
		PyGetSetDef member_##NAME = {										\
			#NAME,								/* name */					\
			(getter)PyAttrib_##NAME::get,		/* get */					\
			(setter)PyAttrib_##NAME::set,		/* set */					\
			NULL,								/* doc */					\
			NULL,								/* closure */				\
		};																	\
		s_attributes.push_back( member_##NAME );							\

/// This macro defines a Python attribute
#define PY_ATTRIBUTE_ALIAS( NAME, NEW_NAME )								\
		PY_ATTRIBUTE_CREATE_WRAPPER( NEW_NAME, NAME )						\
		PyGetSetDef member_##NEW_NAME = {									\
			#NEW_NAME,							/* name */					\
			(getter)PyAttrib_##NEW_NAME::get,	/* get */					\
			(setter)PyAttrib_##NEW_NAME::set,	/* set */					\
			NULL,								/* doc */					\
			NULL,								/* closure */				\
		};																	\
		s_attributes.push_back( member_##NEW_NAME );						\


/// This macro terminates the attribute definitions
#define PY_END_ATTRIBUTES()													\
		PyGetSetDef _member_null = { NULL, NULL, NULL, NULL, NULL };		\
		s_attributes.push_back( _member_null );								\
		return &s_attributes[0];											\
	}																		\


/// This macro defines the factory method for the given class
/// Note: if you're making a class that is created by a factory method and
/// not being included into any other C++ files, then the linker will
/// optimise it out. To avoid this, add the following to app.cpp:
/// extern int ClassName_token;
/// static int projectorTokenSet = ClassName_token;
/// (but replacing ClassName with your class' name)
#define PY_FACTORY( THIS_CLASS, MODULE_NAME )								\
	PY_FACTORY_NAMED( THIS_CLASS,											\
		THIS_CLASS::s_type_.tp_name, MODULE_NAME )							\

/// This macro defines the factory method with python name for the given class
/// Note: if you're making a class that is created by a factory method and
/// not being included into any other C++ files, then the linker will
/// optimise it out. To avoid this, add the following to app.cpp:
/// extern int ClassName_token;
/// static int projectorTokenSet = ClassName_token;
/// (but replacing ClassName with your class' name)
#define PY_FACTORY_NAMED( THIS_CLASS, METHOD_NAME, MODULE_NAME )			\
	PyFactoryMethodLink THIS_CLASS::s_link_pyNew = PyFactoryMethodLink(		\
		#MODULE_NAME, METHOD_NAME, &THIS_CLASS::s_type_ );					\
	/* can't call the constructor directly with VC6 templates (don't ask) */\
	PY_CLASS_TOKEN( THIS_CLASS )											\

/// This macro defines a token for this class, but not inside it
#define PY_CLASS_TOKEN_BASE( THIS_CLASS )									\
	int THIS_CLASS##_token = true;											\

/// This macro uses the macro above. Template classes will need to redefine it
#define PY_CLASS_TOKEN( THIS_CLASS )										\
	PY_CLASS_TOKEN_BASE( THIS_CLASS )										\


/// This macro defines getstate and setstate methods
#define PY_GETSETSTATE_METHODS()											\
	PY_METHOD( __getstate__ )												\
	PY_METHOD( __setstate__ )												\

/// This macro implements standard getstate and setstate methods using
/// script converters (only works with pod types).
#define PY_CONVERTERS_GETSETSTATE_METHODS( THIS_CLASS, POD_TYPE )			\
	PyObject * THIS_CLASS::py___getstate__( PyObject * args )				\
	{																		\
		POD_TYPE basicType;													\
		if (Script::setData( this, basicType, #THIS_CLASS " setstate" ) != 0)\
			return NULL;													\
		return PyString_FromStringAndSize(									\
			(char*)&basicType, sizeof(basicType) );							\
	}																		\
																			\
	PyObject * THIS_CLASS::py___setstate__( PyObject * args )				\
	{																		\
		PyObject * soleArg;													\
		if (PyTuple_Size( args ) != 1 ||									\
			!PyString_Check( soleArg = PyTuple_GET_ITEM( args, 0 ) ) ||		\
			PyString_Size( soleArg ) != sizeof( POD_TYPE ))					\
		{																	\
			PyErr_SetString( PyExc_TypeError, #THIS_CLASS " getstate "		\
				"expects a single string argument of correct length" );		\
		}																	\
		/* not sure how to use getData here... */							\
		/* *this = (POD_TYPE*)PyString_AsString( soleArg ); */				\
		PyObject * goodValue = Script::getData(								\
			(POD_TYPE*)PyString_AsString( soleArg ) );						\
		this->copy( *goodValue );	/* hmmm */								\
		/* this would prolly be better done with normal pickling then...*/	\
		/* ... ah well, next commit :) */									\
		Py_RETURN_NONE;														\
	}																		\

/// This macro defines a pickling reduce method in the method definitions list
#define PY_PICKLING_METHOD()												\
	PY_METHOD( __reduce_ex__ )												\



/**
 *	pyGetAttribute and pySetAttribute method helper macros
 */

//	This macro is deprecated, but left for backwards compatability
#define PY_GETATTR_STD()

//	This macro is deprecated, but left for backwards compatability
#define PY_SETATTR_STD()

// -----------------------------------------------------------------------------
// Section: PyObjectPlus
// -----------------------------------------------------------------------------

/*~ class NoModule.PyObjectPlus
 *	@components{ all }
 *
 *	This is the abstract base class for all Python classes which are implemented
 *	in the engine.  Every other class inherits, directly or indirectly
 *	from PyObjectPlus.
 */
/**
 *	This class is a base class for all implementations of our Python types.
 *	It helps make implementing a Python type easy.
 *
 * 	@ingroup script
 */
class PyObjectPlus : public PyObject
{
	Py_Header( PyObjectPlus, PyObjectPlus )

	public:
		PyObjectPlus( PyTypeObject * pType, bool isInitialised = false );

		/// This method increments the reference count on this object.
		void incRef() const				{ Py_INCREF( (PyObject*)this ); }
		/// This method decrements the reference count on this object.
		void decRef() const				{ Py_DECREF( (PyObject*)this ); }
		/// This method returns the reference count on this object.
		Py_ssize_t refCount() const		{ return ((PyObject*)this)->ob_refcnt; }

		ScriptObject pyGetAttribute( const ScriptString & attrObj );
		bool pySetAttribute( const ScriptString & attrObj,
			const ScriptObject & value );
		bool pyDelAttribute( const ScriptString & attrObj );
		void pyDel();

		PyObject * pyRepr();

		// PyTypeObject * pyType()	{ return ob_type; }

		const char * typeName() const
		{
			return ob_type->tp_name;
		}

		// ####Python2.3 Do we still want these? There is an official way to do
		// this now (__dir__). Need to fully remove it.
		/// This virtual method can be overridden to add the names
		/// of any additional members to a __members__ query
		void pyAdditionalMembers( const ScriptList & pList ) const
		{
		}

		/// This virtual method can be overridden to add the names
		/// of any additional methods to a __methods__ query
		void pyAdditionalMethods( const ScriptList & pList ) const
		{
		}

		static PyObject * _pyNew( PyTypeObject * t, PyObject *, PyObject * );
		static PyObjectPtr coerce( PyObject * pObject )
		{
			return pObject;
		}

		static This * getSelf( PyObject * pObject )
		{
			return static_cast< This * >( pObject );
		}

		static const PyGetSetDef * searchAttributes( const PyGetSetDef *,
				const char *);
		static const PyMethodDef * searchMethods( const PyMethodDef *,
				const char * );

	protected:

		/**
		 * This destructor is protected because nobody should call it
		 * directly except Python, when the reference count reaches zero.
		 */
		~PyObjectPlus();
};

/// Deprecated, use PyObjectPlus instead of PyInstancePlus
typedef PyObjectPlus PyInstancePlus;


template <class PYCLASS>
bool isAdditionalProperty( const char *name )
{
	const PyGetSetDef *attr = PyObjectPlus::searchAttributes(
			PYCLASS::s_getAttributeDefs(), name );
	const PyMethodDef *method = PyObjectPlus::searchMethods(
			PYCLASS::s_getMethodDefs(), name );
	return attr != NULL || method != NULL;
}


/**
 *	Little class to make sure objects are ordered in the right way
 */
class PyObjectPlusWithVD : public PyObjectPlus
{
public:
	PyObjectPlusWithVD( PyTypeObject * pType ) : PyObjectPlus( pType ) { }
	virtual ~PyObjectPlusWithVD() { }
};


// -----------------------------------------------------------------------------
// Section: Weak-referenceable macros
// -----------------------------------------------------------------------------
/**
 *	Little class to control lifetime of a _py_pWeakRefList. It relies on the
 *	magic in the PY_WEAK_REFERENCABLE macro, so ignore it unless it breaks.
 */
template< class PYWEAKREFERENCABLE >
class PyWeakRefListManager
{
public:
	PyWeakRefListManager()
	{
		pOwner()->_py_pWeakRefList = NULL;
	}

	~PyWeakRefListManager()
	{
		PYWEAKREFERENCABLE * pOwner = this->pOwner();
		if (pOwner->_py_pWeakRefList == NULL)
		{
			return;
		}
		PyObject_ClearWeakRefs( static_cast< PyObject * >( pOwner ) );
	}

private:
	PYWEAKREFERENCABLE * pOwner()
	{
		// Not using PYWEAKREFERENCABLE::weakListOffset so that PyObjectPlus
		// subclasses without a _py_pWeakRefList fail to compile, rather than
		// relying on a runtime assertion like:
		// MF_ASSERT( PYWEAKREFERENCABLE::weakListOffset() != 0 );
		return bw_container_of( this, PYWEAKREFERENCABLE,
			_py_weakRefListManager );
	}
};

/// This macro defines the in-class components of a weak-referenceable PyObject
/// It should be private to the class.
/// Classes should also declare their typeobject as PY_TYPEOBJECT_WITH_WEAKREF
/// if they use this macro directly, or PY_TYPEOBJECT_SPECIALISE_WEAKREF if
/// the typeobject is more complex.
#define PY_WEAK_REFERENCABLE( THIS_CLASS )									\
	PyObject *	_py_pWeakRefList;											\
	PyWeakRefListManager< THIS_CLASS > _py_weakRefListManager;				\
	friend class PyWeakRefListManager< THIS_CLASS >;						\
	template< typename T >													\
	friend Py_ssize_t PyTypeObjectUtil::weakListOffset();

/**
 *	This class provides an inheritable version of PyObjectPlus with the
 *	PY_WEAK_REFERENCABLE and PY_TYPEOBJECT_WITH_WEAKREF macros applied, and
 *	also an example of their use.
 *	Note that it also provides the equivalent of PyObjectPlusWithVD
 */
class PyObjectPlusWithWeakReference : public PyObjectPlus
{
	Py_Header( PyObjectPlusWithWeakReference, PyObjectPlus )

	PY_WEAK_REFERENCABLE( PyObjectPlusWithWeakReference )

public:
	PyObjectPlusWithWeakReference( PyTypeObject * pType ) :
		PyObjectPlus( pType ),
		_py_pWeakRefList()
	{}

	virtual ~PyObjectPlusWithWeakReference() {}
};


/**
 *	This macro allows a class that isn't a Python object to appear as if it
 *	were, for the purposes of defining attributes and methods with the macros
 *	in this file.
 *
 *	This can be useful for extensions to PyObjectPlus instances that are not
 *	themselves Python objects. The dummy methods defined here are never called,
 *	they are just to please the macros, particularly PY_TYPEOBJECT.
 *
 *	It needs only to be used in once in a base class and is not necessary in
 *	derived classes. It can be used with or without Py_HeaderSimple. Use it
 *	without if your base class is abstract.
 */
#define PY_FAKE_PYOBJECTPLUS_BASE_DECLARE()								\
	public:																\
		static void _tp_dealloc( PyObject * ) { }						\
																		\
		static PyObject * _tp_getattro( PyObject *, PyObject * )		\
			{ return NULL; }											\
																		\
		static int _tp_setattro( PyObject *, PyObject *, PyObject * )	\
			{ return -1; }												\
																		\
		static PyObject * _tp_repr( PyObject * )						\
			{ return NULL; }											\
																		\
		static PyObject * _pyNew( PyTypeObject * )						\
			{ return NULL; }											\
																		\
	private:															\


/**
 *	This macro is the counterpart definition to the
 *	PY_FAKE_PYOBJECTPLUS_BASE_DECLARE declaration.
 *
 *	@see PY_FAKE_PYOBJECTPLUS_BASE_DECLARE
 */
#define PY_FAKE_PYOBJECTPLUS_BASE( CLASS_NAME )							\


BW_END_NAMESPACE

// This function override ensures that attempting to delete a PyObject* with
// bw_safe_delete leads to a link-time error.
void bw_safe_delete( PyObject * & p );

#endif // PYOBJECT_PLUS_HPP
