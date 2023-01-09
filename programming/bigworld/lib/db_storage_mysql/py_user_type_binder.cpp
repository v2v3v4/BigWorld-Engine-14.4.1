#include "script/first_include.hpp"

#include "py_user_type_binder.hpp"

#include "entitydef/data_description.hpp"

#include "mappings/composite_property_mapping.hpp"
#include "mappings/sequence_mapping.hpp"
#include "mappings/user_type_mapping.hpp"

#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
PyUserTypeBinder::PyUserTypeBinder( const Namer & namer,
			const BW::string & propName,
			DataSectionPtr pDefaultValue, PyTypeObject * pType ) :
		PyObjectPlus( pType )
{
	// Don't add extra naming level if user prop is unnamed
	// i.e. inside a sequence.
	if (propName.empty())
	{
		tables_.push( Context( new UserTypeMapping( propName ),
							namer, pDefaultValue ) );
	}
	else
	{
		tables_.push( Context( new UserTypeMapping( propName ),
							namer, propName, false,
							pDefaultValue ) );
	}
}


/**
 * This method binds an attribute in a USER_TYPE to a BigWorld data type
 * so that DBApp can understand the attribute on a stream and create
 * appropriate storage for it.
 *
 * @param propName       The name of the property to bind.
 * @param typeName       The data type to bind the property to.
 * @param databaseLength The maximum storage size of the data type (if
 *                       appropriate). 
 *
 * @returns true on success, false on failure.
 */
bool PyUserTypeBinder::bind( const BW::string & propName,
		const BW::string & typeName, int databaseLength )
{
	const Context & context = curContext();

	// see what the default value for this element is then
	// this should logically be done by CompositePropertyMapping,
	// but its addChild method wants a constructed ProperyMapping
	// (the default value for a subtable is always the empty sequence)
	DataSectionPtr pPropDefault;

	if (context.pDefaultValue)
	{
		pPropDefault = context.pDefaultValue->openSection( propName );
	}

	// Create type object, before we can create property mapping.
	BW::stringstream typeStrm;
	typeStrm << "<Type>" << typeName << "</Type>";

	XMLSectionPtr pXMLTypeSection =
		XMLSection::createFromStream( BW::string(),  typeStrm );
	DataSectionPtr pTypeSection( pXMLTypeSection.get() );
	DataTypePtr pType = DataType::buildDataType( pTypeSection );

	if (pType.exists())
	{
		// add it to the table on the 'top' of the stack
		context.pCompositeProp->addChild(
			PropertyMapping::create( context.namer, propName, *pType,
									databaseLength, pPropDefault, 
									DATABASE_INDEXING_NONE ) );
	}
	else
	{
		ERROR_MSG( "PyUserTypeBinder::bind: Invalid type name %s.\n",
					typeName.c_str() );
		PyErr_SetString( PyExc_TypeError, typeName.c_str() );
	}

	return pType.exists();
}


/**
 * This method starts a new child table definition for USER_TYPE property.
 * This is required when dealing with compound data types such as lists and
 * dictionaries.
 *
 * @param propName  The property name which the child table represents.
 */
void PyUserTypeBinder::beginTable( const BW::string & propName )
{
	const Context & context = curContext();
	DataSectionPtr pPropDefault;
	if (context.pDefaultValue)
	{
		pPropDefault = context.pDefaultValue->openSection( propName );
	}

	CompositePropertyMappingPtr pChild(
									new CompositePropertyMapping( propName ) );
	PropertyMappingPtr pSequence(
							new SequenceMapping( context.namer,
							propName, pChild ) );
	context.pCompositeProp->addChild( pSequence );
	tables_.push( Context( pChild, context.namer, propName, true,
						   pPropDefault ) );
}


/**
 * This method ends a sub-table definition that was previously started with
 * beginTable().
 *
 * @returns true on success, false on error.
 */
bool PyUserTypeBinder::endTable()
{
	bool isOK = ( tables_.size() > 1 );

	if (isOK)
	{
		tables_.pop();
	}
	else
	{
		PyErr_SetString( PyExc_RuntimeError, "No matching beginTable." );
	}

	return isOK;
}


/**
 *
 */
PropertyMappingPtr PyUserTypeBinder::getResult()
{
	if (tables_.size() == 1)
	{
		return PropertyMappingPtr( curContext().pCompositeProp.get() );
	}
	else
	{
		return PropertyMappingPtr();
	}
}


/**
 *
 */
const PyUserTypeBinder::Context & PyUserTypeBinder::curContext() const
{
	MF_ASSERT( !tables_.empty() );
	return tables_.top();
}


PY_TYPEOBJECT( PyUserTypeBinder )

PY_BEGIN_METHODS( PyUserTypeBinder )
	PY_METHOD( beginTable )
	PY_METHOD( endTable )
	PY_METHOD( bind )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyUserTypeBinder )
PY_END_ATTRIBUTES()

BW_END_NAMESPACE

// py_user_type_binder.cpp
