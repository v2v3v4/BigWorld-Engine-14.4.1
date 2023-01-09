#include "script/first_include.hpp"

#include "udo_ref_mapping.hpp"

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UDORefMapping::UDORefMapping( const Namer& namer, const BW::string& propName,
		DataSectionPtr pDefaultValue ) :
	UniqueIDMapping( namer, propName, getGuidSection( pDefaultValue ) )
{
}


/**
 *	This static method retrieves the GUID of the UDO from the provided
 *	DataSection.
 *
 *	@param pParentSection  The DataSection containing the GUID to use.
 *
 *	@returns The DataSectionPtr of the GUID if it exists, otherwise NULL.
 */
DataSectionPtr UDORefMapping::getGuidSection( DataSectionPtr pParentSection )
{
	return (pParentSection) ? pParentSection->openSection( "guid" ) : NULL;
}

BW_END_NAMESPACE

// udo_ref_mapping.cpp
