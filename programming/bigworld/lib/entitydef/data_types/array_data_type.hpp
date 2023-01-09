#ifndef ARRAY_DATA_TYPE_HPP
#define ARRAY_DATA_TYPE_HPP

#include "sequence_data_type.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This template class is used to represent the data type of an array object.
 *	There are only certain types of arrays that are currently supported.
 *
 *	@ingroup entity
 */
class ArrayDataType : public SequenceDataType
{
public:
	ArrayDataType( MetaDataType * pMeta, DataTypePtr elementType,
			int size = 0, int dbLen = 0 );

	static DataType * construct( MetaDataType * pMeta,
		DataTypePtr elementType, int size, int dbLen );

protected:
	virtual bool startSequence( DataSink & sink, size_t count ) const;

	virtual int compareDefaultValue( const DataType & other ) const;

	virtual void setDefaultValue( DataSectionPtr pSection );
	virtual bool getDefaultValue( DataSink & output ) const;

	virtual DataSectionPtr pDefaultSection() const;

	virtual ScriptObject attach( ScriptObject pObject,
		PropertyOwnerBase * pOwner, int ownerRef );

	virtual void detach( ScriptObject pObject );

	virtual PropertyOwnerBase * asOwner( ScriptObject pObject ) const;

	virtual void addToMD5( MD5 & md5 ) const;

private:
	DataSectionPtr pDefaultSection_;
};

BW_END_NAMESPACE

#endif // ARRAY_DATA_TYPE_HPP
