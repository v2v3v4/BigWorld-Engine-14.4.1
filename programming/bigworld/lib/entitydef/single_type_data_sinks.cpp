#include "pch.hpp"

#include "single_type_data_sinks.hpp"

namespace BW
{

int SingleTypeDataSinks_Token = 0;

// -----------------------------------------------------------------------------
// Section: SingleTypeDataSinkT< TYPE >
// -----------------------------------------------------------------------------

template< typename TYPE >
SingleTypeDataSinkT< TYPE >::SingleTypeDataSinkT() :
	value_()
{}


template< typename TYPE >
const TYPE & SingleTypeDataSinkT< TYPE >::getValue() const
{
	return value_;
}


/**
 * This method overrides the matching-typed method in DataSink
 */
template< typename TYPE >
bool SingleTypeDataSinkT< TYPE >::write( const TYPE & value )
{
	value_ = value;
	return true;
}


// -----------------------------------------------------------------------------
// Section: BlobDataSink
// -----------------------------------------------------------------------------

BlobDataSink::BlobDataSink() :
	value_()
{}


const BW::string & BlobDataSink::getValue() const
{
	return value_;
}


/**
 * This method overrides the writeBlob method in DataSink
 */
bool BlobDataSink::writeBlob( const BW::string & value )
{
	value_ = value;
	return true;
}


// -----------------------------------------------------------------------------
// Section: BoolDataSink
// -----------------------------------------------------------------------------

SingleTypeDataSinkT< bool >::SingleTypeDataSinkT() :
	value_()
{}


bool SingleTypeDataSinkT< bool >::getValue() const
{
	return value_;
}


/**
 * This method overrides the writeBool method in DataSink
 */
bool SingleTypeDataSinkT< bool >::write( const uint8 & value )
{
	value_ = (value != 0);
	return true;
}


#if !defined BWENTITY_DLL_IMPORT
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< float >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< double >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< uint8 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< uint16 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< uint32 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< uint64 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< int8 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< int16 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< int32 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< int64 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< BW::string >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< BW::wstring >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< Vector2 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< Vector3 >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< Vector4 >;
#if defined MF_SERVER
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< EntityMailBoxRef >;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
BWENTITY_EXPORTED_TEMPLATE_CLASS SingleTypeDataSinkT< UniqueID >;
#endif // !defined EDITOR_ENABLED
#endif // !defined BWENTITY_DLL_IMPORT

} // namespace BW

// single_type_data_sinks.cpp
