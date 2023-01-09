#ifndef OC_SCRIPT_OBJECT_HPP
#define OC_SCRIPT_OBJECT_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class ScriptObject
{
public:
	static const bool STEAL_REFERENCE = true;
	static const bool NEW_REFERENCE = false;

	ScriptObject( void * pObject = 0, bool alreadyIncremented = false );
	ScriptObject( const ScriptObject & other );
	~ScriptObject();

	ScriptObject & operator=( const ScriptObject & other );

	static ScriptObject none();
	bool isNone() const;

	int refCount() const;

	void * get() const { return pNSObj_; }

	// PyObject * newRef();

	typedef void * ScriptObject::*unspecified_bool_type;

	operator unspecified_bool_type() const
	{
		return (pNSObj_ == 0) ? 0 : &ScriptObject::pNSObj_;
	}

	const char * typeNameOfObject() const;

	int compareTo( const ScriptObject & other ) const;

	template <class TYPE>
	inline static ScriptObject createFrom( TYPE val );

	template <class TYPE>
	bool convertTo( TYPE & rVal, const char * varName = "" )
	{
		return convertScriptObjectTo( *this, rVal );
	}

protected:
	void * pNSObj_;
};


// -----------------------------------------------------------------------------
// Section: operators
// -----------------------------------------------------------------------------

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

#define STANDARD_SCRIPT_OBJECT_IMP( TYPE, BASE_TYPE )						\
	TYPE( void * pObject = 0, bool alreadyIncremented = false ) :			\
		BASE_TYPE( pObject, alreadyIncremented )							\
	{																		\
	}																		\
																			\
	TYPE( const ScriptObject & object ) : BASE_TYPE( object )				\
	{																		\
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


// -----------------------------------------------------------------------------
// Section: ScriptDict
// -----------------------------------------------------------------------------

/**
 *
 */
class ScriptDict : public ScriptObject
{
public:

	STANDARD_SCRIPT_OBJECT_IMP( ScriptDict, ScriptObject )

	static bool check( const ScriptObject & object );

	static ScriptDict create( int capacity = 0 );

	bool setItemString( const char * key, ScriptObject sObj );
	ScriptObject getItemString( const char * key ) const;

	int size() const;
};


// -----------------------------------------------------------------------------
// Section: ScriptSequence
// -----------------------------------------------------------------------------

class ScriptSequence : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptSequence, ScriptObject )

	int size() const;

	ScriptObject getItem( int pos ) const;
	bool setItem( int pos, ScriptObject item );

	static bool check( const ScriptObject & object );
};


// -----------------------------------------------------------------------------
// Section: ScriptTuple
// -----------------------------------------------------------------------------

class ScriptTuple : public ScriptSequence
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptTuple, ScriptSequence )

	static ScriptTuple create( int len );
};


// -----------------------------------------------------------------------------
// Section: ScriptList
// -----------------------------------------------------------------------------

/**
 *
 */
class ScriptList : public ScriptTuple
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptList, ScriptTuple )

	static bool check( ScriptObject object );

	static ScriptList create( int len = 0 );

	bool append( ScriptObject object );
};


// -----------------------------------------------------------------------------
// Section: ScriptString
// -----------------------------------------------------------------------------

class ScriptString : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptString, ScriptObject )

	static bool check( ScriptObject object );
};


// -----------------------------------------------------------------------------
// Section: ScriptBlob
// -----------------------------------------------------------------------------

class ScriptBlob : public ScriptObject
{
public:
	STANDARD_SCRIPT_OBJECT_IMP( ScriptBlob, ScriptObject )

	static bool check( const ScriptObject & object );

	static ScriptBlob create( const BW::string & blobStr );

	BW::string asString() const;
};


// -----------------------------------------------------------------------------
// Section:
// -----------------------------------------------------------------------------

namespace Script
{

inline void printError()
{
	// TODO: ObjC
}

inline void clearError()
{
	// TODO: ObjC
}

inline bool hasError()
{
	// TODO: ObjC
	return false;
}

}

BW_END_NAMESPACE
#include "cstdmf/bw_string.hpp"
BW_BEGIN_NAMESPACE

class Vector2;
class Vector3;
class Vector4;

ScriptObject createScriptObjectFrom( double value );
ScriptObject createScriptObjectFrom( float value );
ScriptObject createScriptObjectFrom( int value );
ScriptObject createScriptObjectFrom( unsigned int value );
ScriptObject createScriptObjectFrom( long value );
ScriptObject createScriptObjectFrom( unsigned long value );
ScriptObject createScriptObjectFrom( long long value );
ScriptObject createScriptObjectFrom( unsigned long long value );
ScriptObject createScriptObjectFrom( const BW::string & value );
ScriptObject createScriptObjectFrom( const char * value );
ScriptObject createScriptObjectFrom( const Vector2 & value );
ScriptObject createScriptObjectFrom( const Vector3 & value );
ScriptObject createScriptObjectFrom( const Vector4 & value );

bool convertScriptObjectTo( ScriptObject object, double & rValue );
bool convertScriptObjectTo( ScriptObject object, float & rValue );
bool convertScriptObjectTo( ScriptObject object, int & rValue );
bool convertScriptObjectTo( ScriptObject object, unsigned int & rValue );
bool convertScriptObjectTo( ScriptObject object, long & rValue );
bool convertScriptObjectTo( ScriptObject object, unsigned long & rValue );
bool convertScriptObjectTo( ScriptObject object, long long & rValue );
bool convertScriptObjectTo( ScriptObject object, unsigned long long & rValue );
bool convertScriptObjectTo( ScriptObject object, BW::string & rValue );
bool convertScriptObjectTo( ScriptObject object, Vector2 & rValue );
bool convertScriptObjectTo( ScriptObject object, Vector3 & rValue );
bool convertScriptObjectTo( ScriptObject object, Vector4 & rValue );

template <class TYPE>
ScriptObject ScriptObject::createFrom( TYPE val )
{
    return createScriptObjectFrom( val );
}

BW_END_NAMESPACE

#endif
