#ifndef PY_SCRIPT_OBJECT_HPP
#define PY_SCRIPT_OBJECT_HPP

#include "Python.h"

#include "bwentity/bwentity_api.hpp"
#include "pyscript/pyobject_pointer.hpp"
#include "pyscript/script.hpp" // getData and setData


BW_BEGIN_NAMESPACE

class ScriptDict;
class ScriptIter;
class ScriptTuple;
class ScriptString;
class ScriptArgs;

namespace Script
{
inline void printError()
{
	PyErr_Print();
}

inline void clearError()
{
	PyErr_Clear();
}

inline bool hasError()
{
	return (PyErr_Occurred() != 0);
}
} // Namespace script

/**
 *	The ScriptErrorClear class will clear errors which occur when executing
 *	script object methods.
 */
class ScriptErrorClear
{
public:
	/**
	 *	This method handles what happens when an error occurs
	 */
	inline void handleError() const
	{
		Script::clearError();
	}

	/**
	 *	This method handles checking if a pointer is null, but no exception
	 *	is available
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrNoException( const void * ptr ) const {}

	/**
	 *	This method handles checking if a pointer is null
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrError( const void * ptr ) const
	{
		if (ptr == NULL)
		{
			this->handleError();
		}
	}

	/**
	 *	This method handles checking if a result is -1
	 *	@param result	The result to check
	 */
	inline void checkMinusOne( int result ) const
	{
		if (result == -1)
		{
			this->handleError();
		}
	}


	/**
	 *	This method handles checking if an exception has been marked as occured
	 */
	inline void checkErrorOccured() const
	{
		if (Script::hasError())
		{
			this->handleError();
		}
	}
};


/**
 *	The ScriptErrorPrint class will print and clear errors which occur when 
 *	executing script object methods.
 */
class ScriptErrorPrint
{
public:
	/**
	 *	ScriptErrorPrint constructor
	 *	@param prefix		A prefix to display when an error occurs
	 */
	ScriptErrorPrint( const char * prefix = NULL ) : errorPrefix_( prefix )
	{
	}


	/**
	 *	This method handles what happens when an error occurs
	 */
	inline void handleError() const
	{
		if (errorPrefix_)
		{
			ERROR_MSG( "%s\n", errorPrefix_ );
		}

		Script::printError();
		Script::clearError();
	}

	/**
	 *	This method handles checking if a pointer is null, but no exception
	 *	is available
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrNoException( const void * ptr ) const {}


	/**
	 *	This method handles checking if a pointer is null
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrError( const void * ptr ) const
	{
		if (ptr == NULL)
		{
			this->handleError();
		}
	}

	/**
	 *	This method handles checking if a result is -1
	 *	@param result	The result to check
	 */
	inline void checkMinusOne( int result ) const
	{
		if (result == -1)
		{
			this->handleError();
		}
	}


	/**
	 *	This method handles checking if an exception has been marked as occured
	 */
	inline void checkErrorOccured() const
	{
		if (Script::hasError())
		{
			this->handleError();
		}
	}
private:
	const char * errorPrefix_;
};

/**
 *	The ScriptErrorRetain class will not have any affect on errors which occur.
 */
class ScriptErrorRetain
{
public:
	/**
	 *	This method handles what happens when an error occurs
	 */
	inline void handleError() const {}
	/**
	 *	This method handles checking if a pointer is null, but no exception
	 *	is available
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrNoException( const void * ptr ) const {}
	/**
	 *	This method handles checking if a pointer is null
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrError( const void * ptr ) const {}
	/**
	 *	This method handles checking if a result is -1
	 *	@param result	The result to check
	 */
	inline void checkMinusOne( int result ) const {}
	/**
	 *	This method handles checking if an exception has been marked as occured
	 */
	inline void checkErrorOccured() const {}
};


/**
 *	This class is the base class for all other ScriptObjects, it provides
 *	generic functionality that is avilable to all ScriptObjects.
 */
class BWENTITY_API ScriptObject : public PyObjectPtr
{
public:
	static const bool FROM_NEW_REFERENCE = true;
	static const bool FROM_BORROWED_REFERENCE = false;

	/**
	 *	ScriptObject Constructor
	 */
	ScriptObject() : PyObjectPtr()
	{
	}

	/**
	 *	ScriptObject Constructor
	 *
	 *	@param pObject		The Python object to construct the ScriptObject from
	 *	@param alreadyIncremented	For this value you should use the relevent
	 *		ScriptObject::FROM_NEW_REFERENCE or 
	 *		ScriptObject::FROM_BORROWED_REFERENCE based on pObject
	 */
	ScriptObject( PyObject * pObject, bool alreadyIncremented ) :
		PyObjectPtr( pObject, alreadyIncremented )
	{
	}


	/**
	 *	ScriptObject Constructor
	 *
	 *	@param A PyObjectPtr to construct a script object from
	 */
	explicit ScriptObject( const PyObjectPtr & pObject ) :
		PyObjectPtr( pObject )
	{
	}

	/**
	 *	This method assigns the ScriptObject to the the value of a PyObjectPtr
	 *
	 *	@param other The PyObjectPtr to set the ScriptObject to
	 */
	ScriptObject & operator=( const PyObjectPtr & other )
	{
		this->PyObjectPtr::operator=( other );

		return *this;
	}


	/**
	 *	This method checks if the ScriptObject has an attribute. It is 
	 *	recommended to use getAttribute if you plan on retreiving the 
	 *	attribute if it exists.
	 *
	 *	@param key		The attribute key name
	 *	@return			True if the attribute exists, false otherwise
	 */
	bool hasAttribute( const char * key ) const
	{
		return PyObject_HasAttrString( this->get(), const_cast< char * > ( key ) ) == 1;
	}


	/**
	 *	This method gets an attribute from a ScriptObject
	 *
	 *	@param key			The key to lookup within the ScriptObject
	 *	@param errorHandler The type of error handling to use if this method 
	 *		fails
	 *	@return				The ScriptObject for the specified key
	 */
	template <class ERROR_HANDLER>
	ScriptObject getAttribute( const char * key, 
		const ERROR_HANDLER & errorHandler ) const
	{
		PyObject * result = PyObject_GetAttrString( this->get(), 
			const_cast< char * >( key ) );

		errorHandler.checkPtrError( result );

		return ScriptObject( result,
				ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method gets an attribute from a ScriptObject, and assigns it to
	 *	the supplied result if the type is appropriate.
	 *
	 *	@param key			The key to lookup within the ScriptObject
	 *	@param rResult		The value to assign the attribute's value to
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *
	 *	@return				True if successful, false if attribute does not
	 *		exist or value could not be converted into rResult.
	 */
	template <class ERROR_HANDLER, class RESULT_TYPE>
	bool getAttribute( const char * key, RESULT_TYPE & rResult,
		const ERROR_HANDLER & errorHandler ) const
	{
		ScriptObject attr = this->getAttribute( key, errorHandler );
		if (!attr)
		{
			return false;
		}
		return attr.convertTo( rResult, key, errorHandler );
	}


	/**
	 *	This method sets an attribute on the ScriptObject
	 *
	 *	@param key			The key to set the value of on the ScriptObject
	 *	@param value		The value to set key to
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return				True on success, false otherwise
	 */
	template <class ERROR_HANDLER>
	bool setAttribute( const char * key, const ScriptObject & value,
			const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyObject_SetAttrString( this->get(), key, value.get() );
		errorHandler.checkMinusOne( result );
		return result != -1;
	}


	/**
	 *	This method sets an attribute on the ScriptObject
	 *
	 *	@param key			The key to set the value of on the ScriptObject
	 *	@param value		The value to set key to
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return				True on success, false otherwise
	 */
	template <class ERROR_HANDLER>
	bool setAttribute( const char * key, PyObject * value,
		const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyObject_SetAttrString( this->get(), key, value );
		errorHandler.checkMinusOne( result );
		return result != -1;
	}


	/**
	 *	This method sets an attribute on the ScriptObject
	 *
	 *	@param key			The key to set the value of on the ScriptObject
	 *	@param value		The value to set key to
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return				True on success, false otherwise
	 */
	template <class ERROR_HANDLER>
	bool setAttribute( const char * key, PyTypeObject & value,
		const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyObject_SetAttrString( this->get(),
				key, (PyObject*)&value );
		errorHandler.checkMinusOne( result );
		return result != -1;
	}


	/**
	 *	This method sets an attribute on the ScriptObject
	 *
	 *	@param key			The key to set the value of on the ScriptObject
	 *	@param value		The value to set key to
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return				True on success, false otherwise
	 */
	template <class ERROR_HANDLER, class RESULT_TYPE>
	bool setAttribute( const char * key, const RESULT_TYPE & value,
		const ERROR_HANDLER & errorHandler ) const
	{
		ScriptObject valueObj = ScriptObject::createFrom( value );

		if (!valueObj)
		{
			return false;
		}

		return this->setAttribute( key, valueObj, errorHandler );
	}


	template <class ERROR_HANDLER>
	ScriptObject callMethod( const char * methodName,
			const ERROR_HANDLER & errorHandler,
			bool allowNullMethod = false ) const;

	template <class ERROR_HANDLER>
	ScriptObject callMethod( const char * methodName, 
			const ScriptArgs & args,
			const ERROR_HANDLER & errorHandler,
			bool allowNullMethod = false ) const;

	template <class ERROR_HANDLER>
	ScriptObject callFunction( const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER>
	ScriptObject callFunction( const ScriptArgs & args,
		const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER>
	inline ScriptIter getIter( const ERROR_HANDLER & errorHandler ) const;


	/**
	 *	This method gets a None ScriptObject
	 *
	 *	@return A none ScriptObject
	 */
	static ScriptObject none()
	{
		return ScriptObject( Py_None, ScriptObject::FROM_BORROWED_REFERENCE );
	}


	template <class ERROR_HANDLER>
	inline ScriptString str( const ERROR_HANDLER & errorHandler ) const;


	/**
	 *	This method checks if this ScriptObject is none
	 *
	 *	@return True if the ScriptObject is None, false otherwise.
	 */
	bool isNone() const
	{
		return this->get() == Py_None;
	}


	/**
	 *	This method checks if the ScriptObject is true
	 *
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return		True if this object is true, false otherwise
	 */
	template <class ERROR_HANDLER>
	bool isTrue( const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyObject_IsTrue( this->get() );
		errorHandler.checkMinusOne( result );
		return result == 1;
	}


	/**
	 *	This method gets a new PyObject* reference to this object
	 *	@return A new reference to the ScriptObject
	 */
	PyObject * newRef() const
	{
		if (object_)
		{
			incrementReferenceCount( *object_ );
		}

		return this->get();
	}


	/**
	 *	This method gets the type name of this object
	 *	@return The name of this objects type
	 */
	const char * typeNameOfObject() const
	{
		return object_->ob_type->tp_name;
	}


	/**
	 *	This method checks if this object is callable
	 *	@return True if the object is callable, false otherwise
	 */
	bool isCallable() const
	{
		return PyCallable_Check( this->get() ) == 1;
	}


	/**
	 * This method checks if this object is a member of the given type
	 *	@param type		The type object to check this object is a subclass of
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return			True if this object is a subtype of type, 
	 *		false otherwise
	 */
	template <class ERROR_HANDLER>
	bool isSubClass( const PyTypeObject & type, 
		const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyObject_IsSubclass( this->get(), (PyObject *)&type );
		errorHandler.checkMinusOne( result );
		return result == 1;
	}

	/**
	 *	This method compares two objects together
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return The compare result
	 */
	template <class ERROR_HANDLER>
	int compareTo( ScriptObject other,
		const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyObject_Compare( this->get(), other.get() );
		errorHandler.checkErrorOccured();
		return result;
	}


	template <class TYPE>
	static ScriptObject createFrom( TYPE val );

	template <class ERROR_HANDLER, class TYPE>
	bool convertTo( TYPE & rVal, const char * varName, 
			const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER, class TYPE>
	bool convertTo( TYPE & rVal, const ERROR_HANDLER & errorHandler ) const;
};


inline bool operator==( const ScriptObject & obj1, const ScriptObject & obj2 )
{
	return obj1.get() == obj2.get();
}

inline bool operator!=( const ScriptObject & obj1, const ScriptObject & obj2 )
{
	return obj1.get() != obj2.get();
}

inline bool operator<( const ScriptObject & obj1, const ScriptObject & obj2 )
{
	return obj1.get() < obj2.get();
}

inline bool operator>( const ScriptObject & obj1, const ScriptObject & obj2 )
{
	return obj1.get() > obj2.get();
}


#define BASE_SCRIPT_OBJECT_IMP_WITHOUT_CREATE( OBJECT_TYPE, TYPE, BASE_TYPE ) \
	TYPE() : BASE_TYPE()													\
	{																		\
	}																		\
																			\
	TYPE( OBJECT_TYPE * pObject, bool alreadyIncremented ) :				\
		BASE_TYPE( pObject, alreadyIncremented )							\
	{																		\
	}																		\
																			\
	explicit TYPE( const ScriptObject & object ) : BASE_TYPE( object )		\
	{																		\
		MF_ASSERT( !object.exists() || TYPE::check( object ) );				\
	}																		\
																			\
	explicit TYPE( const PyObjectPtr & object ) : BASE_TYPE( object )		\
	{																		\
		MF_ASSERT( !object.exists() || TYPE::check( *this ) );				\
	}																		\
																			\
	TYPE( const TYPE & object ) :											\
		BASE_TYPE( (const ScriptObject &)object )							\
	{																		\
	}																		\
																			\
	TYPE & operator=( const TYPE & other )									\
	{																		\
		this->ScriptObject::operator=( other );								\
																			\
		return *this;														\
	}																		\
																			\
	TYPE & operator=( const ScriptObject & other )							\
	{																		\
		this->ScriptObject::operator=( other );								\
																			\
		return *this;														\
	}																		\
																			\

#define BASE_SCRIPT_OBJECT_IMP( OBJECT_TYPE, TYPE, BASE_TYPE )				\
	BASE_SCRIPT_OBJECT_IMP_WITHOUT_CREATE( OBJECT_TYPE, TYPE, BASE_TYPE )	\
	static TYPE create( const ScriptObject & other )						\
	{																		\
		if (other && TYPE::check( other ))									\
		{																	\
			return TYPE( other );											\
		}																	\
		return TYPE();														\
	}


#define STANDARD_SCRIPT_OBJECT_IMP( TYPE, BASE_TYPE )						\
			BASE_SCRIPT_OBJECT_IMP( PyObject, TYPE, BASE_TYPE )

#define NO_CREATE_SCRIPT_OBJECT_IMP( TYPE, BASE_TYPE )						\
			BASE_SCRIPT_OBJECT_IMP_WITHOUT_CREATE( PyObject, TYPE, BASE_TYPE )

#define SCRIPT_CONVERTER( CLASS )											\
	inline int setData( PyObject * pObj, CLASS & rScriptObject,				\
		const char * varName = "" )											\
	{																		\
		ScriptObject sc =													\
			ScriptObject( pObj, ScriptObject::FROM_BORROWED_REFERENCE );	\
																			\
		if (!sc.exists() || !CLASS::check( sc ))							\
		{																	\
			PyErr_Format( PyExc_TypeError,									\
					"%s must be set to a "#CLASS" object.", varName );		\
			return -1;														\
		}																	\
																			\
		rScriptObject = CLASS( sc );										\
		return 0;															\
	}																		\
																			\
	inline PyObject * getData( const CLASS & data )							\
	{																		\
		PyObject * ret = data ? data.get() : Py_None;						\
		Py_INCREF( ret );													\
		return ret;															\
	}

/**
 *	This class should be used as an alternative to SmartPointer<> for objects
 *	which inherit from PyObjectPlus. This provides them with the same 
 *	functionality as ScriptObject and also the same functionality as a normal
 *	SmartPointer to the object
 */
template <typename CLASS>
class ScriptObjectPtr : public ScriptObject
{
public:
	BASE_SCRIPT_OBJECT_IMP( CLASS, ScriptObjectPtr, ScriptObject );

	static bool check( const ScriptObject & object )
	{
		return CLASS::Check( object );
	}

	const CLASS * get() const
	{
		return static_cast<const CLASS*>( this->ScriptObject::get() );
	}

	CLASS * get()
	{
		return static_cast<CLASS*>( this->ScriptObject::get() );
	}

	const CLASS & operator*() const
	{
		return static_cast< const CLASS & >( this->ScriptObject::operator*() );
	}

	CLASS & operator*()
	{
		return static_cast< CLASS & >( this->ScriptObject::operator*() );
	}

	const CLASS * operator->() const
	{
		return static_cast<const CLASS*>( this->ScriptObject::operator->() );
	}

	CLASS * operator->()
	{
		return static_cast<CLASS*>( this->ScriptObject::operator->() );
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptArgs
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create arguments to be used on
 *	ScriptObject's call methods
 */
class ScriptArgs : public ScriptObject
{
public:
	NO_CREATE_SCRIPT_OBJECT_IMP( ScriptArgs, ScriptObject );

	/**
	 *	This method checks if the given object is a ScriptArgs object
	 *	@param object The object to check
	 *	@return True if object is a ScriptArgs object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyTuple_Check( object.get() );
	}

	static ScriptArgs none();

	template < typename T1 >
	static ScriptArgs create( const T1 & arg1 );

	template < typename T1, typename T2 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2 );

	template < typename T1, typename T2, typename T3 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2, 
		const T3 & arg3 );

	template < typename T1, typename T2, typename T3, typename T4 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2, 
		const T3 & arg3, const T4 & arg4 );

	template < typename T1, typename T2, typename T3, typename T4, typename T5 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2, 
		const T3 & arg3, const T4 & arg4, const T5 & arg5 );

	template < typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2, 
		const T3 & arg3, const T4 & arg4, const T5 & arg5, const T6 & arg6 );

	template < typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2, 
		const T3 & arg3, const T4 & arg4, const T5 & arg5, const T6 & arg6,
		const T7 & arg7 );

	template < typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7, typename T8 >
	static ScriptArgs create( const T1 & arg1, const T2 & arg2, 
		const T3 & arg3, const T4 & arg4, const T5 & arg5, const T6 & arg6,
		const T7 & arg7, const T8 & arg8 );
};


// -----------------------------------------------------------------------------
// Section: ScriptModule
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create, import and modify modules
 */
class ScriptModule : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptModule, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptModule object
	 *	@param object The object to check
	 *	@return True if object is a ScriptModule object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyModule_Check( object.get() );
	}

	template <class ERROR_HANDLER>
	static ScriptModule import( const char * name,
		const ERROR_HANDLER & errorHandler );

	template <class ERROR_HANDLER>
	static ScriptModule getOrCreate( const char * name,
		const ERROR_HANDLER & errorHandler );

	template <class ERROR_HANDLER>
	static ScriptModule reload( ScriptModule module,
		const ERROR_HANDLER & errorHandler );

	template <class ERROR_HANDLER>
	bool addObject( const char * name, const ScriptObject & value,
		const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER>
	bool addObject( const char * name, PyTypeObject * value,
		const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER>
	bool addIntConstant( const char * name, long value,
		const ERROR_HANDLER & errorHandler ) const;

	ScriptDict getDict() const;
};


// -----------------------------------------------------------------------------
// Section: ScriptType
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create and modify tuple objects
 */
class ScriptType : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptType, ScriptObject )

	/**
	 *	ScriptType Constructor
	 *
	 *	@param pType		The Python type to construct the ScriptType from
	 *	@param alreadyIncremented	For this value you should use the relevent
	 *		ScriptObject::FROM_NEW_REFERENCE or 
	 *		ScriptObject::FROM_BORROWED_REFERENCE based on pType
	 */
	ScriptType( PyTypeObject * pType, bool alreadyIncremented ) :
		ScriptObject( reinterpret_cast< PyObject * >( pType ), 
					  alreadyIncremented )
	{
	}

	/**
	 *	This method checks if the given object is a ScriptType object
	 *	@param object The object to check
	 *	@return True if object is a ScriptType object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyType_Check( object.get() );
	}


	/**
	 *	This method gets the name of the type
	 *	@return The name of the type
	 */
	const char * name() const
	{
		return ((PyTypeObject*)this->get())->tp_name;
	}

	/**
	 *	This method checks if the object is a given type
	 *	@param type The type object to check against
	 *	@returns True if type is the type of this object
	 */
	bool isObjectOfType( const PyObject * pObject ) const
	{
		return pObject->ob_type == ((PyTypeObject*)this->get());
	}

	/**
	 *	This method checks if the object is a given type
	 *	@param type The type object to check against
	 *	@returns True if type is the type of this object
	 */
	bool isObjectOfType( const ScriptObject & object ) const
	{
		return this->isObjectOfType( object.get() );
	}

	/**
	 *	This method gets the type of the given ScriptObject
	 */
	static ScriptType getType( const ScriptObject & object )
	{
		return ScriptType( (PyObject*)(object.get()->ob_type),
			ScriptType::FROM_BORROWED_REFERENCE );
	}

	/**
	 *	This method allocates a new object of this type
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@returns A new object of this type
	 */
	template <class ERROR_HANDLER>
	PyObject * genericAlloc( const ERROR_HANDLER & errorHandler ) const
	{
		// TODO: Need a nicer way of doing this
		PyObject * pObject = PyType_GenericAlloc( 
			(PyTypeObject*)this->get(), 0 );
		errorHandler.checkPtrError( pObject );
		return pObject;
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptDict
// -----------------------------------------------------------------------------

/**
 *	This class provides the ability to create and modify dict objects
 */
class ScriptDict : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptDict, ScriptObject )

	typedef Py_ssize_t size_type;

	/**
	 *	This method checks if the given object is a ScriptDict object
	 *	@param object The object to check
	 *	@return True if object is a ScriptDict object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyDict_Check( object.get() );
	}

	static ScriptDict create( int capacity = 0 );

	bool next( size_type & pos, 
		ScriptObject & key,
		ScriptObject & value )
	{
		PyObject * pKey = NULL;
		PyObject * pValue = NULL;

		int result = PyDict_Next( this->get(), &pos, &pKey, &pValue );

		if (result != 0)
		{
			key = ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE );
			value = ScriptObject( pValue,
				ScriptObject::FROM_BORROWED_REFERENCE );
		}
		return result != 0;
	}

	template <class ERROR_HANDLER>
	bool setItem( const char * key, const ScriptObject & value,
		const ERROR_HANDLER & errorHandler ) const;
		
	template <class ERROR_HANDLER>
	bool setItem( const ScriptObject & key, const ScriptObject & value,
		const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER>
	ScriptObject getItem( const char * key, 
		const ERROR_HANDLER & errorHandler ) const;
		
	template <class ERROR_HANDLER>
	ScriptObject getItem( const ScriptObject & key, 
		const ERROR_HANDLER & errorHandler ) const;

	size_type size() const;

	template <class ERROR_HANDLER>
	bool update( const ScriptDict & other, 
		const ERROR_HANDLER & errorHandler ) const;
};


// -----------------------------------------------------------------------------
// Section: ScriptSequence
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to use the sequence protocol on an object
 */
class ScriptSequence : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptSequence, ScriptObject )

	typedef Py_ssize_t size_type;

	/**
	 *	This method checks if the given object is a ScriptSequence object
	 *	@param object The object to check
	 *	@return True if object is a ScriptSequence object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PySequence_Check( object.get() ) != 0;
	}


	template <class ERROR_HANDLER>
	ScriptObject getItem( Py_ssize_t pos, 
		const ERROR_HANDLER & errorHandler ) const;

	template <class ERROR_HANDLER>
	bool setItem( Py_ssize_t pos, const ScriptObject & item,
		const ERROR_HANDLER & errorHandler ) const;

	size_type size() const;
};


// -----------------------------------------------------------------------------
// Section: ScriptTuple
// -----------------------------------------------------------------------------
/**
 *	This class provide the ability to create and modify tuple objects
 */
class ScriptTuple : public ScriptSequence
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptTuple, ScriptSequence )

	typedef Py_ssize_t size_type;

	/**
	 *	This method checks if the given object is a ScriptTuple object
	 *	@param object The object to check
	 *	@return True if object is a ScriptTuple object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyTuple_Check( object.get() );
	}

	static ScriptTuple create( size_type len );

	ScriptObject getItem( size_type pos ) const;

	bool setItem( size_type pos, const ScriptObject & item ) const;

	size_type size() const;
};


// -----------------------------------------------------------------------------
// Section: ScriptList
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create and modify list objects
 */
class ScriptList : public ScriptSequence
{
public:
	typedef Py_ssize_t size_type;

	STANDARD_SCRIPT_OBJECT_IMP( ScriptList, ScriptSequence )

	/**
	 *	This method checks if the given object is a ScriptList object
	 *	@param object The object to check
	 *	@return True if object is a ScriptList object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyList_Check( object.get() );
	}

	static ScriptList create( Py_ssize_t len = 0 );

	bool append( const ScriptObject & object ) const;

	ScriptObject getItem( size_type pos ) const;

	bool setItem( size_type pos, ScriptObject item ) const;

	size_type size() const;
};


// -----------------------------------------------------------------------------
// Section: ScriptInt
// -----------------------------------------------------------------------------

/**
 *	This class provides the ability to create and use 32-bit integer objects
 */
class ScriptInt : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptInt, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptInt object
	 *	@param object The object to check
	 *	@return True if object is a ScriptInt object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyInt_Check( object.get() );
	}

	/**
	 *	This method creates a ScriptInt from a string, automatically attempting
	 *	to determine the base, if the string starts with 0x the string will
	 *	be assumed to be hexidecimal, if it starts with 0 it will be assumed to
	 *	be octal, otherwise it will be treated as decimal
	 *	@param str			The string to create the integer from
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return				A new ScriptInt representing str
	 */
	template <class ERROR_HANDLER>
	static ScriptInt createFromString( const char * str,
		const ERROR_HANDLER & errorHandler )
	{
		return createFromString( str, 0, errorHandler );
	}

	/**
	 *	This method creates a ScriptInt from a string, with a specified base
	 *	@param str			The  string to create the integer from
	 *	@param errorHandler The type of error handling to use if this method
	 *	fails
	 *	@return				A new ScriptInt representing str
	 */
	template <class ERROR_HANDLER>
	static ScriptInt createFromString( const char * str, int base,
		const ERROR_HANDLER & errorHandler )
	{
		PyObject * pInt = PyInt_FromString( 
				const_cast<char*>(str), NULL, base );
		// Note: If overflow warnings supression may affect this
		errorHandler.checkPtrError( pInt );
		return ScriptInt( pInt, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method creates a new ScriptInt from a long
	 *	@param value	The value to create the ScriptInt from
	 *	@return			A new ScriptInt with the value of value
	 */
	static ScriptInt create( long value )
	{
		// Note from python manual:
		// The current implementation keeps an array of integer objects for all 
		// integers between -5 and 256, when you create an int in that range you
		// actually just get back a reference to the existing object. So it 
		// should be possible to change the value of 1. I suspect the behaviour 
		// of Python in this case is undefined. :-)
		PyObject * pInt = PyInt_FromLong( value );
		MF_ASSERT( pInt );
		return ScriptInt( pInt, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method gets the long value from the ScriptInt
	 *	@return A long representation of the ScriptInt
	 */
	long asLong() const
	{
		return PyInt_AS_LONG( this->get() );
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptLong
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create and use 64-bit integer objects
 */
class ScriptLong : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptLong, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptLong object
	 *	@param object The object to check
	 *	@return True if object is a ScriptLong object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyLong_Check( object.get() );
	}


	/**
	 *	This method creates a ScriptLong from a string, automatically attempting
	 *	to determine the base, if the string starts with 0x the string will
	 *	be assumed to be hexidecimal, if it starts with 0 it will be assumed to
	 *	be octal, otherwise it will be treated as decimal
	 *	@param str			The string to create the integer from
	 *	@param errorHandler The type of error handling to use if this method
	 *		fails
	 *	@return				A new ScriptLong representing str
	 */
	template <class ERROR_HANDLER>
	static ScriptLong createFromString( const char * str,
		const ERROR_HANDLER & errorHandler )
	{
		return createFromString( str, 0, errorHandler );
	}


	/**
	 *	This method creates a ScriptLong from a string, with a specified base
	 *	@param str			The  string to create the integer from
	 *	@param errorHandler The type of error handling to use if this method
	*		fails
	 *	@return				A new ScriptLong representing str
	 */
	template <class ERROR_HANDLER>
	static ScriptLong createFromString( const char * str, int base,
		const ERROR_HANDLER & errorHandler )
	{
		PyObject * pLong = PyLong_FromString( 
			const_cast<char*>(str), NULL, base );
		errorHandler.checkPtrError( pLong );
		return ScriptLong( pLong, ScriptObject::FROM_NEW_REFERENCE );
	}

	/**
	 *	This method creates a new ScriptLong from a unsigned int
	 *	@param value	The value to create the ScriptLong from
	 *	@return			A new ScriptLong with the value of value
	 */
	static ScriptLong create( unsigned int value )
	{
		PyObject * pLong = PyLong_FromUnsignedLong( value );
		MF_ASSERT( pLong );
		return ScriptLong( pLong, ScriptObject::FROM_NEW_REFERENCE );
	}

	/**
	 *	This method gets the long value from the ScriptLong
	 *	@return A long representation of the ScriptLong
	 */
	long asLong() const
	{
		// TODO: -1 is error?
		return PyLong_AsLong( this->get() );
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptFloat
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create and use float objects
 */
class ScriptFloat : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptFloat, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptFloat object
	 *	@param object The object to check
	 *	@return True if object is a ScriptFloat object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyFloat_Check( object.get() );
	}

	static ScriptFloat create( double value )
	{
		PyObject * pFloat = PyFloat_FromDouble( value );
		MF_ASSERT( pFloat );
		return ScriptFloat( pFloat, ScriptObject::FROM_NEW_REFERENCE );
	}

	double asDouble()
	{
		return PyFloat_AS_DOUBLE( this->get() );
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptString
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create and use string objects
 */
class ScriptString : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptString, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptString object
	 *	@param object The object to check
	 *	@return True if object is a ScriptString object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyString_Check( object.get() );
	}


	/**
	 *	This method creates a ScriptString from a null terminated char pointer
	 *	@param str		The null terminated string
	 *	@return			A ScriptString representing str
	 */
	static ScriptString create( const char * str )
	{
		PyObject * pStr = PyString_FromString( const_cast< char * >( str ) );
		MF_ASSERT( pStr );
		return ScriptString( pStr, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method creates a new ScriptString from a char pointer, and size
	 *	@param str		The string to create the ScriptString from
	 *	@param size		The size of str in bytes
	 *	@return			A ScriptString representing str
	 */
	static ScriptString create( const char * str, int size )
	{
		PyObject * pStr = PyString_FromStringAndSize( 
				const_cast< char * >( str ), size );
		MF_ASSERT( pStr );
		return ScriptString( pStr, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method creates a new ScriptString from a BW::string
	 *	@param str		The string to create the ScriptString from
	 *	@return			A Script string representing str
	 */
	template<typename Traits, typename Alloc>
	static ScriptString create( const std::basic_string<char, Traits, Alloc> & str )
	{
		PyObject * pStr = PyString_FromStringAndSize( str.c_str(), str.size() );
		MF_ASSERT( pStr );
		return ScriptString( pStr, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This gets a BW::string from the ScriptString
	 *	@param str		The place to store the BW::string
	 */
	template<typename Traits, typename Alloc>
	void getString( std::basic_string<char, Traits, Alloc> & str ) const
	{
		str.assign( PyString_AS_STRING( this->get() ),
			PyString_GET_SIZE( this->get() ) );
	}


	/**
	 *	This method gets the pointer to the string
	 *	@return		A char pointer to the string
	 */
	const char * c_str() const
	{
		return PyString_AS_STRING( this->get() );
	}
};

inline std::ostream & operator<<( std::ostream & o, const ScriptString & obj )
{
	BW::string objStr;
	obj.getString( objStr );
	return o << objStr;
}

inline std::ostream & operator<<( std::ostream & o, const ScriptObject & obj )
{
	return o << obj.str( ScriptErrorClear() );
}


// -----------------------------------------------------------------------------
// Section: ScriptBlob
// -----------------------------------------------------------------------------

typedef ScriptString ScriptBlob;

// -----------------------------------------------------------------------------
// Section: ScriptMapping
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to use the mapping protocol on objects
 */
class ScriptMapping : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptMapping, ScriptObject )

	ScriptMapping ( const ScriptDict & dict ) :
		ScriptObject( dict )
	{
	}
	/**
	 *	This method checks if the given object is a ScriptMapping object
	 *	@param object The object to check
	 *	@return True if object is a ScriptMapping object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyMapping_Check( object.get() ) == 1;
	}

	/**
	 *	This method checks if the keys exists the mapping
	 *	@param key The key to check
	 *		fails
	 *	@return	true if the mapping object has the key key and false otherwise
	 */
	bool hasKey( const char * key ) const
	{
		return PyMapping_HasKeyString( this->get(), const_cast<char *>(key) ) == 1;
	}

	/**
	 *	This method gets the keys from the mapping
	 *	@param errorHandler The type of error handling to use if this method 
	 *		fails
	 *	@return				The keys list from the mapping object
	 */
	template <class ERROR_HANDLER>
	ScriptList keys( const ERROR_HANDLER & errorHandler ) const
	{
		PyObject * pKeys = PyMapping_Keys( this->get() );
		errorHandler.checkPtrError( pKeys );
		return ScriptList( pKeys, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method gets the values from the mapping
	 *	@param errorHandler The type of error handling to use if this method 
	 *		fails
	 *	@return				The values list from the mapping object
	 */
	template <class ERROR_HANDLER>
	ScriptList values( const ERROR_HANDLER & errorHandler ) const
	{
		PyObject * pValues = PyMapping_Values( this->get() );
		errorHandler.checkPtrError( pValues );
		return ScriptList( pValues, ScriptObject::FROM_NEW_REFERENCE );
	}
	
	/**
	 *	This method gets list of the key-value pairs from the mapping
	 *	@param errorHandler The type of error handling to use if this method 
	 *		fails
	 *	@return		The list of the key-value pairs from the mapping object
	 */
	template <class ERROR_HANDLER>
	ScriptList items( const ERROR_HANDLER & errorHandler ) const
	{
		PyObject * pItems = PyMapping_Items( this->get() );
		errorHandler.checkPtrError( pItems );
		return ScriptList( pItems, ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method gets an item from a mapping
	 *	@param key			The key of the item to retrieve
	 *	@param errorHandler The type of error handling to use if this method 
	 *		fails
	 *	@return				The value of key
	 */
	template <class ERROR_HANDLER>
	ScriptObject getItem( const char * key,
		const ERROR_HANDLER & errorHandler ) const
	{
		PyObject * pItem = PyMapping_GetItemString( this->get(), 
			const_cast<char *>(key) );
		errorHandler.checkPtrError( pItem );
		return ScriptObject( pItem, ScriptObject::FROM_NEW_REFERENCE );
	}
	
	/**
	*	This method sets an item within the mapping
	*	@param key		The key to set the item of
	*	@param value	The value to set the item to
	*	@param errorHandler The type of error handling to use if this method
	*		fails
	*	@return			True if the value was successfully set, false otherwise
	*/
	template <class ERROR_HANDLER>
	bool setItem( const char * key, 
		const ScriptObject & value, const ERROR_HANDLER & errorHandler ) const
	{
		int result = PyMapping_SetItemString( this->get(),
				const_cast< char * >( key ), value.get() );
		errorHandler.checkMinusOne( result );
		return result == 0;
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptClass
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to create class objects
 */
class ScriptClass : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptClass, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptClass object
	 *	@param object The object to check
	 *	@return True if object is a ScriptClass object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyClass_Check( object.get() );
	}

	/**
	 *	This creates a new ScriptClass
	 *	@param bases		The base classes for the class
	 *	@param dict			The dict for the class
	 *	@param name			The name of the class
	 *	@param errorHandler The type of error handling to use if this method 
	 *		fails
	 *	@return				A new class
	 */
	template <class ERROR_HANDLER>
	static ScriptClass create( ScriptTuple bases, ScriptDict dict,
		ScriptString name, const ERROR_HANDLER & errorHandler )
	{
		// Unable to find docs for if new or borrowed, looks to be new
		PyObject * pClass = PyClass_New( bases.get(), dict.get(), name.get() );
		errorHandler.checkPtrError( pClass );
		return ScriptClass( pClass, ScriptObject::FROM_NEW_REFERENCE );
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptIter
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability use iterator objects
 */
class ScriptIter : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptIter, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptIter object
	 *	@param object The object to check
	 *	@return True if object is a ScriptIter object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyIter_Check( object.get() );
	}


	/**
	 *	This method gets the next item from the iterator
	 *	@return		The next item from the iterator
	 */
	ScriptObject next() const
	{
		return ScriptObject( PyIter_Next( this->get() ),
			ScriptObject::FROM_NEW_REFERENCE );
	}
};


// -----------------------------------------------------------------------------
// Section: ScriptWeakRef
// -----------------------------------------------------------------------------
/**
 *	This class provides the ability to use weakref objects
 */
class ScriptWeakRef : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptWeakRef, ScriptObject )

	/**
	 *	This method checks if the given object is a ScriptWeakRef object
	 *	@param object The object to check
	 *	@return True if object is a ScriptWeakref object, false otherwise
	 */
	static bool check( const ScriptObject & object )
	{
		return PyWeakref_Check( object.get() );
	}


	/**
	 *	This method checks if the given object is a weakref reference object
	 *	@param object The object to check
	 *	@return True if object is a weakref reference object, false otherwise
	 */
	static bool checkRef( const ScriptObject & object )
	{
		return PyWeakref_CheckRef( object.get() );
	}


	/**
	 *	This method checks if the given object is a weakref proxy object
	 *	@param object The object to check
	 *	@return True if object is a weakref proxy object, false otherwise
	 */
	static bool checkProxy( const ScriptObject & object )
	{
		return PyWeakref_CheckProxy( object.get() );
	}


	/**
	 *	This method gets gets the referent object of a proxy object
	 *	@return		The referant or ScriptObject::none if the referent no longer
	 *		exists
	 */
	ScriptObject getRefent() const
	{
		// Note: This item return Py_None if the referent no longer exists
		PyObject * pRefedObject = PyWeakref_GET_OBJECT( this->get() );
		return ScriptObject( pRefedObject, 
			ScriptObject::FROM_BORROWED_REFERENCE );
	}
};


/**
 *	This script error handler fetches the exception type, value and traceback
 *	object, and clears the error state. They can be retrieved via accessors on
 *	the object.
 */
class ScriptErrorFetch
{
public:

	/**
	 *	Constructor.
	 */
	ScriptErrorFetch() :
		exceptionType_( NULL ),
		exceptionValue_( NULL ),
		exceptionTraceback_( NULL )
	{}

	/** This method returns the fetched exception type. */
	ScriptObject type() const		{ return exceptionType_; }
	/** This method returns the fetched exception value. */
	ScriptObject value() const		{ return exceptionValue_; }
	/** This method returns the fetched exception traceback. */
	ScriptObject traceback() const	{ return exceptionTraceback_; }


	/**
	 *	This method restores and prints the fetched exception. The error
	 *	indicator is temporarily restored and cleared.
	 */
	inline void restoreAndPrint()
	{
		MF_ASSERT( !PyErr_Occurred() );
		MF_ASSERT( PyType_Check( exceptionType_.get() ) );
		MF_ASSERT( !exceptionTraceback_.exists() || 
			PyTraceBack_Check( exceptionTraceback_.get() ) );

		PyErr_Restore( exceptionType_.newRef(), 
			exceptionValue_.newRef(),
			exceptionTraceback_.newRef() );
		PyErr_Print();
	}


	/**
	 *	This method handles what happens when an error occurs
	 */
	inline void handleError() const
	{
		PyObject * pyType = NULL;
		PyObject * pyValue = NULL;
		PyObject * pyTraceback = NULL;
		PyErr_Fetch( &pyType, &pyValue, &pyTraceback );

		exceptionType_ = ScriptObject( pyType, 
			ScriptObject::FROM_NEW_REFERENCE );
		exceptionValue_ = ScriptObject( pyValue, 
			ScriptObject::FROM_NEW_REFERENCE );
		exceptionTraceback_ = ScriptObject( pyTraceback, 
			ScriptObject::FROM_NEW_REFERENCE );
	}


	/**
	 *	This method handles checking if a pointer is null, but no exception
	 *	is available
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrNoException( const void * ptr ) const {}


	/**
	 *	This method handles checking if a pointer is null.
	 *
	 *	@param ptr		The pointer to check
	 */
	inline void checkPtrError( const void * ptr ) const
	{
		if (ptr == NULL)
		{
			this->handleError();
		}
	}


	/**
	 *	This method handles checking if a result is -1.
	 *
	 *	@param result	The result to check
	 */
	inline void checkMinusOne( int result ) const
	{
		if (result == -1)
		{
			this->handleError();
		}
	}


	/**
	 *	This method handles checking if an exception has been marked as
	 *	occured.
	 */
	inline void checkErrorOccured() const
	{
		if (Script::hasError())
		{
			this->handleError();
		}
	}

private:

	// TODO: These need to be mutable because error handlers are assumed to be
	// const.
	mutable ScriptObject exceptionType_;
	mutable ScriptObject exceptionValue_;
	mutable ScriptObject exceptionTraceback_;
};


// -----------------------------------------------------------------------------
// Section: Script
// -----------------------------------------------------------------------------

namespace Script
{
SCRIPT_CONVERTER( ScriptSequence )
SCRIPT_CONVERTER( ScriptDict )
SCRIPT_CONVERTER( ScriptTuple )
SCRIPT_CONVERTER( ScriptList )
SCRIPT_CONVERTER( ScriptInt )
SCRIPT_CONVERTER( ScriptFloat )
SCRIPT_CONVERTER( ScriptLong )
SCRIPT_CONVERTER( ScriptString )
//SCRIPT_CONVERTER( ScriptBlob )
SCRIPT_CONVERTER( ScriptMapping )
SCRIPT_CONVERTER( ScriptClass )
SCRIPT_CONVERTER( ScriptModule )
SCRIPT_CONVERTER( ScriptType )
SCRIPT_CONVERTER( ScriptIter )
SCRIPT_CONVERTER( ScriptWeakRef )

inline int setData( PyObject * pObj, ScriptObject & rScriptObject,
	const char * varName = "" )
{
	rScriptObject = pObj;

	return 0;
}

}

#include "py_script_object.ipp"
#include "py_script_args.ipp"
#include "py_script_module.ipp"
#include "py_script_dict.ipp"
#include "py_script_sequence.ipp"
#include "py_script_tuple.ipp"
#include "py_script_list.ipp"


BW_END_NAMESPACE

#endif // PY_SCRIPT_OBJECT_HPP
