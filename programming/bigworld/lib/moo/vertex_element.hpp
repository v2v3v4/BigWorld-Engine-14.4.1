#ifndef VERTEX_ELEMENT_HPP
#define VERTEX_ELEMENT_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
/**
 * This class is used to define a list of accepted semantics and 
 * storage types of vertex elements. This class is also used
 * to define properties that each semantic and storage type
 * has, and provides conversion facilities between different
 * storage types within a given semantic.
 */
namespace VertexElement
{
	// -----------------------------------------------------------------------------
	// Section: SemanticType, StorageType
	// -----------------------------------------------------------------------------

	/**
	 * The semantic of a vertex element defining the use 
	 * of the stored data.
	 */
	struct SemanticType
	{
		enum Value
		{
			UNKNOWN,
			POSITION,
			BLENDWEIGHT,
			BLENDINDICES,
			NORMAL,
			PSIZE,
			TEXCOORD,
			TANGENT,
			BINORMAL,
			TESSFACTOR,
			POSITIONT,
			COLOR,
			FOG,
			DEPTH,
			SAMPLE
		};
	};

	/**
	 * The storage type of the vertex element.
	 */
	struct StorageType
	{
		enum Value
		{
			UNKNOWN,
			FLOAT1,
			FLOAT2,
			FLOAT3,
			FLOAT4,
			HALF2,
			HALF4,
			COLOR,
			UBYTE1,
			UBYTE2,
			UBYTE3,
			UBYTE4,	// in a NORMALs context, this is (11, 11, 10) packing
			UBYTE4_NORMAL_8_8_8,
			SHORT1,
			SHORT2,
			SHORT3,
			SHORT4,
			UBYTE4N,
			SHORT2N,
			SHORT4N,
			USHORT2N,
			USHORT4N,
			UDEC3,
			DEC3N,

			// types for Special Cases
			SC_DIV3_III,
			SC_REVERSE_III,
			SC_REVERSE_PADDED_III_,
			SC_REVERSE_PADDED__WW_,
			SC_REVERSE_PADDED_WW_,
		};

	};

	/// String conversion utilities, case insensitive
	SemanticType::Value SemanticFromString( const BW::StringRef & semanticName );
	StorageType::Value StorageFromString( const BW::StringRef & storageName );

	const BW::StringRef StringFromSemantic( SemanticType::Value semanticType );
	const BW::StringRef StringFromStorage( StorageType::Value storageType );

	/// Get count of enums defined.
	/// Relies on enum <-> string map in implementation file being in sync.
	uint32 SemanticEntryCount();
	uint32 StorageEntryCount();

	/// Byte size of a specific storage type, returns 0 on unknown.
	size_t typeSize( StorageType::Value type );

	/// Component count of a specific storage type, returns 0 on unknown.
	uint32 componentCount( StorageType::Value storageType );

}	// namespace VertexElement

} // namespace Moo

BW_END_NAMESPACE

#endif // VERTEX_ELEMENT_HPP
