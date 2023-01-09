#ifndef UDO_REF_DATA_TYPE_HPP
#define UDO_REF_DATA_TYPE_HPP

#include "entitydef/data_description.hpp"


BW_BEGIN_NAMESPACE

class MetaDataType;

/**
 *	This class is a simple stand-in implementation of UserDataObjectDataType that is
 *	found in bigworld/src/lib/chunk/user_data_object_link_data_type.hpp.
 */
class UDORefDataType : public DataType
{
public:
	UDORefDataType( MetaDataType * pMeta );

	virtual bool isSameType( ScriptObject pValue ) { return true; }

	virtual void setDefaultValue( DataSectionPtr pSection ) {}

	virtual bool getDefaultValue( DataSink & output ) const { return false; }

	virtual int streamSize() const;

	virtual bool addToSection( DataSource & source,
			DataSectionPtr pSection ) const
	{
		return false;
	}

	virtual bool createFromSection( DataSectionPtr pSection,
			DataSink & sink ) const
	{
		return false;
	}

	virtual void addToMD5( MD5 & md5 ) const;

	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;

	virtual bool operator<( const DataType & other ) const;

private:
};

BW_END_NAMESPACE

#endif // UDO_REF_DATA_TYPE_HPP
