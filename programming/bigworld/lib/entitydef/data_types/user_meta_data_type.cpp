#include "pch.hpp"

#include "user_meta_data_type.hpp"

#include "user_data_type.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UserMetaDataType::UserMetaDataType()
{
	MetaDataType::addMetaType( this );
}


UserMetaDataType::~UserMetaDataType()
{
	MetaDataType::delMetaType( this );
}


DataTypePtr UserMetaDataType::getType( DataSectionPtr pSection )
{
	BW::string implementedBy = pSection->readString( "implementedBy" );

	if (implementedBy.empty())
	{
		ERROR_MSG( "UserMetaDataType::getType: "
									"implementedBy is not specified.\n" );
		return NULL;
	}

	BW::string::size_type pos = implementedBy.find_last_of( "." );

	if (pos == BW::string::npos)
	{
		ERROR_MSG( "UserMetaDataType::getType: "
							"Invalid implementedBy %s - must contain a '.'",
						implementedBy.c_str() );
		return NULL;
	}


	BW::string moduleName = implementedBy.substr( 0, pos );
	BW::string instanceName = implementedBy.substr( pos + 1,
			implementedBy.size() );

	return new UserDataType( this, moduleName, instanceName );
}


static UserMetaDataType s_USER_TYPE_metaDataType;
DATA_TYPE_LINK_ITEM( USER_TYPE )

BW_END_NAMESPACE

// user_meta_data_type.cpp
