#ifndef SINGLE_TYPE_DATA_SOURCES_HPP
#define SINGLE_TYPE_DATA_SOURCES_HPP

#pragma once

#include "data_source.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/unique_id.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "network/basictypes.hpp"

namespace BW
{

class BWENTITY_API BlobDataSource : public UnsupportedDataSource
{
public:
	BlobDataSource( const BW::string & value );

private:
	bool readBlob( BW::string & value );

	const BW::string & value_;
};

template< typename TYPE >
class BWENTITY_API SingleTypeDataSourceT : public UnsupportedDataSource
{
public:
	SingleTypeDataSourceT( const TYPE & value );

private:
	bool read( TYPE & value );

	const TYPE & value_;
};

template<>
class BWENTITY_API SingleTypeDataSourceT< bool > : public UnsupportedDataSource
{
public:
	SingleTypeDataSourceT( bool value );

private:
	bool read( uint8 & value );

	const bool value_;
};

// Only used in import mode, the export is in the .cpp as we require the method
// bodies.
#if defined BWENTITY_DLL_IMPORT
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
#endif // defined BWENTITY_DLL_IMPORT

typedef SingleTypeDataSourceT< float > FloatDataSource;
typedef SingleTypeDataSourceT< double > DoubleDataSource;
typedef SingleTypeDataSourceT< uint8 > UInt8DataSource;
typedef SingleTypeDataSourceT< uint16 > UInt16DataSource;
typedef SingleTypeDataSourceT< uint32 > UInt32DataSource;
typedef SingleTypeDataSourceT< uint64 > UInt64DataSource;
typedef SingleTypeDataSourceT< int8 > Int8DataSource;
typedef SingleTypeDataSourceT< int16 > Int16DataSource;
typedef SingleTypeDataSourceT< int32 > Int32DataSource;
typedef SingleTypeDataSourceT< int64 > Int64DataSource;
typedef SingleTypeDataSourceT< BW::string > StringDataSource;
typedef SingleTypeDataSourceT< BW::wstring > WStringDataSource;
typedef SingleTypeDataSourceT< Vector2 > Vector2DataSource;
typedef SingleTypeDataSourceT< Vector3 > Vector3DataSource;
typedef SingleTypeDataSourceT< Vector4 > Vector4DataSource;
#if defined MF_SERVER
typedef SingleTypeDataSourceT< EntityMailBoxRef > EntityMailBoxRefDataSource;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
typedef SingleTypeDataSourceT< UniqueID > UniqueIDDataSource;
#endif // !defined EDITOR_ENABLED
typedef SingleTypeDataSourceT< bool > BoolDataSource;

} // namespace BW

#endif // SINGLE_TYPE_DATA_SOURCES_HPP
