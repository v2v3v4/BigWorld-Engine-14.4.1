/**
 * 	@file
 *
 *	This file provides the implementation of the BaseUserDataObjectDescription class.
 *  It is intended that EntityDescription and UserDataObjectDescription both descend from this class to minimise code 
 *  duplication.
 *
 * 	@ingroup UserDataObject
 */

#include "pch.hpp"

#include "base_user_data_object_description.hpp"

#include "script_data_sink.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include <float.h>

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "base_user_data_object_description.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: BaseUserDataObjectDescription
// This class is responsible for reading a UserDataObject defined in a def file
// 
// -----------------------------------------------------------------------------

/**
 *	The constructor.
 */
BaseUserDataObjectDescription::BaseUserDataObjectDescription()
{
}


/**
 *	Destructor.
 */
BaseUserDataObjectDescription::~BaseUserDataObjectDescription()
{
}

/**
 *	This method parses an user data object description from a datasection.
 *
 *	@param name		The name of the  type.
 *	@param pSection	If not NULL, this data section is used, otherwise the
 *					data section is opened based on the name.
 *
 *	@return true if successful, false otherwise.
 */
bool BaseUserDataObjectDescription::parse( const BW::string & name,
		DataSectionPtr pSection )
{
	if (!pSection)
	{
		BW::string filename = this->getDefsDir() + "/" + name + ".def";
		pSection = BWResource::openSection( filename );

		if (!pSection)
		{
			ERROR_MSG( "BaseUserDataObjectDescription::parse: Could not open %s\n",
					filename.c_str() );
			return false;
		}
	}

	BW::string parentName = pSection->readString( "Parent" );

	if (!parentName.empty())
	{
		if (!this->parse( parentName, NULL ))
		{
			ERROR_MSG( "BaseUserDataObjectDescription::parse: "
						"Could not parse %s, parent of %s\n",
					parentName.c_str(), name.c_str() );
			return false;
		}
	}

	name_ = name;
	bool result = this->parseInterface( pSection, name_.c_str(), BW::string() );
	return result;
}


/**
 *	This method parses a data section for the properties and methods associated
 *	with this entity description.
 */
bool BaseUserDataObjectDescription::parseInterface( DataSectionPtr pSection,
		const char * interfaceName, const BW::string & componentName )
{
	if (!pSection)
	{
		return false;
	}

	bool result =
		this->parseImplements( pSection->openSection( "Implements" ),
				componentName ) &&
		this->parseProperties( pSection->openSection( "Properties" ),
				componentName ); 
	return result;
}


/**
 *	This method parses an "Implements" section. This is used so that defs can
 *	share interfaces. It adds each of the referred interfaces to this
 *	description.
 */
bool BaseUserDataObjectDescription::parseImplements(
		DataSectionPtr pInterfaces, const BW::string & componentName )
{

	bool result = true;

	if (pInterfaces)
	{
		DataSection::iterator iter = pInterfaces->begin();

		while (iter != pInterfaces->end())
		{
			BW::string interfaceName = (*iter)->asString();

			DataSectionPtr pInterface = BWResource::openSection(
				 	this->getDefsDir() + "/interfaces/" + interfaceName + ".def" );

			if (!this->parseInterface( pInterface, interfaceName.c_str(),
					componentName ))
			{
				ERROR_MSG( "BaseUserDataObjectDescription::parseImplements: "
					"Failed parsing interface %s\n", interfaceName.c_str() );
				result = false;
			}

			++iter;
		}
	}

	return result;

}


/**
 *	This method searches a datasection for properties belonging to
 *	this entity description, and adds them to the given python
 *	dictionary.
 *
 *	@param pSection		Datasection containing properties
 *	@param sDict		Dictionary to add the properties to
 */
void BaseUserDataObjectDescription::addToDictionary( DataSectionPtr pSection,
	ScriptDict sDict ) const
{
	for (Properties::const_iterator itDesc = properties_.begin();
		itDesc != properties_.end(); ++itDesc)
	{
		if (itDesc->isComponentised())
		{
			continue;
		}

		DataSectionPtr pPropSec = pSection->openSection( itDesc->name() );
		ScriptObject pValue = itDesc->pInitialValue();

		// change to created value if it parses ok
		if (pPropSec)
		{
			ScriptDataSink sink;
			if (itDesc->createFromSection( pPropSec, sink ))
			{
				pValue = sink.finalise();
			}
		}

		MF_ASSERT_DEV( pValue );

		if (pValue)
		{
			sDict.setItem( itDesc->name().c_str(), pValue,
				ScriptErrorPrint() );
		}
	}
}


// -----------------------------------------------------------------------------
// Section: Property related.
// -----------------------------------------------------------------------------

/**
 *	This method returns the number of data properties of this entity class.
 */
unsigned int BaseUserDataObjectDescription::propertyCount() const
{
	return static_cast<uint>(properties_.size());
}


/**
 *	This method returns a given data property for this entity class.
 *  TODO: move to entity
 */
DataDescription* BaseUserDataObjectDescription::property( unsigned int n ) const
{
	MF_ASSERT_DEV(n < properties_.size());
	if (n >= properties_.size()) return NULL;

	return const_cast<DataDescription *>( &properties_[n] );
}


/**
 *	This method returns data property description for given property 
 *	and component name.
 */
DataDescription*
BaseUserDataObjectDescription::findProperty( const char * name,
											 const char * component ) const
{
	MF_ASSERT_DEV( name );
	MF_ASSERT_DEV( component );
	
	ComponentPropertyMap::const_iterator itComp = 
		propertyMap_.find( component );
	
	if (itComp == propertyMap_.end())
	{
		return NULL;
	}
	PropertyMap::const_iterator iter = itComp->second.find( name );

	return (iter != itComp->second.end()) ? 
		this->property( iter->second ) : NULL;
}

/**
 *	This method returns data property description for given property name, 
 *	the name can include component name separated by dot.
 */
DataDescription* 
BaseUserDataObjectDescription::findCompoundProperty( const char * name ) const
{
	MF_ASSERT_DEV( name );
	
	const char * pSep = strchr(name, MemberDescription::COMPONENT_NAME_SEPARATOR);
	if (pSep)
	{
		const BW::string component( name, pSep );
		return this->findProperty( pSep + 1, component.c_str() );
	}
	else
	{
		return this->findProperty( name, /*component*/"" );
	}
}

/**
 *	This method returns a property map for given component.
 */
BaseUserDataObjectDescription::PropertyMap & 
BaseUserDataObjectDescription::getComponentProperties( 
											const char * componentName )
{
	MF_ASSERT_DEV( componentName );
	
	return propertyMap_[componentName];
}

BW_END_NAMESPACE

// base_custom_item_description.cpp
