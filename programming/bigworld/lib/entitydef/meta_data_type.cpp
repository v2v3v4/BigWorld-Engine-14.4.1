#include "pch.hpp"

#include "meta_data_type.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MetaDataType
// -----------------------------------------------------------------------------

MetaDataType::MetaDataTypes * MetaDataType::s_metaDataTypes_ = NULL;

/*static*/ void MetaDataType::fini()
{
	bw_safe_delete(s_metaDataTypes_);
}

/**
 * 	This static method adds an alias to a native data type name e.g. FLOAT to
 * 	FLOAT32. This is different to alias.xml which aliases a name to a whole
 * 	data type definition e.g. Gun to FIXED_DICT of 3 properties: STRING name,
 * 	UINT32 ammo, FLOAT accuracy.
 */
void MetaDataType::addAlias( const BW::string& orig, const BW::string& alias )
{
	MetaDataType * pMetaDataType = MetaDataType::find( orig );
	IF_NOT_MF_ASSERT_DEV( pMetaDataType )
	{
		return;
	}
	(*s_metaDataTypes_)[ alias ] = pMetaDataType;
}

/**
 *	This static method registers a meta data type.
 */
void MetaDataType::addMetaType( MetaDataType * pMetaType )
{
	if (s_metaDataTypes_ == NULL)
	{
		s_metaDataTypes_ = new MetaDataTypes();

		atexit( &MetaDataType::fini );
	}

	const char * name = pMetaType->name();

	// Some error checking
	if (s_metaDataTypes_->find( name ) != s_metaDataTypes_->end())
	{
		CRITICAL_MSG( "MetaDataType::addType: "
			"%s has already been registered.\n", name );
		return;
	}

	(*s_metaDataTypes_)[ name ] = pMetaType;
}


/**
 *	This static method deregisters a meta data type.
 */
void MetaDataType::delMetaType( MetaDataType * pMetaType )
{
	//s_metaDataTypes_->erase( pMetaType->name() );
	// too tricky to do this on shutdown...
}


/**
 *	This static method finds the given meta data type by name.
 */
MetaDataType * MetaDataType::find( const BW::string & name )
{
	MetaDataTypes::iterator found = s_metaDataTypes_->find( name );
	if (found != s_metaDataTypes_->end()) return found->second;
	return NULL;
}

BW_END_NAMESPACE

// meta_data_type.cpp
