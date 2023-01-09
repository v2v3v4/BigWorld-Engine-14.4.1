#include "pch.hpp"
#include "worldeditor/world/entity_property_type_parser.hpp"
#include "worldeditor/world/entity_property_parser.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/editor_entity_proxy.hpp"
#include "entitydef/entity_description_map.hpp"

BW_BEGIN_NAMESPACE

/*static*/ BW::vector<EntityPropertyTypeParser::Factory *>
										EntityPropertyTypeParser::s_factories_;


// -----------------------------------------------------------------------------
// Section: Helper parser classes
// -----------------------------------------------------------------------------


/**
 *	Implementation of the INT entity property parser
 */
class EntityIntParser : public EntityPropertyTypeParser
{
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		return PyInt_Check( val );
	}


	int addEnum( PyObject* val, int index )
	{
		BW_GUARD;

		if ( val )
			index = PyInt_AsLong( val );
		return index;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		BW_GUARD;

		if ( val )
			index = val->asInt();
		return index;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		int min = std::numeric_limits< int32 >::min();
		int max = std::numeric_limits< int32 >::max();
		if ( dataType->typeName().substr(0, 4) == "INT8" )
		{
			min = std::numeric_limits< int8 >::min();
			max = std::numeric_limits< int8 >::max();
		}
		else if ( dataType->typeName().substr(0, 5) == "INT16" )
		{
			min = std::numeric_limits< int16 >::min();
			max = std::numeric_limits< int16 >::max();
		}
		else if ( dataType->typeName().substr(0, 5) == "INT32" )
		{
			min = std::numeric_limits< int32 >::min();
			max = std::numeric_limits< int32 >::max();
		}
		else
		{
			WARNING_MSG( "Unsupported integer type '%s' for property '%s'. "
						"Using INT32 in the editor.\n",
						dataType->typeName().c_str(), name );
		}

		return new GenIntProperty( name,
			new EntityIntProxy( props, propIndex, min, max ) );
	}


	GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices )
	{
		BW_GUARD;

		return new ChoiceProperty( name,
			new EntityIntProxy( props, propIndex ),
			choices );
	}
private:
	class IntFactory : public EntityPropertyTypeParser::Factory
	{
	public:
		IntFactory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 3) == "INT" )
				return new EntityIntParser();
			else
				return NULL;
		}
	};
	static IntFactory s_factory;
};
EntityIntParser::IntFactory EntityIntParser::s_factory;


/**
 *	Implementation of the UINT entity property parser
 */
class EntityUIntParser : public EntityPropertyTypeParser
{
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		return PyInt_Check( val );
	}


	int addEnum( PyObject* val, int index )
	{
		BW_GUARD;

		if ( val )
			index = PyInt_AsLong( val );
		return index;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		BW_GUARD;

		if ( val )
			index = val->asInt();
		return index;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		uint32 min = std::numeric_limits< uint32 >::min();
		uint32 max = std::numeric_limits< uint32 >::max();
		if ( dataType->typeName().substr(0, 5) == "UINT8" )
		{
			min = std::numeric_limits< uint8 >::min();
			max = std::numeric_limits< uint8 >::max();
		}
		else if ( dataType->typeName().substr(0, 6) == "UINT16" )
		{
			min = std::numeric_limits< uint16 >::min();
			max = std::numeric_limits< uint16 >::max();
		}
		else if ( dataType->typeName().substr(0, 6) == "UINT32" )
		{
			min = std::numeric_limits< uint32 >::min();
			max = std::numeric_limits< uint32 >::max();
		}
		else
		{
			WARNING_MSG( "Unsupported integer type '%s' for property '%s'. "
						"Using UINT32 in the editor.\n",
						dataType->typeName().c_str(), name );
		}

		return new GenUIntProperty( name,
			new EntityUIntProxy( props, propIndex, min, max ) );
	}


	GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices )
	{
		BW_GUARD;

		return new ChoiceProperty( name,
			new EntityUIntProxy( props, propIndex ),
			choices );
	}
private:
	class UIntFactory : public EntityPropertyTypeParser::Factory
	{
	public:
		UIntFactory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 4) == "UINT" )
				return new EntityUIntParser();
			else
				return NULL;
		}
	};
	static UIntFactory s_factory;
};
EntityUIntParser::UIntFactory EntityUIntParser::s_factory;


/**
 *	Implementation of the FLOAT entity property parser
 */
class EntityFloatParser : public EntityPropertyTypeParser
{
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		return PyFloat_Check( val );
	}


	int addEnum( PyObject* val, int index )
	{
		BW_GUARD;

		if ( val )
			enumMap_[ float(PyFloat_AsDouble( val )) ] = index;
		return index;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		BW_GUARD;

		if ( val )
			enumMap_[ val->asFloat() ] = index;
		return index;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		return new GenFloatProperty( name,
			new EntityFloatProxy( props, propIndex ) );
	}


	GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices )
	{
		BW_GUARD;

		return new ChoiceProperty( name,
			new EntityFloatEnumProxy( props, propIndex, enumMap_ ),
			choices );
	}


	GeneralProperty* radiusProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		MatrixProxy* pMP,
		uint32 widgetColour,
		float widgetRadius )
	{
		BW_GUARD;

		return new GenRadiusProperty( name,
			new EntityFloatProxy( props, propIndex ), pMP,
			widgetColour, widgetRadius );
	}

private:
	BW::map<float,int> enumMap_;

	class FloatFactory : public EntityPropertyTypeParser::Factory
	{
	public:
		FloatFactory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 5) == "FLOAT" )
				return new EntityFloatParser();
			else
				return NULL;
		}
	};
	static FloatFactory s_factory;
};
EntityFloatParser::FloatFactory EntityFloatParser::s_factory;



/**
 *	Implementation of the VECTOR2 entity property parser
 */
class EntityVector2Parser : public EntityPropertyTypeParser
{
	Vector2 asVector2( PyObject* val )
	{
		BW_GUARD;

		PyObjectPtr obj0( PySequence_GetItem( val, 0 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj1( PySequence_GetItem( val, 1 ), PyObjectPtr::STEAL_REFERENCE );

		return Vector2( (float)PyFloat_AsDouble( obj0.get() ),
			(float)PyFloat_AsDouble( obj1.get() ) );
	}
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		if (PySequence_Check( val ) != 1 ||
			PySequence_Size( val ) != 2)
		{
			return false;
		}

		PyObjectPtr obj0( PySequence_GetItem( val, 0 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj1( PySequence_GetItem( val, 1 ), PyObjectPtr::STEAL_REFERENCE );

		return PyFloat_Check( obj0.get() ) && PyFloat_Check( obj1.get() );
	}


	int addEnum( PyObject* val, int index )
	{
		return -1;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		return -1;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		return new Vector2Property( name,
			new EntityVector2Proxy( props, propIndex ) );
	}


	GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices )
	{
		BW_GUARD;

		return new ChoiceProperty( name,
			new EntityVector2EnumProxy( props, propIndex, enumMap_ ),
			choices );
	}

private:
	BW::map<Vector2,int> enumMap_;

	class Vector2Factory : public EntityPropertyTypeParser::Factory
	{
	public:
		Vector2Factory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 7) == "VECTOR2" )
				return new EntityVector2Parser();
			else
				return NULL;
		}
	};
	static Vector2Factory s_factory;
};
EntityVector2Parser::Vector2Factory EntityVector2Parser::s_factory;


/**
 *	Implementation of the VECTOR4 entity property parser
 */
class EntityVector4Parser : public EntityPropertyTypeParser
{
	Vector4 asVector4( PyObject* val )
	{
		BW_GUARD;

		PyObjectPtr obj0( PySequence_GetItem( val, 0 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj1( PySequence_GetItem( val, 1 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj2( PySequence_GetItem( val, 2 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj3( PySequence_GetItem( val, 3 ), PyObjectPtr::STEAL_REFERENCE );

		return Vector4( (float)PyFloat_AsDouble( obj0.get() ),
			(float)PyFloat_AsDouble( obj1.get() ),
			(float)PyFloat_AsDouble( obj2.get() ),
			(float)PyFloat_AsDouble( obj3.get() ) );
	}
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		if (PySequence_Check( val ) != 1 ||
			PySequence_Size( val ) != 4)
		{
			return false;
		}

		PyObjectPtr obj0( PySequence_GetItem( val, 0 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj1( PySequence_GetItem( val, 1 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj2( PySequence_GetItem( val, 2 ), PyObjectPtr::STEAL_REFERENCE );
		PyObjectPtr obj3( PySequence_GetItem( val, 3 ), PyObjectPtr::STEAL_REFERENCE );

		return PyFloat_Check( obj0.get() ) && PyFloat_Check( obj1.get() )
			&& PyFloat_Check( obj2.get() ) && PyFloat_Check( obj3.get() );
	}


	int addEnum( PyObject* val, int index )
	{
		return -1;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		return -1;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		return new Vector4Property( name,
			new EntityVector4Proxy( props, propIndex ) );
	}


	GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices )
	{
		BW_GUARD;

		return new ChoiceProperty( name,
			new EntityVector4EnumProxy( props, propIndex, enumMap_ ),
			choices );
	}

private:
	BW::map<Vector4,int> enumMap_;

	class Vector4Factory : public EntityPropertyTypeParser::Factory
	{
	public:
		Vector4Factory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 7) == "VECTOR4" )
				return new EntityVector4Parser();
			else
				return NULL;
		}
	};
	static Vector4Factory s_factory;
};
EntityVector4Parser::Vector4Factory EntityVector4Parser::s_factory;


/**
 *	Implementation of the STRING entity property parser
 */
class EntityStringParser : public EntityPropertyTypeParser
{
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		return PyString_Check( val );
	}


	int addEnum( PyObject* val, int index )
	{
		BW_GUARD;

		if ( val )
			enumMap_[ PyString_AsString( val ) ] = index;
		return index;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		BW_GUARD;

		if ( val )
			enumMap_[ val->asString() ] = index;
		return index;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		return new TextProperty( name,
			new EntityStringProxy( props, propIndex ) );
	}


	GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices )
	{
		BW_GUARD;

		return new ChoiceProperty( name,
			new EntityStringEnumProxy( props, propIndex, enumMap_ ),
			choices );
	}

private:
	BW::map<BW::string,int> enumMap_;
	class StringFactory : public EntityPropertyTypeParser::Factory
	{
	public:
		StringFactory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 6) == "STRING" )
				return new EntityStringParser();
			else
				return NULL;
		}
	};
	static StringFactory s_factory;
};
EntityStringParser::StringFactory EntityStringParser::s_factory;


/**
 *	Implementation of the ARRAY entity property parser
 */
class EntityArrayParser : public EntityPropertyTypeParser
{
public:
	bool checkVal( PyObject* val )
	{
		BW_GUARD;

		return PySequence_Check( val ) == 1;
	}


	int addEnum( PyObject* val, int index )
	{
		// Not supported in arrays
		return -1;
	}


	int addEnum( DataSectionPtr val, int index )
	{
		// Not supported in arrays
		return -1;
	}


	GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType )
	{
		BW_GUARD;

		return new ArrayProperty( name,
			new EntityArrayProxy(
				props, dataType, propIndex ) );
	}


private:
	class ArrayFactory : public EntityPropertyTypeParser::Factory
	{
	public:
		ArrayFactory()
		{
			BW_GUARD;

			EntityPropertyTypeParser::registerFactory( this );
		}

		EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType )
		{
			BW_GUARD;

			if ( dataType->typeName().substr(0, 5) == "ARRAY" )
				return new EntityArrayParser();
			else
				return NULL;
		}
	};
	static ArrayFactory s_factory;
};
EntityArrayParser::ArrayFactory EntityArrayParser::s_factory;


// -----------------------------------------------------------------------------
// Section: EntityPropertyTypeParser
// -----------------------------------------------------------------------------


/**
 *	Default implementation of the enum property, which prints an error
 *	and returns the result of calling the parser's plainProperty.
 */
GeneralProperty* EntityPropertyTypeParser::enumProperty(
	BasePropertiesHelper* props,
	int propIndex,
	const Name& name,
	DataTypePtr dataType,
	DataSectionPtr choices )
{
	BW_GUARD;

	ERROR_MSG(
		"'%s': The ENUM widget is not supported in the '%s' data type\n",
		props->pItem()->edDescription().c_str(), dataType->typeName().c_str() );
	return plainProperty( props, propIndex, name, dataType );
}


/**
 *	Default implementation of the radius property, which prints an error
 *	and returns the result of calling the parser's plainProperty.
 */
GeneralProperty* EntityPropertyTypeParser::radiusProperty(
	BasePropertiesHelper* props,
	int propIndex,
	const Name& name,
	DataTypePtr dataType,
	MatrixProxy* pMP,
	uint32 widgetColour,
	float widgetRadius )
{
	BW_GUARD;

	ERROR_MSG(
		"'%s': The RADIUS widget is not supported in the '%s' data type\n",
		props->pItem()->edDescription().c_str(), dataType->typeName().c_str() );
	return plainProperty( props, propIndex, name, dataType );
}


/**
 *	Static method that creates the approriate parser for the DataDescription
 *  passed in, and returns it.
 *
 *	@param pDD		Entity property data description
 *	@return			Appropriate parser for pDD
 */
/*static*/ EntityPropertyTypeParserPtr EntityPropertyTypeParser::create(
	const Name& name, DataTypePtr dataType )
{
	BW_GUARD;

	EntityPropertyTypeParserPtr result;
	for ( BW::vector<Factory *>::iterator i = s_factories_.begin();
		i != s_factories_.end(); ++i )
	{
		result = (*i)->create( name, dataType );
		if ( result )
			break;
	}
	return result;
}


/**
 *	Static method that registers a parser factory.
 *
 *	@param factory		Factory to add to the list
 */
/*static*/ void EntityPropertyTypeParser::registerFactory( Factory * factory )
{
	BW_GUARD;

	if ( factory )
		s_factories_.push_back( factory );
}
BW_END_NAMESPACE

