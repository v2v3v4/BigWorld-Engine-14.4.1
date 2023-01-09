#include "pch.hpp"
#include "class_meta_data_type.hpp"

#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
ClassMetaDataType::ClassMetaDataType()
{
	MetaDataType::addMetaType( this );
}


/**
 *	Destructor.
 */
ClassMetaDataType::~ClassMetaDataType()
{
	MetaDataType::delMetaType( this );
}


/**
 *
 */
DataTypePtr ClassMetaDataType::getType( DataSectionPtr pSection )
{
	return ClassMetaDataType::buildType( pSection, *this );
}


static ClassMetaDataType s_CLASS_metaDataType;
DATA_TYPE_LINK_ITEM( CLASS )

BW_END_NAMESPACE

// class_meta_data_type.cpp
