/**
 * 	@file
 *
 *	This file provides the implementation of the UserDataObjectDescription class.
 *
 * 	@ingroup udo
 */
#include "pch.hpp"

#include "user_data_object_description.hpp"
#include "constants.hpp"

#include "cstdmf/debug.hpp"

#include "resmgr/bwresource.hpp"

#include <algorithm>
#include <cctype>


DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

#include "user_data_object_description.ipp"


/**
 *	This method parses a data section for the properties and methods associated
 *	with this user data object description.
 */
bool UserDataObjectDescription::parseInterface( DataSectionPtr pSection,
		const char * interfaceName, const BW::string & componentName )
{
	if (!pSection)
	{
		return false;
	}

	bool result = BaseUserDataObjectDescription::parseInterface( pSection,
			interfaceName, componentName );
	// Read domain here to ensure no interface or parent overrides it
	BW::string domainString = pSection->readString("Domain");
	std::transform(domainString.begin(), domainString.end(), domainString.begin(), 
		(int(*)(int)) toupper);
	if ( domainString.empty() ){
		//DEFAULT TO CELL
		domain_ = CELL; 
		WARNING_MSG("UserDataObjectDescription::parseInterface:"
				"domain not set for user data object default is cell");
	} else if (domainString == "BASE"){
		domain_ = BASE;
	} else if (domainString == "CELL"){
		domain_ = CELL;
	} else if (domainString == "CLIENT"){
		domain_ = CLIENT;
	}
	return result;
}


/**
 *	This method parses a data section for the properties associated with this
 *	entity description.
 *
 *	@param pProperties	The datasection containing the properties.
 *
 *	@return true if successful, false otherwise.
 */
bool UserDataObjectDescription::parseProperties( DataSectionPtr pProperties,
		const BW::string & componentName )
{
//	MF_ASSERT( properties_.empty() );

	if (pProperties)
	{
		for (DataSectionIterator iter = pProperties->begin();
				iter != pProperties->end();
				++iter)
		{
			DataDescription dataDescription;

			if (!dataDescription.parse( *iter, name_, componentName, NULL,
					DataDescription::PARSE_IGNORE_FLAGS ))
			{
				WARNING_MSG( "Error parsing properties for %s\n",
						name_.c_str() );
				return false;
			}

#ifndef EDITOR_ENABLED
			if (dataDescription.isEditorOnly())
			{
				continue;
			}
#endif

			int index = static_cast<int>(properties_.size());
			PropertyMap & propertyMap 
				= this->getComponentProperties( componentName.c_str() );
			PropertyMap::const_iterator propIter =
					propertyMap.find( dataDescription.name().c_str() );
			if (propIter != propertyMap.end())
			{
				INFO_MSG( "UserDataObjectDescription::parseProperties: "
						"property %s%s.%s is being overridden.\n",
						name_.c_str(), 
						componentName.empty() ? "" : ( "." + componentName ).c_str(),
						dataDescription.name().c_str() );
				index = propIter->second;
			}
			dataDescription.index( index );
			propertyMap[dataDescription.name().c_str()] = dataDescription.index();
#ifdef EDITOR_ENABLED
			DataSectionPtr widget = (*iter)->openSection( "Widget" );
			if ( !!widget )
			{
				dataDescription.widget( widget );
			}
#endif
			if (index == int(properties_.size()))
			{
				properties_.push_back( dataDescription );
			}
			else
			{
				properties_[index] = dataDescription;
			}
		}
	}
	/*
	else
	{
		// Not really the correct message since the data section may be an
		// interface. Also probably not worthwhile since it's fine for the files
		// not to have this section.
		WARNING_MSG( "%s has no Properties section.\n", name_.c_str() );
	}
	*/

	return true;
}


/**
  * Tell udo description which directory it should try read
  * the .def files from
  */
const BW::string UserDataObjectDescription::getDefsDir() const
{
	return EntityDef::Constants::userDataObjectsDefsPath();
}

BW_END_NAMESPACE

// user_data_object_description.cpp
