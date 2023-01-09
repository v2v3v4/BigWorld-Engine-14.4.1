#include "pch.hpp"
#include "vertex_element.hpp"
#include "cstdmf/string_builder.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

// -----------------------------------------------------------------------------
// Section: VertexElement
// -----------------------------------------------------------------------------

typedef VertexElement::SemanticType Sem;
typedef std::pair< Sem::Value, const char* > SemanticEntry;

SemanticEntry s_semanticData[] = 
{
	SemanticEntry( Sem::POSITION, "POSITION" ),
	SemanticEntry( Sem::BLENDWEIGHT, "BLENDWEIGHT" ),
	SemanticEntry( Sem::BLENDINDICES, "BLENDINDICES" ),
	SemanticEntry( Sem::NORMAL, "NORMAL" ),
	SemanticEntry( Sem::PSIZE, "PSIZE" ),
	SemanticEntry( Sem::TEXCOORD, "TEXCOORD" ),
	SemanticEntry( Sem::TANGENT, "TANGENT" ),
	SemanticEntry( Sem::BINORMAL, "BINORMAL" ),
	SemanticEntry( Sem::TESSFACTOR, "TESSFACTOR" ),
	SemanticEntry( Sem::POSITIONT, "POSITIONT" ),
	SemanticEntry( Sem::COLOR, "COLOR" ),
	SemanticEntry( Sem::FOG, "FOG" ),
	SemanticEntry( Sem::DEPTH, "DEPTH" ),
	SemanticEntry( Sem::SAMPLE, "SAMPLE" )
};

uint32 VertexElement::SemanticEntryCount()
{
	return ARRAYSIZE( s_semanticData );
}


typedef VertexElement::StorageType Stor;
typedef std::pair< Stor::Value, const char* > StorageEntry;

StorageEntry s_storageData[] = 
{
	StorageEntry( Stor::FLOAT1, "FLOAT1" ),
	StorageEntry( Stor::FLOAT2, "FLOAT2" ),
	StorageEntry( Stor::FLOAT3, "FLOAT3" ),
	StorageEntry( Stor::FLOAT4, "FLOAT4" ),
	StorageEntry( Stor::COLOR, "COLOR" ),
	StorageEntry( Stor::UBYTE1, "UBYTE1" ),
	StorageEntry( Stor::UBYTE2, "UBYTE2" ),
	StorageEntry( Stor::UBYTE3, "UBYTE3" ),
	StorageEntry( Stor::UBYTE4, "UBYTE4" ),
	StorageEntry( Stor::UBYTE4_NORMAL_8_8_8, "UBYTE4_NORMAL_8_8_8" ),
	StorageEntry( Stor::SHORT1, "SHORT1" ),
	StorageEntry( Stor::SHORT2, "SHORT2" ),
	StorageEntry( Stor::SHORT3, "SHORT3" ),
	StorageEntry( Stor::SHORT4, "SHORT4" ),
	StorageEntry( Stor::UBYTE4N, "UBYTE4N" ),
	StorageEntry( Stor::SHORT2N, "SHORT2N" ),
	StorageEntry( Stor::SHORT4N, "SHORT4N" ),
	StorageEntry( Stor::USHORT2N, "USHORT2N" ),
	StorageEntry( Stor::USHORT4N, "USHORT4N" ),
	StorageEntry( Stor::UDEC3, "UDEC3" ),
	StorageEntry( Stor::DEC3N, "DEC3N" ),
	StorageEntry( Stor::HALF2, "FLOAT16_2" ),
	StorageEntry( Stor::HALF4, "FLOAT16_4" ),

	StorageEntry( Stor::SC_DIV3_III, "SC_DIV3_III" ),
	StorageEntry( Stor::SC_REVERSE_III, "SC_REVERSE_III" ),
	StorageEntry( Stor::SC_REVERSE_PADDED_III_, "SC_REVERSE_PADDED_III_" ),
	StorageEntry( Stor::SC_REVERSE_PADDED__WW_, "SC_REVERSE_PADDED__WW_" ),
	StorageEntry( Stor::SC_REVERSE_PADDED_WW_, "SC_REVERSE_PADDED_WW_" )
};

uint32 VertexElement::StorageEntryCount()
{
	return ARRAYSIZE( s_storageData );
}


///----------------------------------------------------------------------------
/// Lookup predicates

// pair.first (enum val) equality
template < class PairType >
struct EqualsVal : public std::unary_function< PairType, bool >
{
	typedef typename PairType::first_type value_type;
	value_type val_;
	EqualsVal( value_type val ) : val_(val) {}

	bool operator()( const PairType & p ) const
	{
		return p.first == val_;
	}
};

// pair.second (name string) equality
template < class PairType >
struct EqualsName : public std::unary_function< PairType, bool >
{
	const BW::StringRef & name_;
	EqualsName( const BW::StringRef & name ) : name_(name) {}

	bool operator()( const PairType & p ) const
	{
		return name_.equals_lower( p.second );
	}
};

///----------------------------------------------------------------------------
/// string -> enum conversions

VertexElement::SemanticType::Value VertexElement::SemanticFromString( 
	const BW::StringRef & semanticName )
{
	SemanticEntry * end = s_semanticData + ARRAYSIZE( s_semanticData );
	const SemanticEntry * iter = std::find_if( s_semanticData, end, 
		EqualsName< SemanticEntry >( semanticName ));
	if (iter != end)
	{
		return iter->first;
	}

	ERROR_MSG( "VertexElement: Unable to find semantic type '%s'\n",
		semanticName.to_string().c_str() );
	MF_ASSERT_DEV( !"enum string not found" );

	return SemanticType::UNKNOWN;
}

VertexElement::StorageType::Value VertexElement::StorageFromString( 
	const BW::StringRef & storageName )
{
	StorageEntry * end = s_storageData + ARRAYSIZE( s_storageData );
	const StorageEntry * iter = std::find_if( s_storageData, end, 
		EqualsName< StorageEntry >( storageName ));
	if (iter != end)
	{
		return iter->first;
	}

	ERROR_MSG( "VertexElement: Unable to find storage type '%s'\n",
		storageName.to_string().c_str() );
	MF_ASSERT_DEV( !"enum string not found" );

	return StorageType::UNKNOWN;
}

///----------------------------------------------------------------------------
/// enum -> string conversions

const BW::StringRef VertexElement::StringFromSemantic( 
	SemanticType::Value semanticType )
{
	SemanticEntry * end = s_semanticData + ARRAYSIZE(s_semanticData);
	const SemanticEntry * iter = std::find_if( s_semanticData, end, 
		EqualsVal< SemanticEntry >( semanticType ));
	if (iter != end)
	{
		return iter->second;
	}
	return BW::StringRef();
}


const BW::StringRef VertexElement::StringFromStorage( 
	StorageType::Value storageType )
{
	StorageEntry * end = s_storageData + ARRAYSIZE(s_storageData);
	const StorageEntry * iter = std::find_if( s_storageData, end, 
		EqualsVal< StorageEntry >( storageType ));
	if (iter != end)
	{
		return iter->second;
	}
	return BW::StringRef();
}


} // namespace Moo

BW_END_NAMESPACE

