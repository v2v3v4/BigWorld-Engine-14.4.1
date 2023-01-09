#ifndef VERTEX_FORMAT_CONVERSIONS_HPP
#define VERTEX_FORMAT_CONVERSIONS_HPP

#include "cstdmf/bw_namespace.hpp"
#include "vertex_element.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
namespace VertexConversions
{

	/**
	 * Conversion function definition.
	 * @param dst destination memory address for conversion
	 * @param src source memory address for conversion
	 * @param sourceTypeSize for memcopy conversion fallback
	 */
	typedef void (*ConversionFunc)( void *, const void *, size_t );

	/**
	 * Default Value function definition.
	 * @param dst destination memory address for default value
	 * @param sourceTypeSize for memcopy conversion fallback
	 */
	typedef void (*DefaultValueFunc)( void *, size_t );

	/**
	 * Validation function definition.
	 * For a conversion, returns false if the source data is unsuitable
	 * for the conversion (e.g. source data is outside the range supported 
	 * by the conversion).
	 * @param src source memory address for validation
	 * @return True if source data is convertible
	 */
	typedef bool (*ValidationFunc)( const void * );

	//-------------------------------------------------------------------------
	// Conversion and Default Value lookup functions

	/// Select an appropriate validation function based on 
	/// runtime semantic and src,dst types.
	ValidationFunc fetchValidationFunction( 
		VertexElement::SemanticType::Value semantic,
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType );

	/// Select an appropriate Default Value function based on 
	/// runtime semantic and dst type.
	DefaultValueFunc fetchDefaultValueFunction( 
		VertexElement::SemanticType::Value semantic,
		VertexElement::StorageType::Value dstType );

	/// Select an appropriate conversion function based on 
	/// runtime semantic and src,dst types.
	ConversionFunc fetchConvertFunction( 
		VertexElement::SemanticType::Value semantic,
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType );

	/// Perform conversion of a single value based upon
	/// compiletime semantic, and runtime dst/src types
	bool convertValue( VertexElement::SemanticType::Value semantic, 
		void* dstData, VertexElement::StorageType::Value dstType, 
		const void* srcData, VertexElement::StorageType::Value srcType );


	//-------------------------------------------------------------------------
	// Conversion function declarations

	/// Simple memory copy
	void reinterpretConvert( void *dst, const void *src, size_t dstSize );

	// Special cases (see impl)
	void positionAddWConversion( void* dst, const void* src, size_t );
	void normalPackf3tob4( void* dst, const void* src, size_t);
	void normalUnpackb4tof3( void* dst, const void* src, size_t);
	void packf3tob4Normal888( void* dst, const void* src, size_t );
	void unpackb4Normal888tof3( void* dst, const void* src, size_t );
	void convertPackedNormalUbyte4_8_8_8_to_11_11_10( void* dst, const void* src, size_t );
	void convertPackedNormalUbyte4_11_11_10_to_8_8_8( void* dst, const void* src, size_t );

	//-------------------------------------------------------------------------
	// Validation function declarations

	bool validateUnitLengthNormal( const void * src );
	bool validatePackingRangeFloatToInt16( const void * src );

	//-------------------------------------------------------------------------
	// Template conversion function declarations
	
	/// Conversion between types via assignment operator
	template < class DestType, class SourceType >		
	void assignmentConvert( void *dst, const void *src, size_t );

	/// Conversion between types using a normalization policy
	/// Must be specialized based on component types
	template < class DestType, class SourceType >
	void normalizedConvert( void* dst, const void* src, size_t );

	/// Conversion between types using a preset packing policy between the types
	/// Must be specialized based on component types
	template < class DestType, class SourceType >
	void packedConvert( void* dst, const void* src, size_t );

	//-------------------------------------------------------------------------
	// Helper struct forward declarations

	/// Prefer over direct function, since dest size is ensured as safe minimum.
	struct ReinterpretConverter
	{
		template < class DestType, class SourceType >
		static void convert( void *dst, const void *src, size_t );
	};


	/// Conversion Functors so we can supply conversion policy at compile time as
	/// template params. This is useful for the declarations in the implementation.
	/// Converts per component. Uses truncation/fill feature of ElementValue.
	/// Passes on component type template params to actual conversion function
	struct AssignmentComponentConverter
	{
		template <class DestType, class SourceType>
		static void convert( void* dst, const void* src, size_t copyCount);
	};

	struct NormalizedComponentConverter
	{
		template <class DestType, class SourceType>
		static void convert( void* dst, const void* src, size_t copyCount);
	};

	struct PackedComponentConverter
	{
		template <class DestType, class SourceType>
		static void convert( void* dst, const void* src, size_t copyCount );
	};


	/// Validating functor for checking source data on ElementValue
	/// combinations, similar to the Component Converters
	struct SourceDataComponentValidator
	{
		template <class DestType, class SourceType>
		static bool validate( const void* src );
	};

} // namespace VertexConversions

} // namespace Moo

BW_END_NAMESPACE


// template definitions
#include "vertex_format_conversions.ipp"


#endif // VERTEX_FORMAT_CONVERSIONS_HPP
