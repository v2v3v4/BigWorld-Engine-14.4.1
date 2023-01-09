
#include "pch.hpp"
#include "value_type.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

namespace
{

const int STRBUFSIZE = 4096;


///////////////////////////////////////////////////////////////////////////////
// Section: EmptyValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a special empty value type object.
 */
class EmptyValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::UNKNOWN; }
	virtual const char * sectionName() const { return ""; }
};


///////////////////////////////////////////////////////////////////////////////
// Section: BoolValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a bool value type object.
 */
class BoolValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::BOOL; }
	virtual const char * sectionName() const { return "BOOL"; }

	virtual bool toString( bool v, BW::string & ret ) const
	{
		BW_GUARD;

		ret = (v ? "1" : "0");
		return true;
	}

	virtual bool toString( PyObject * v, BW::string & ret ) const
	{
		BW_GUARD;

		bool boolVal;
		if (Script::setData( v, boolVal ) == 0)
		{
			return toString( boolVal, ret );
		}
		PyErr_Clear();
		return false;
	}

	virtual bool fromString( const BW::string & v, bool & ret ) const
	{
		BW_GUARD;

		ret = (v == "0" ? false : true);
		return true;
	}

	virtual bool fromString( const BW::string & v, PyObject * & ret ) const
	{
		BW_GUARD;

		bool boolVal;
		fromString( v, boolVal );
		ret = Script::getData( boolVal );
		return ret != NULL;
	}
};


///////////////////////////////////////////////////////////////////////////////
// Section: IntValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a integer value type object.
 */
class IntValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::INT; }
	virtual const char * sectionName() const { return "INT"; }

	virtual bool toString( int v, BW::string & ret ) const
	{
		BW_GUARD;

		char buffer[ STRBUFSIZE ];
		bw_snprintf( buffer, STRBUFSIZE, "%d", v );
		ret = buffer;
		return true;
	}

	virtual bool toString( PyObject * v, BW::string & ret ) const
	{
		BW_GUARD;

		int intVal;
		if (Script::setData( v, intVal ) == 0)
		{
			return toString( intVal, ret );
		}
		PyErr_Clear();
		return false;
	}

	// TODO: Implement the corresponding "fromString" methods
};


///////////////////////////////////////////////////////////////////////////////
// Section: FloatValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a floating-point value type object.
 */
class FloatValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::FLOAT; }
	virtual const char * sectionName() const { return "FLOAT"; }

	virtual bool toString( float v, BW::string & ret ) const
	{
		BW_GUARD;

		char buffer[ STRBUFSIZE ];
		bw_snprintf( buffer, STRBUFSIZE, "%f", v );
		ret = buffer;
		return true;
	}

	virtual bool toString( PyObject * v, BW::string & ret ) const
	{
		BW_GUARD;

		float floatVal;
		if (Script::setData( v, floatVal ) == 0)
		{
			return toString( floatVal, ret );
		}
		PyErr_Clear();
		return false;
	}

	// TODO: Implement the corresponding "fromString" methods
};


///////////////////////////////////////////////////////////////////////////////
// Section: VectorValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a vector value type object.
 */
class VectorValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::VECTOR; }
	virtual const char * sectionName() const { return "VECTOR"; }

	virtual bool toString( const Vector2 & v, BW::string & ret ) const
	{
		BW_GUARD;

		char buffer[ STRBUFSIZE ];
		bw_snprintf( buffer, STRBUFSIZE, "%.2f, %.2f", v.x, v.y );
		ret = buffer;
		return true;
	}

	virtual bool toString( const Vector3 & v, BW::string & ret ) const
	{
		BW_GUARD;

		char buffer[ STRBUFSIZE ];
		bw_snprintf( buffer, STRBUFSIZE, "%.2f, %.2f, %.2f",
															v.x, v.y, v.z );
		ret = buffer;
		return true;
	}

	virtual bool toString( const Vector4 & v, BW::string & ret ) const
	{
		BW_GUARD;

		char buffer[ STRBUFSIZE ];
		bw_snprintf( buffer, STRBUFSIZE, "%.2f, %.2f, %.2f, %.2f",
														v.x, v.y, v.z, v.w );
		ret = buffer;
		return true;
	}

	virtual bool toString( PyObject * v, BW::string & ret ) const
	{
		BW_GUARD;

		Vector2 vec2Val;
		Vector3 vec3Val;
		Vector4 vec4Val;
		bool result = false;
		if (Script::setData( v, vec4Val ) == 0)
		{
			result = toString( vec4Val, ret );
		}
		else if (Script::setData( v, vec3Val ) == 0)
		{
			result = toString( vec3Val, ret );
		}
		else if (Script::setData( v, vec2Val ) == 0)
		{
			result = toString( vec2Val, ret );
		}

		PyErr_Clear();

		return result;
	}

	// TODO: Implement the corresponding "fromString" methods
};


///////////////////////////////////////////////////////////////////////////////
// Section: MatrixValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a matrix value type object.
 */
class MatrixValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::MATRIX; }
	virtual const char * sectionName() const { return "MATRIX"; }

	virtual bool toString( const Matrix & v, BW::string & ret ) const
	{
		BW_GUARD;

		char buffer[ STRBUFSIZE ];
		bw_snprintf( buffer, STRBUFSIZE,
						"[%.2f, %.2f, %.2f, %.2f],"
						"[%.2f, %.2f, %.2f, %.2f],"
						"[%.2f, %.2f, %.2f, %.2f],"
						"[%.2f, %.2f, %.2f, %.2f]",
						v[0][0], v[0][1], v[0][2], v[0][3],
						v[1][0], v[1][1], v[1][2], v[1][3],
						v[2][0], v[2][1], v[2][2], v[2][3],
						v[3][0], v[3][1], v[3][2], v[3][3] );
		ret += buffer;
		return true;
	}

	virtual bool toString( PyObject * v, BW::string & ret ) const
	{
		BW_GUARD;

		Matrix matrixVal;
		if (Script::setData( v, matrixVal ) == 0)
		{
			return toString( matrixVal, ret );
		}
		PyErr_Clear();
		return false;
	}

	// TODO: Implement the corresponding "fromString" methods
};


///////////////////////////////////////////////////////////////////////////////
// Section: ColourValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a colour value type object.
 */
class ColourValueType : public VectorValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::COLOUR; }
	virtual const char * sectionName() const { return "COLOUR"; }
};


///////////////////////////////////////////////////////////////////////////////
// Section: StringValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a string value type object.
 */
class StringValueType : public BaseValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::STRING; }
	virtual const char * sectionName() const { return "STRING"; }

	virtual bool toString( const BW::string & v, BW::string & ret ) const
	{
		BW_GUARD;

		ret = v;
		return true;
	}

	virtual bool toString( PyObject * v, BW::string & ret ) const
	{
		BW_GUARD;

		if (Script::setData( v, ret ) == 0)
		{
			return true;
		}
		PyErr_Clear();
		return false;
	}

	virtual bool fromString( const BW::string & v, PyObject * & ret ) const
	{
		BW_GUARD;

		ret = Script::getData( v );
		return ret != NULL;
	}

	// TODO: Implement the corresponding "fromString" methods
};


///////////////////////////////////////////////////////////////////////////////
// Section: FilePathValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a file path string value type object.
 */
class FilePathValueType : public StringValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::FILEPATH; }
	virtual const char * sectionName() const { return "FILEPATH"; }
};


///////////////////////////////////////////////////////////////////////////////
// Section: DateStringValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class implements a date string value type object.
 */
class DateStringValueType : public StringValueType
{
public:
	virtual ValueTypeDesc::Desc desc() const { return ValueTypeDesc::DATE_STRING; }
	virtual const char * sectionName() const { return "DATE_STRING"; }
};


///////////////////////////////////////////////////////////////////////////////
// Section: ValueTypesHolder
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class manages the supported value types.
 */
class ValueTypesHolder
{
public:
	typedef BW::map< ValueTypeDesc::Desc, BaseValueType * > TypeMap;

	/**
	 *	Constructor
	 */
	ValueTypesHolder()
	{
		BW_GUARD;

		valueTypes_[ ValueTypeDesc::UNKNOWN ] = &invalid_;
		valueTypes_[ ValueTypeDesc::BOOL ] = &bool_;
		valueTypes_[ ValueTypeDesc::INT ] = &int_;
		valueTypes_[ ValueTypeDesc::FLOAT ] = &float_;
		valueTypes_[ ValueTypeDesc::STRING ] = &string_;
		valueTypes_[ ValueTypeDesc::VECTOR ] = &vector_;
		valueTypes_[ ValueTypeDesc::MATRIX ] = &matrix_;
		valueTypes_[ ValueTypeDesc::COLOUR ] = &colour_;
		valueTypes_[ ValueTypeDesc::FILEPATH ] = &path_;
		valueTypes_[ ValueTypeDesc::DATE_STRING ] = &dateStr_;
	}


	/**
	 *	This method returns a value type from a description enum.
	 */
	BaseValueType * valueType( ValueTypeDesc::Desc desc ) const
	{
		BW_GUARD;

		TypeMap::const_iterator it = valueTypes_.find( desc );
		if (it != valueTypes_.end())
		{
			return (*it).second;
		}
		else
		{
			return &invalid_;
		}
	}

	/**
	 *	This method returns a value type from a data section name.
	 */
	BaseValueType * valueType( const BW::string & sectionName ) const
	{
		BW_GUARD;

		for (TypeMap::const_iterator it = valueTypes_.begin();
			it != valueTypes_.end(); ++it)
		{
			if ((*it).second->sectionName() == sectionName)
			{
				return (*it).second;
			}
		}
		return &invalid_;
	}

private:
	TypeMap valueTypes_;
	mutable EmptyValueType invalid_;
	BoolValueType bool_;
	IntValueType int_;
	FloatValueType float_;
	StringValueType string_;
	VectorValueType vector_;
	MatrixValueType matrix_;
	ColourValueType colour_;
	FilePathValueType path_;
	DateStringValueType dateStr_;
};

// This global class helps creating ValueType objects
ValueTypesHolder s_valueTypesHolder;


} // anonymous namespace


///////////////////////////////////////////////////////////////////////////////
// Section: ValueType
///////////////////////////////////////////////////////////////////////////////

/**
 *	Default constructor.
 */
ValueType::ValueType() :
	actualType_( NULL )
{
	BW_GUARD;

	actualType_ = s_valueTypesHolder.valueType( ValueTypeDesc::UNKNOWN );
}


/**
 *	Copy constructor.
 */
ValueType::ValueType( const ValueType & other ) :
	actualType_( NULL )
{
	BW_GUARD;

	actualType_ = other.actualType_;
}


/**
 *	Value type description constructor.
 */
ValueType::ValueType( ValueTypeDesc::Desc desc ) :
	actualType_( NULL )
{
	BW_GUARD;

	actualType_ = s_valueTypesHolder.valueType( desc );
}


/**
 *	This method returns the ValueType that corresponds to a data section name.
 */
/*static*/
ValueType ValueType::fromSectionName( const BW::string & sectionName )
{
	BW_GUARD;

	return s_valueTypesHolder.valueType( sectionName )->desc();
}
BW_END_NAMESPACE

