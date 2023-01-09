#include "pch.hpp"

#include "user_data_object_description.hpp"
#include "user_data_object_description_map.hpp"

#include "cstdmf/debug.hpp"

#include "resmgr/bwresource.hpp"

#include "script/init_time_job.hpp"

DECLARE_DEBUG_COMPONENT2( "UserDataObjectDescriptionMap", 0 )


BW_BEGIN_NAMESPACE

int UserDataObjectDescriptionMap_Token =1;

/*
 *	This class is used to clear entity description state before PyFinalize is
 *	called.
 */
class UserDataObjectDefFiniTimeJob : public Script::FiniTimeJob
{
protected:
	void fini()
	{
		DataType::clearAllScript();
	}
};

static UserDataObjectDefFiniTimeJob s_udoDefFiniTimeJob;


/**
 *	Constructor.
 */
UserDataObjectDescriptionMap::UserDataObjectDescriptionMap()
{
}

/**
 *	Destructor.
 */
UserDataObjectDescriptionMap::~UserDataObjectDescriptionMap()
{
}

/**
 *	This method parses the udo description map from a datasection.
 *
 *	@param pSection	Datasection containing the entity descriptions.
 *
 *	@return true if successful, false otherwise.
 */
bool UserDataObjectDescriptionMap::parse( DataSectionPtr pSection )
{
	if (!pSection)
	{
		ERROR_MSG( "UserDataObjectDescriptionMap::parse: pSection is NULL\n" );
		return false;
	}

	bool isOkay = true;
	int size = pSection->countChildren();

	for (int i = 0; i < size; i++)
	{
		DataSectionPtr pSubSection = pSection->openChild( i );
		UserDataObjectDescription desc;

		BW::string typeName = pSubSection->sectionName();

		if (!desc.parse( typeName ))
		{
			ERROR_MSG( "UserDataObjectDescriptionMap: "
				"Failed to load or parse def for UserDataObject %s\n",
				typeName.c_str() );

			isOkay = false;
			continue;
		}
		map_[ desc.name() ] = desc ;
	}
	return isOkay;
}


/**
 *	This method returns the number of UserDataObjects.
 *
 *	@return Number of User Data Objects.
 */
int UserDataObjectDescriptionMap::size() const
{
	return static_cast<int>(map_.size());
}


/**
 *	This method returns the entity description with the given index.
 */
const UserDataObjectDescription& UserDataObjectDescriptionMap::udoDescription(
		BW::string name ) const
{
	DescriptionMap::const_iterator result = map_.find(name);
	IF_NOT_MF_ASSERT_DEV( result != map_.end() )
	{
		MF_EXIT( "can't find UDO description" );
	}

    return result->second;
}



/**
 *	This method clears all the entity descriptions stored in this object.
 */
void UserDataObjectDescriptionMap::clear()
{
	map_.clear();
}


/**
 *	
 */
bool UserDataObjectDescriptionMap::isUserDataObject( const BW::string& name ) const
{
	return map_.find( name ) != map_.end();
}


/**
 *	This method gets the names of all the UDO types.
 *
 *	@param names The vector that receives the names.
 */
void UserDataObjectDescriptionMap::getNames( BW::vector< BW::string > & names ) const
{
	names.reserve( map_.size() );

	DescriptionMap::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		names.push_back( iter->first );

		++iter;
	}
}

BW_END_NAMESPACE

/* user_data_object_description_map.cpp */
