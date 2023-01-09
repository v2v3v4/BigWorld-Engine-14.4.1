#ifndef SEQUENCE_META_DATA_TYPE_HPP
#define SEQUENCE_META_DATA_TYPE_HPP

#include "entitydef/meta_data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used for objects that create sequence types.
 */
class SequenceMetaDataType : public MetaDataType
{
public:
	typedef DataType * (*SequenceTypeFactory)(
		MetaDataType * pMeta, DataTypePtr elementPtr , int size, int dbLen );

	SequenceMetaDataType( const char * name, SequenceTypeFactory factory );
	virtual ~SequenceMetaDataType();

	virtual const char * name() const { return name_; }

	virtual DataTypePtr getType( DataSectionPtr pSection );

protected:
	const char * name_;
	SequenceTypeFactory factory_;
};

BW_END_NAMESPACE

#endif // SEQUENCE_META_DATA_TYPE_HPP
