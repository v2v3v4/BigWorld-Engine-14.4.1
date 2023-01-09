#ifndef USER_DATA_OBJECT_DESCRIPTION_MAP_HPP
#define USER_DATA_OBJECT_DESCRIPTION_MAP_HPP
#include "resmgr/datasection.hpp"
#include "user_data_object_description.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class MD5;

/**
 * 	This class parses the UserDataObjects.xml file, and stores a map of UserDataObject
 * 	descriptions.  It provides access to udo descriptions by their name.
 * 	@ingroup udo
 */
class UserDataObjectDescriptionMap
{
public:
	UserDataObjectDescriptionMap();
	virtual ~UserDataObjectDescriptionMap();
	bool 	parse( DataSectionPtr pSection );
	int	size() const;
	const UserDataObjectDescription& udoDescription( BW::string UserDataObjectName ) const;
	void clear();
	bool isUserDataObject( const BW::string& name ) const;

	typedef BW::map<BW::string, UserDataObjectDescription> DescriptionMap;
	DescriptionMap::const_iterator begin() const { return map_.begin(); }
	DescriptionMap::const_iterator end() const{ return map_.end(); }

	void getNames( BW::vector< BW::string > & names ) const;

private:
	DescriptionMap 		map_;
};

BW_END_NAMESPACE

#endif // USER_DATA_OBJECT_DESCRIPTION_MAP_HPP

