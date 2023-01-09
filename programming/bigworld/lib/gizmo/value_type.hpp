#ifndef VALUE_TYPE_HPP
#define VALUE_TYPE_HPP


#include "cstdmf/debug.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/matrix.hpp"
#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

namespace ValueTypeDesc
{
	/**
	 *	This enumerates the available value types.
	 */
	enum Desc
	{
		UNKNOWN,
		BOOL,
		INT,
		FLOAT,
		STRING,
		VECTOR,
		MATRIX,
		COLOUR,
		FILEPATH,
		DATE_STRING
	};
}


/**
 *	This class defines the interface for a basic value type.
 */
class BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const = 0;

	virtual const char * sectionName() const = 0;

	virtual bool toString( bool v, BW::string & ret ) const { return badType(); }
	virtual bool toString( int v, BW::string & ret ) const { return badType(); }
	virtual bool toString( float v, BW::string & ret ) const { return badType(); }
	virtual bool toString( const BW::string & v, BW::string & ret ) const { return badType(); }
	virtual bool toString( const Vector2 & v, BW::string & ret ) const { return badType(); }
	virtual bool toString( const Vector3 & v, BW::string & ret ) const { return badType(); }
	virtual bool toString( const Vector4 & v, BW::string & ret ) const { return badType(); }
	virtual bool toString( const Matrix & v, BW::string & ret ) const { return badType(); }
	virtual bool toString( PyObject * v, BW::string & ret ) const { return badType(); }

	virtual bool fromString( const BW::string & v, bool & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, int & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, float & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, BW::string & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, Vector2 & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, Vector3 & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, Vector4 & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, Matrix & ret ) const { return badType(); }
	virtual bool fromString( const BW::string & v, PyObject * & ret ) const { return badType(); }

private:
	bool badType() const
	{
		ERROR_MSG( "Failed processing ValueType.\n" );
		return false;
	}
};


/**
 *	This class serves as a wrapper for BaseValueType-derived classes in order
 *	to simplify use of ValueType instances.
 */
class ValueType
{
public:
	ValueType();
	ValueType( const ValueType & other );
	ValueType( ValueTypeDesc::Desc desc );

	ValueTypeDesc::Desc desc() const { return actualType_->desc(); }

	bool isNumeric() const
	{
		ValueTypeDesc::Desc desc = actualType_->desc();
		return desc == ValueTypeDesc::INT || desc == ValueTypeDesc::FLOAT;
	}

	bool isValid() const { return actualType_->desc() != ValueTypeDesc::UNKNOWN; }

	bool toString( bool v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( int v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( float v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( const BW::string & v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( const Vector2 & v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( const Vector3 & v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( const Vector4 & v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( const Matrix & v, BW::string & ret ) const { return actualType_->toString( v, ret ); }
	bool toString( PyObject * v, BW::string & ret ) const { return actualType_->toString( v, ret ); }

	bool fromString( const BW::string & v, bool & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, int & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, float & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, BW::string & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, Vector2 & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, Vector3 & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, Vector4 & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, Matrix & ret ) const { return actualType_->fromString( v, ret ); }
	bool fromString( const BW::string & v, PyObject * & ret ) const { return actualType_->fromString( v, ret ); }

	bool operator==( const ValueType & other ) const
		{ return this->actualType_->desc() == other.actualType_->desc(); }

	BW::string toSectionName() const { return actualType_->sectionName(); }
	static ValueType fromSectionName( const BW::string & sectionName );

private:
	BaseValueType * actualType_;
};


BW_END_NAMESPACE
#endif // VALUE_TYPE_HPP
