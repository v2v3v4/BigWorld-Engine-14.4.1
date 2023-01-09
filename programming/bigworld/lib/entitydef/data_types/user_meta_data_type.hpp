#ifndef USER_META_DATA_TYPE_HPP
#define USER_META_DATA_TYPE_HPP

#include "entitydef/meta_data_type.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements an object that creates user defined (in script) data
 *	types.
 *
 *	An alias should usually be defined for these types.
 *
 *	In alias.xml, you could have the following:
 *	@code
 *	<ITEM>
 *		USER_TYPE <implementedBy> ItemDataType.instance </implementedBy>
 *	</ITEM>
 *	@endcode
 *
 *	The object ItemDataType.instance needs to support a set of data type
 *	methods.
 *
 *	@see UserDataType
 */
class UserMetaDataType : public MetaDataType
{
public:
	UserMetaDataType();
	virtual ~UserMetaDataType();

	virtual const char * name() const { return "USER_TYPE"; }

	virtual DataTypePtr getType( DataSectionPtr pSection );
};

BW_END_NAMESPACE

#endif // USER_META_DATA_TYPE_HPP
