#include "pch.hpp"

#include "fixed_dict_meta_data_type.hpp"

#include "class_meta_data_type.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_types.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
FixedDictMetaDataType::FixedDictMetaDataType()
{
	MetaDataType::addMetaType( this );
}


/**
 *	Destructor.
 */
FixedDictMetaDataType::~FixedDictMetaDataType()
{
	MetaDataType::delMetaType( this );
}


/**
 *	This method parses the type name for the given FIXED_DICT data type.
 *
 *	@param pSection 	The data section from which the type name is parsed.
 *	@param dataType 	The data type that is receiving the custom type name.
 */
bool FixedDictMetaDataType::parseCustomTypeName( DataSectionPtr pSection, 
		FixedDictDataType & dataType )
{
	BW::string typeName = pSection->readString( "TypeName" );
	if (typeName.empty())
	{
		// Allow empty type names, but don't set anything on dataType.
		return true;
	}

	if (typeName.find( " " ) != BW::string::npos)
	{
		ERROR_MSG( "FixedDictMetaDataType::parseTypeName: "
				"<TypeName> must not contain spaces: \"%s\"\n",
			typeName.c_str() );
		return false;
	}

	if (typeNames_.count( typeName ))
	{
		ERROR_MSG( "FixedDictMetaDataType::parseTypeName: "
				"<TypeName> is not unique: \"%s\"\n",
			typeName.c_str() );
		return false;
	}

	typeNames_.insert( typeName );
	dataType.customTypeName( typeName );

	return true;
}


/**
 *	This method parses a custom class specification for a FIXED_DICT data type
 *	from a data section.
 *
 *	@param pSection 	The data section in which the custom-class is
 *						specified.
 *	@param dataType		The data type that is being specified.
 */
bool FixedDictMetaDataType::parseCustomClass( DataSectionPtr pSection,
		FixedDictDataType & dataType )
{
	BW::string implementedBy = pSection->readString( "implementedBy" );

	if (!implementedBy.empty())
	{
		BW::string::size_type pos = implementedBy.find_last_of( "." );

		if (pos == BW::string::npos)
		{
			ERROR_MSG( "FixedDictMetaDataType::getType: "
						"<implementedBy> %s - must contain a '.'",
						implementedBy.c_str() );
			return false;
		}

		BW::string moduleName = implementedBy.substr( 0, pos );
		BW::string instanceName = implementedBy.substr( pos + 1,
				implementedBy.size() );

		dataType.setCustomClassImplementor( moduleName,
				instanceName );
	}

	return true;
}


/**
 *
 */
DataTypePtr FixedDictMetaDataType::getType( DataSectionPtr pSection )
{
	// Process <Properties> section just like CLASS
	std::auto_ptr< FixedDictDataType > pFixedDictType( 
		ClassMetaDataType::buildType( pSection, *this ) );

	if (pFixedDictType.get() == NULL)
	{
		return NULL;
	}

	if (!this->parseCustomTypeName( pSection, *pFixedDictType ))
	{
		return NULL;
	}

#if defined( SCRIPT_PYTHON )
	if (!this->parseCustomClass( pSection, *pFixedDictType ))
	{
		return NULL;
	}
#endif

	return pFixedDictType.release();
}



namespace
{	
	FixedDictMetaDataType s_FIXED_DICT_metaDataType;
}

DATA_TYPE_LINK_ITEM( FIXED_DICT )

BW_END_NAMESPACE

// fixed_dict_meta_data_type.cpp
