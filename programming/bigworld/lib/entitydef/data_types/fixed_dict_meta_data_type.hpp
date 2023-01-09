#ifndef FIXED_DICT_META_DATA_TYPE_HPP
#define FIXED_DICT_META_DATA_TYPE_HPP

#include "entitydef/meta_data_type.hpp"

#include "fixed_dict_data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements a factory for FixedDictDataType.
 */
class FixedDictMetaDataType : public MetaDataType
{
public:
	typedef FixedDictDataType DataType;

	FixedDictMetaDataType();
	virtual ~FixedDictMetaDataType();

	virtual const char * name()	const { return "FIXED_DICT"; }

	virtual DataTypePtr getType( DataSectionPtr pSection );

private:
	bool parseCustomTypeName( DataSectionPtr pSection, 
		FixedDictDataType & dataType );
	bool parseCustomClass( DataSectionPtr pSection,
		FixedDictDataType & dataType );

	typedef BW::set< BW::string > TypeNames;
	TypeNames typeNames_;
};

BW_END_NAMESPACE

#endif // FIXED_DICT_META_DATA_TYPE_HPP
