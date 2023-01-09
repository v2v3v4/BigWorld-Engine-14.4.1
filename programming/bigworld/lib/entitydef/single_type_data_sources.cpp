#include "pch.hpp"

#include "single_type_data_sources.hpp"

namespace BW
{

int SingleTypeDataSources_Token = 0;

// -----------------------------------------------------------------------------
// Section: BlobDataSource
// -----------------------------------------------------------------------------

BlobDataSource::BlobDataSource( const BW::string & value ) :
	value_( value )
{};


/**
 * This method overrides the readBlob method in DataSource
 */
bool BlobDataSource::readBlob( BW::string & value )
{
	value = value_;
	return true;
}


// -----------------------------------------------------------------------------
// Section: SingleTypeDataSourceT< TYPE >
// -----------------------------------------------------------------------------

template< typename TYPE >
SingleTypeDataSourceT< TYPE >::SingleTypeDataSourceT( const TYPE & value ) :
	value_( value )
{};


/**
 * This method overrides the matching-typed method in DataSource
 */
template< typename TYPE >
bool SingleTypeDataSourceT< TYPE >::read( TYPE & value )
{
	value = value_;
	return true;
}


// -----------------------------------------------------------------------------
// Section: BoolDataSource
// -----------------------------------------------------------------------------

SingleTypeDataSourceT< bool >::SingleTypeDataSourceT( bool value ) :
	value_( value )
{};


/**
 * This method overrides the read method in DataSource
 */
bool SingleTypeDataSourceT< bool >::read( uint8 & value )
{
	value = (value_ ? 1 : 0);
	return true;
}


#if !defined BWENTITY_DLL_IMPORT
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< float >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< double >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< uint8 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< uint16 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< uint32 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< uint64 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< int8 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< int16 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< int32 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< int64 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< BW::string >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< BW::wstring >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< Vector2 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< Vector3 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< Vector4 >;
#if defined MF_SERVER
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< EntityMailBoxRef >;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSourceT< UniqueID >;
#endif // !defined EDITOR_ENABLED
#endif // !defined BWENTITY_DLL_IMPORT

} // namespace BW

// single_type_data_sources.cpp
