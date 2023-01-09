#ifndef TUPLE_DATA_TYPE_HPP
#define TUPLE_DATA_TYPE_HPP

#include "sequence_data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent the data type of a tuple object. Each
 *	element must be of the same type.
 *
 *	@ingroup entity
 */
class TupleDataType : public SequenceDataType
{
public:
	TupleDataType( MetaDataType * pMeta, DataTypePtr elementType,
			int size = 0, int dbLen = 0 );

	static DataType * construct( MetaDataType * pMeta,
		DataTypePtr elementType, int size, int dbLen );

protected:
	virtual bool startSequence( DataSink & sink, size_t count ) const;

	virtual int compareDefaultValue( const DataType & other ) const;

	virtual void addToMD5( MD5 & md5 ) const;

	virtual void setDefaultValue( DataSectionPtr pSection );

	virtual bool getDefaultValue( DataSink & output ) const;

private:
	DataSectionPtr pDefaultSection_;
};

BW_END_NAMESPACE

#endif // TUPLE_DATA_TYPE_HPP
