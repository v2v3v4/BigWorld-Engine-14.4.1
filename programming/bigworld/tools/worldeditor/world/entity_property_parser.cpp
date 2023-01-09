#include "pch.hpp"
#include "worldeditor/world/entity_property_parser.hpp"
#include "worldeditor/world/entity_property_type_parser.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "common/base_properties_helper.hpp"
#include "entitydef/base_user_data_object_description.hpp"
#include "pyscript/script.hpp"
#include "resmgr/xml_section.hpp"

BW_BEGIN_NAMESPACE

/*static*/ BW::vector<EntityPropertyParser::FactoryPtr>
	EntityPropertyParser::s_factories_;


// -----------------------------------------------------------------------------
// Section: Helper widget parser classes
// -----------------------------------------------------------------------------

/**
 *	Implementation of the plain entity property parser (no widgets)
 */
class EntityPlainParser : public EntityPropertyParser
{
public:
	virtual GeneralProperty* createProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr widget,
		MatrixProxy* pMP )
	{
		BW_GUARD;

		EntityPropertyTypeParserPtr parser = EntityPropertyTypeParser::create( name, dataType );
		if ( parser )
			return parser->plainProperty( props, propIndex, name, dataType );
		else
			return NULL;
	}
};


/**
 *	Implementation of the ENUM entity property widget parser
 */
class EntityEnumWidget : public EntityPropertyParser
{
public:
	EntityEnumWidget( PyObject* pyEnum ) :
		pyEnum_( pyEnum )
	{
	}

	virtual GeneralProperty* createProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr widget,
		MatrixProxy* pMP )
	{
		BW_GUARD;

		EntityPropertyTypeParserPtr parser = EntityPropertyTypeParser::create( name, dataType );
		if ( parser == NULL )
			return NULL;

		if ( pyEnum_ != NULL )
		{
			DataSectionPtr choices( new XMLSection( "ENUM_choices" ) );
			bool failed = false;

			if( PySequence_Check( pyEnum_ ) )
			{
				Py_ssize_t nItems = PySequence_Size( pyEnum_ );
				MF_ASSERT( nItems <= INT_MAX );
				for (int j = 0; j < ( int ) nItems; j++)
				{
					PyObject* item = PySequence_GetItem( pyEnum_, j );
					if( PyTuple_Check( item ) )
					{
						PyObject* valueObj = PyTuple_GetItem( item, 0 );
						PyObject* nameObj = PyTuple_GetItem( item, 1 );
						if( valueObj && nameObj && parser->checkVal( valueObj ) && PyString_Check( nameObj ) )
						{
							BW::string itemName = PyString_AsString( nameObj );
							Py_ssize_t index = parser->addEnum( valueObj, j );
							MF_ASSERT( index <= INT_MAX );
							choices->newSection( itemName )->setInt( ( int ) index );
						}
						else
						{
							failed = true;
							Py_DECREF( item );
							break;
						}
					}
					else
						failed = true;
					Py_DECREF( item );
				}
			}
			else
				failed = true;

			Py_XDECREF( pyEnum_ );

			if( failed )
			{
				WorldManager::instance().addError( NULL, NULL,
					LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/BAD_ENUM",
					props->pItem()->edDescription().c_str(), name.c_str() ).c_str() );
				PyErr_Print();
				return NULL;
			}
			else
			{
				return parser->enumProperty( props, propIndex, name, dataType, choices );
			}
		}
		else
		{
			// must be an XML-based ENUM
			if ( !widget || widget->asString() != "ENUM" )
				return NULL;

			DataSectionPtr choices( new XMLSection( "ENUM_choices" ) );

			BW::vector<DataSectionPtr> sections;
			widget->openSections( "enum", sections );
			if ( sections.empty() )
				return NULL;

			int j = 0;
			for( BW::vector<DataSectionPtr>::iterator it = sections.begin();
				it != sections.end(); ++it)
			{
				int index = parser->addEnum( (*it)->openSection( "value" ), j++ );
				choices->newSection( (*it)->readString( "display" ) )->setInt( index );
			}

			return parser->enumProperty( props, propIndex, name, dataType, choices );
		}
	}

private:
	PyObject* pyEnum_;

	class EnumFactory : public EntityPropertyParser::Factory
	{
	public:

		EnumFactory()
		{
			BW_GUARD;

			EntityPropertyParser::registerFactory( this );
		}

		EntityPropertyParserPtr create( PyObject* pyClass,
			const Name& name, DataTypePtr dataType, DataSectionPtr widget )
		{
			BW_GUARD;

			// check for python enums first
			PyObject* pyEnum = NULL;
			if( pyClass )
			{
				BW::string fnName = "getEnums_" + name.str();
				PyObject * fnPtr = PyObject_GetAttrString( pyClass, const_cast<char *>(fnName.c_str()) );
				if( fnPtr )
				{
					pyEnum = Script::ask(
							fnPtr,
							PyTuple_New( 0 ),
							"EditorChunkEntity::edEdit: " );
				}
				else
				{
					PyErr_Clear();
				}
			}

			if ( pyEnum || (widget && widget->asString() == "ENUM") )
				return new EntityEnumWidget( pyEnum );
			else
				return NULL;
		}
	};
	typedef SmartPointer<EnumFactory> EnumFactoryPtr;
	static EnumFactoryPtr s_factory;
};
EntityEnumWidget::EnumFactoryPtr EntityEnumWidget::s_factory =
	new EntityEnumWidget::EnumFactory;


/**
 *	Implementation of the RADIUS entity property widget parser
 */
class EntityRadiusWidget : public EntityPropertyParser
{
public:
	virtual GeneralProperty* createProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr widget,
		MatrixProxy* pMP )
	{
		BW_GUARD;

		EntityPropertyTypeParserPtr parser = EntityPropertyTypeParser::create( name, dataType );
		if ( parser == NULL )
			return NULL;

		if ( !widget || widget->asString() != "RADIUS" )
			return NULL;

		// default colour is red with 255 with 192 of alpha
		Vector4 colour = widget->readVector4( "colour", Vector4( 255, 0, 0, 192 ) );
		uint32 radiusColour =
			(((uint8)colour[0]) << 16) |
			(((uint8)colour[1]) << 8) |
			(((uint8)colour[2])) |
			(((uint8)colour[3]) << 24);
		float radius = widget->readFloat( "gizmoRadius", 2.0f );
		return parser->radiusProperty(
			props, propIndex, name, dataType, pMP, radiusColour, radius );
	}

private:
	class EnumFactory : public EntityPropertyParser::Factory
	{
	public:
		EnumFactory()
		{
			BW_GUARD;

			EntityPropertyParser::registerFactory( this );
		}

		EntityPropertyParserPtr create( PyObject* pyClass,
			const Name& name, DataTypePtr dataType, DataSectionPtr widget )
		{
			BW_GUARD;

			if ( widget && widget->asString() == "RADIUS" )
				return new EntityRadiusWidget();
			else
				return NULL;
		}
	};
	typedef SmartPointer<EnumFactory> EnumFactoryPtr;
	static EnumFactoryPtr s_factory;
};
EntityRadiusWidget::EnumFactoryPtr EntityRadiusWidget::s_factory =
	new EntityRadiusWidget::EnumFactory;


// -----------------------------------------------------------------------------
// Section: EntityPropertyParser class
// -----------------------------------------------------------------------------


/**
 *	Static method that creates the approriate parser for the DataDescription
 *  passed in, and returns it.
 *
 *	@param pDD		Entity property data description
 *	@return			Appropriate parser for pDD
 */
/*static*/ EntityPropertyParserPtr EntityPropertyParser::create(
	PyObject* pyClass, const Name& name, DataTypePtr dataType, DataSectionPtr widget )
{
	BW_GUARD;

	EntityPropertyParserPtr result;
	for ( BW::vector<FactoryPtr>::iterator i = s_factories_.begin();
		i != s_factories_.end(); ++i )
	{
		result = (*i)->create( pyClass, name, dataType, widget );
		if ( result )
			break;
	}
	if ( result == NULL )
	{
		result = new EntityPlainParser();
	}

	return result;
}


/**
 *	Static method that registers a parser factory.
 *
 *	@param factory		Factory to add to the list
 */
/*static*/ void EntityPropertyParser::registerFactory( FactoryPtr factory )
{
	BW_GUARD;

	if ( factory )
		s_factories_.push_back( factory );
}

BW_END_NAMESPACE

