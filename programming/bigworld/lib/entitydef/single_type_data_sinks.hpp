#ifndef SINGLE_TYPE_DATA_SINKS_HPP
#define SINGLE_TYPE_DATA_SINKS_HPP

#pragma once

#include "data_sink.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/unique_id.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "network/basictypes.hpp"

namespace BW
{

class BWENTITY_API BlobDataSink : public UnsupportedDataSink
{
public:
	BlobDataSink();
	const BW::string & getValue() const;

private:
	bool writeBlob( const BW::string & value );

	BW::string value_;
};

template< typename TYPE >
class BWENTITY_API SingleTypeDataSinkT : public UnsupportedDataSink
{
public:
	SingleTypeDataSinkT();
	const TYPE & getValue() const;

private:
	bool write( const TYPE & value );

	TYPE value_;
};

template<>
class BWENTITY_API SingleTypeDataSinkT< bool > : public UnsupportedDataSink
{
public:
	SingleTypeDataSinkT();
	bool getValue() const;

private:
	bool write( const uint8 & value );

	bool value_;
};

// Only used in import mode, the export is in the .cpp as we require the method
// bodies.
#if defined BWENTITY_DLL_IMPORT
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
#endif // defined BWENTITY_DLL_IMPORT

typedef SingleTypeDataSinkT< float > FloatDataSink;
typedef SingleTypeDataSinkT< double > DoubleDataSink;
typedef SingleTypeDataSinkT< uint8 > UInt8DataSink;
typedef SingleTypeDataSinkT< uint16 > UInt16DataSink;
typedef SingleTypeDataSinkT< uint32 > UInt32DataSink;
typedef SingleTypeDataSinkT< uint64 > UInt64DataSink;
typedef SingleTypeDataSinkT< int8 > Int8DataSink;
typedef SingleTypeDataSinkT< int16 > Int16DataSink;
typedef SingleTypeDataSinkT< int32 > Int32DataSink;
typedef SingleTypeDataSinkT< int64 > Int64DataSink;
typedef SingleTypeDataSinkT< BW::string > StringDataSink;
typedef SingleTypeDataSinkT< BW::wstring > WStringDataSink;
typedef SingleTypeDataSinkT< Vector2 > Vector2DataSink;
typedef SingleTypeDataSinkT< Vector3 > Vector3DataSink;
typedef SingleTypeDataSinkT< Vector4 > Vector4DataSink;
#if defined MF_SERVER
typedef SingleTypeDataSinkT< EntityMailBoxRef > EntityMailBoxRefDataSink;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
typedef SingleTypeDataSinkT< UniqueID > UniqueIDDataSink;
#endif // !defined EDITOR_ENABLED
typedef SingleTypeDataSinkT< bool > BoolDataSink;

} // namespace BW

#endif // SINGLE_TYPE_DATA_SINKS_HPP
