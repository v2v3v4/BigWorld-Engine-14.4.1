#include "pch.hpp"
#include "vertex_format_conversions.hpp"

#include "vertex_element_value.hpp"
#include "vertex_element_special_cases.hpp"
#include "moo_math.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
// -------------------------------------------------------------------------
// X Macros for storage types and semantics
// -------------------------------------------------------------------------

#define FOR_EACH_STORAGE_TYPE( X ) \
	X( FLOAT4, Float4 ) \
	X( FLOAT3, Float3 ) \
	X( FLOAT2, Float2 ) \
	X( FLOAT1, Float1 ) \
	X( UBYTE4, Ubyte4 ) \
	X( UBYTE3, Ubyte3 ) \
	X( UBYTE2, Ubyte2 ) \
	X( UBYTE1, Ubyte1 ) \
	X( UBYTE4_NORMAL_8_8_8, UByte4Normal_8_8_8 ) \
	X( SHORT4, Short4 ) \
	X( SHORT3, Short3 ) \
	X( SHORT2, Short2 ) \
	X( SHORT1, Short1 ) \
	X( COLOR, Color ) \
	X( SC_DIV3_III, ElementDiv3III ) \
	X( SC_REVERSE_III, ElementReverseIII ) \
	X( SC_REVERSE_PADDED_III_, ElementReversePaddedIII_ ) \
	X( SC_REVERSE_PADDED__WW_, ElementReversePadded_WW_ ) \
	X( SC_REVERSE_PADDED_WW_, ElementReversePaddedWW_ ) \


#define FOR_EACH_SEMANTIC_TYPE( X ) \
	X( UNKNOWN ) \
	X( POSITION ) \
	X( BLENDWEIGHT ) \
	X( BLENDINDICES ) \
	X( NORMAL ) \
	X( PSIZE ) \
	X( TEXCOORD ) \
	X( TANGENT ) \
	X( BINORMAL ) \
	X( TESSFACTOR ) \
	X( POSITIONT ) \
	X( COLOR ) \
	X( FOG ) \
	X( DEPTH ) \
	X( SAMPLE ) \


// -------------------------------------------------------------------------
// Section: Validation Function Definitions
// -------------------------------------------------------------------------
namespace detail 
{

/**
 * This function returns a ValidationFunc for the corresponding source to
 * destination storage type conversions. Some conversions such as packing
 * conversions map a reduced source range. These functions validate that the
 * source data is suitable for the conversion, and emit warnings where 
 * relevant.
 */

	template< class DestType, class SourceType >
	VertexConversions::ValidationFunc getValidateFunction()
	{
		return NULL;	
	}


	/// define a conversion using an ElementValue based validation functor
#define ENABLE_VALIDATION_TEMPLATE_FUNCTOR( dstType, srcType,	\
		validateFunctor )										\
	template <>													\
	VertexConversions::ValidationFunc getValidateFunction<		\
		VertexElement::dstType, VertexElement::srcType >()		\
	{															\
		return &validateFunctor::validate<						\
			VertexElement::dstType, VertexElement::srcType >;	\
	}															\

	/// This macro generates bidirectional specializations of 
	/// srcType <-> dstType using ElementValue based conversion functor
#define ENABLE_VALIDATION_TEMPLATE_FUNCTOR_2WAY( dstType, srcType, cfunct ) \
	ENABLE_VALIDATION_TEMPLATE_FUNCTOR( dstType, srcType, cfunct );			\
	ENABLE_VALIDATION_TEMPLATE_FUNCTOR( srcType, dstType, cfunct );

	/// This macro generates 4 directional specialization combinations of 
	/// srcType[1-4] -> dstType using ElementValue based conversion functor
#define ENABLE_VALIDATION_SRC_X4_TEMPLATE_FUNCTOR( dstType, srcType, cfunct ) \
	ENABLE_VALIDATION_TEMPLATE_FUNCTOR( dstType, srcType##1, cfunct );		\
	ENABLE_VALIDATION_TEMPLATE_FUNCTOR( dstType, srcType##2, cfunct );		\
	ENABLE_VALIDATION_TEMPLATE_FUNCTOR( dstType, srcType##3, cfunct );		\
	ENABLE_VALIDATION_TEMPLATE_FUNCTOR( dstType, srcType##4, cfunct ); 

	/// This macro generates 16 directional specialization combinations of 
	/// srcType[1-4] -> dstType[1-4] using ElementValue based conversion functor
#define ENABLE_VALIDATION_4X4_TEMPLATE_FUNCTOR( dstType, srcType, cfunct )	\
	ENABLE_VALIDATION_SRC_X4_TEMPLATE_FUNCTOR( dstType##1, srcType, cfunct ); \
	ENABLE_VALIDATION_SRC_X4_TEMPLATE_FUNCTOR( dstType##2, srcType, cfunct ); \
	ENABLE_VALIDATION_SRC_X4_TEMPLATE_FUNCTOR( dstType##3, srcType, cfunct ); \
	ENABLE_VALIDATION_SRC_X4_TEMPLATE_FUNCTOR( dstType##4, srcType, cfunct ); 

	//----------------------------------------------------------------
	// Put validation specializations here to enable them ...
	//----------------------------------------------------------------

	ENABLE_VALIDATION_4X4_TEMPLATE_FUNCTOR( Short, Float, 
		VertexConversions::SourceDataComponentValidator );


#undef ENABLE_VALIDATION_4X4_TEMPLATE_FUNCTOR
#undef ENABLE_VALIDATION_SRC_X4_TEMPLATE_FUNCTOR
#undef ENABLE_VALIDATION_TEMPLATE_FUNCTOR_2WAY
#undef ENABLE_VALIDATION_TEMPLATE_FUNCTOR


	/// Double dispatch across from non-templated builtinConvert to 
	/// select builtin conversion function
	template<class DestType>
	VertexConversions::ValidationFunc selectValidationTemplateFunction( 
		VertexElement::StorageType::Value srcType )
	{
		// Converting "srcType" runtime param into template param
		switch( srcType )
		{
#define RETURN_SRC_TYPE_SPEC_FUNCTION(storageTypeEnum, runtimeType)			\
		case VertexElement::StorageType::storageTypeEnum:						\
		return getValidateFunction< DestType, VertexElement::runtimeType >();

			FOR_EACH_STORAGE_TYPE( RETURN_SRC_TYPE_SPEC_FUNCTION );
#undef RETURN_SRC_TYPE_SPEC_FUNCTION
		default:
			return 0;
		}

		return 0;
	}

	/// Select builtin conversion function using runtime types
	VertexConversions::ValidationFunc selectValidationTemplateFunction( 
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType )
	{
		// Converting "dstType" runtime param into template param
		switch( dstType )
		{
#define RETURN_DEST_TYPE_SPEC_FUNCTION(storageTypeEnum, runtimeType)	\
	case VertexElement::StorageType::storageTypeEnum:					\
		return selectValidationTemplateFunction<							\
			VertexElement::runtimeType >( srcType );

			FOR_EACH_STORAGE_TYPE( RETURN_DEST_TYPE_SPEC_FUNCTION );
#undef RETURN_DEST_TYPE_SPEC_FUNCTION

		default:
			return 0;
		}

		return 0;
	}

	/// Fetch the default validate routine for runtime types
	/// that does not take semantic into account.
	VertexConversions::ValidationFunc fetchDefaultValidationFunction(
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType )
	{
		if (dstType == VertexElement::StorageType::UNKNOWN ||
			srcType == VertexElement::StorageType::UNKNOWN)
		{
			return NULL;
		}

		if (dstType == srcType)
		{
			return NULL;
		}

		return selectValidationTemplateFunction( dstType, srcType );
	}


	/// Opportunity to define semantic specific validation
	/// functions by specialising this function.
	template <VertexElement::SemanticType::Value Semantic>
	VertexConversions::ValidationFunc fetchValidationFunction( 
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType )
	{
		// Specialize this function for semantic specific validation
		return fetchDefaultValidationFunction( dstType, srcType );
	}


	// Normals Specialization
	template <>
	VertexConversions::ValidationFunc fetchValidationFunction< 
		VertexElement::SemanticType::NORMAL >( 
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType )
	{
		if (dstType == VertexElement::StorageType::UBYTE4 ||
			dstType == VertexElement::StorageType::UBYTE4_NORMAL_8_8_8)
		{
			if (srcType == VertexElement::StorageType::FLOAT3)
			{
				// float3 -> UBYTE4 or UBYTE4_NORMAL_8_8_8
				return &VertexConversions::validateUnitLengthNormal;
			}
		}
		return fetchDefaultValidationFunction( dstType, srcType );
	}

	/// BINORMAL Specialization: use NORMAL's behaviour
	template <>
	VertexConversions::ValidationFunc fetchValidationFunction< 
		VertexElement::SemanticType::BINORMAL >( 
		VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType )
	{
		return fetchValidationFunction< 
			VertexElement::SemanticType::NORMAL >( dstType, srcType );
	}


	/// TANGENT Specialization: use NORMAL's behaviour
	template <>
	VertexConversions::ValidationFunc fetchValidationFunction< 
		VertexElement::SemanticType::TANGENT >
		( VertexElement::StorageType::Value dstType, 
		VertexElement::StorageType::Value srcType )
	{
		return fetchValidationFunction< 
			VertexElement::SemanticType::NORMAL >( dstType, srcType );
	}


} // namespace detail

/// Select an appropriate conversion function based on 
/// runtime semantic and src,dst types.
VertexConversions::ValidationFunc
	VertexConversions::fetchValidationFunction( 
	VertexElement::SemanticType::Value semantic,
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	// Converting "semantic" runtime param into template param
	switch (semantic)
	{
#define RETURN_SEMANTIC_TYPE_SPEC_FUNCTION( value )						\
	case VertexElement::SemanticType::value:							\
		return detail::fetchValidationFunction<							\
			VertexElement::SemanticType::value >( dstType, srcType );

		FOR_EACH_SEMANTIC_TYPE( RETURN_SEMANTIC_TYPE_SPEC_FUNCTION );
#undef RETURN_SEMANTIC_TYPE_SPEC_FUNCTION
	}
	return NULL;
}

// -------------------------------------------------------------------------
// Section: Creation Function Definitions
// -------------------------------------------------------------------------
namespace detail 
{

/**
 * This function returns a pointer to a ConversionFunc, but is only invoked 
 * for determining specialised element creation in conversions where there
 * is no corresponding source element. The default behaviour is to do a memory
 * zero fill. You can override this default behaviour by specialising 
 * getDefaultValueFunction. To use a default constructor, you can use the
 * ENABLE_CTOR_CREATION_FUNC macro to define an override.
 */
template< class DestType >
VertexConversions::DefaultValueFunc getDefaultValueFunction()
{
	return &createWithZeroFill< DestType >;
}


template < class DestType >
void createWithZeroFill( void * dst, size_t )
{
	memset( dst, 0, sizeof(DestType) );
}

template < class DestType >
void createWithDefaultCtor( void * dst, size_t size )
{
	MF_ASSERT( size == sizeof(DestType) );
	new (dst) DestType;	
}

#define ENABLE_CTOR_CREATION_FUNC( dstType )					\
template <>														\
VertexConversions::DefaultValueFunc getDefaultValueFunction<	\
	VertexElement::dstType >()									\
{																\
	return &createWithDefaultCtor< VertexElement::dstType >;	\
}																\

//----------------------------------------------------------------
// Declare default values here to enable them ...

ENABLE_CTOR_CREATION_FUNC( ElementReversePadded_WW_ );
ENABLE_CTOR_CREATION_FUNC( ElementReversePaddedWW_ );

// For UBYTE2 BLENDWEIGHT initialisation
ENABLE_CTOR_CREATION_FUNC( ElementInitWW );

// For packed X888 normal/tangent/binormal zero-equiv init
ENABLE_CTOR_CREATION_FUNC( UByte4Normal_8_8_8 );

#undef ENABLE_CTOR_CREATION_FUNC


/// Fetch the default conversion routine for runtime types
/// that does not take semantic into account.
VertexConversions::DefaultValueFunc selectDefaultValueFunction(
	VertexElement::StorageType::Value dstType )
{
	// Converting "dstType" runtime param into template param
	switch( dstType )
	{
#define RETURN_CREATE_SPEC_FUNCTION(storageTypeEnum, runtimeType)		\
	case VertexElement::StorageType::storageTypeEnum:					\
		return getDefaultValueFunction< VertexElement::runtimeType >();

	FOR_EACH_STORAGE_TYPE( RETURN_CREATE_SPEC_FUNCTION )
#undef RETURN_CREATE_SPEC_FUNCTION

	default:
		return NULL;
	}

	return NULL;
}


/// Opportunity to define semantic specific creation
/// functions by specialising this function.
template < VertexElement::SemanticType::Value Semantic >
VertexConversions::DefaultValueFunc fetchDefaultValueFunction( 
	VertexElement::StorageType::Value dstType )
{
	// Specialize this function for semantic specific conversion
	return selectDefaultValueFunction( dstType );
}


template <>
VertexConversions::DefaultValueFunc fetchDefaultValueFunction< 
	VertexElement::SemanticType::BLENDWEIGHT > ( 
	VertexElement::StorageType::Value dstType )
{
	if ( dstType == VertexElement::StorageType::UBYTE2 )
	{
		return &createWithDefaultCtor< VertexElement::ElementInitWW >;
	}

	return selectDefaultValueFunction( dstType );
}

} // namespace detail


/// Select an appropriate conversion function based on 
/// runtime semantic and src,dst types.
VertexConversions::DefaultValueFunc 
	VertexConversions::fetchDefaultValueFunction( 
	VertexElement::SemanticType::Value semantic,
	VertexElement::StorageType::Value dstType )
{
	// Converting "semantic" runtime param into template param
	switch (semantic)
	{
#define RETURN_SEMANTIC_TYPE_SPEC_FUNCTION( value )				\
	case VertexElement::SemanticType::value:					\
		return detail::fetchDefaultValueFunction<				\
			VertexElement::SemanticType::value >( dstType );

	FOR_EACH_SEMANTIC_TYPE( RETURN_SEMANTIC_TYPE_SPEC_FUNCTION );
#undef RETURN_SEMANTIC_TYPE_SPEC_FUNCTION
	}
	return NULL;
}


// -------------------------------------------------------------------------
// Section: Conversion Function Definitions
// -------------------------------------------------------------------------

/// Memcpy based conversion function for fallback and dealing with 
/// data of the same type
void VertexConversions::reinterpretConvert( void *dst, const void *src, 
	size_t dstSize )
{
	memcpy( dst, src, dstSize );
}


/// Normals pack 
void VertexConversions::normalPackf3tob4( void* dst, const void* src, size_t dstSize )
{
	MF_ASSERT(sizeof( uint32 ) <= dstSize);
	*((uint32*)dst) = Moo::packNormal( *(const Vector3*)src );
}


/// Normals unpack 
void VertexConversions::normalUnpackb4tof3( void* dst, const void* src, size_t dstSize )
{
	*((Vector3*)dst) = Moo::unpackNormal( *(const uint32*)src );
}


void VertexConversions::packf3tob4Normal888( void* dst, const void* src, size_t dstSize )
{
	MF_ASSERT(sizeof( uint32 ) <= dstSize);
	*((uint32*)dst) = packFloat3toUByte4Normal888( *(const Vector3*)src );
}

void VertexConversions::unpackb4Normal888tof3( void* dst, const void* src, size_t dstSize )
{
	*((Vector3*)dst) = unpackUByte4Normal888toFloat3( *(const uint32*)src );
}


/// Normals packing conversion
void VertexConversions::convertPackedNormalUbyte4_8_8_8_to_11_11_10( 
	void* dst, const void* src, size_t dstSize )
{
	MF_ASSERT(sizeof( uint32 ) <= dstSize);
	*((uint32*)dst) = Moo::convertUByte4Normal_8_8_8_to_11_11_10( *(const uint32*)src );
}
void VertexConversions::convertPackedNormalUbyte4_11_11_10_to_8_8_8( 
	void* dst, const void* src, size_t dstSize )
{
	*((uint32*)dst) = Moo::convertUbyte4Normal_11_11_10_to_8_8_8( *(const uint32*)src );
}

/// Normals validation
bool VertexConversions::validateUnitLengthNormal( const void * src )
{
	return Moo::validateUnitLengthNormal( *(const Vector3*)src );
}

/// Texcoords validation
bool VertexConversions::validatePackingRangeFloatToInt16( const void * src )
{
	return Moo::validatePackingRangeFloatToInt16( *(float*)src );
}


namespace detail
{

/**
 * This function returns a pointer to a function that performs the conversion.
 * Defaults to a null pointer, which means the conversion is not enabled. 
 * Override with specialisation that returns valid fp. to enable a conversion.
 */
template< class DestType, class SourceType >
VertexConversions::ConversionFunc getConvertFunction()
{
	return 0;	
}

/// define a conversion using a simple function
#define ENABLE_CONVERSION_PLAIN_FUNC( dstType, srcType, conversionFunc )	\
	template <>																\
	VertexConversions::ConversionFunc getConvertFunction<					\
		VertexElement::dstType, VertexElement::srcType >()					\
	{																		\
		return &conversionFunc;												\
	}																		\

/// define a conversion using a VertexElement template conversion function
#define ENABLE_CONVERSION_TEMPLATE_FUNC( dstType, srcType, conversionFunc )	\
	template <>																\
	VertexConversions::ConversionFunc getConvertFunction<					\
		VertexElement::dstType, VertexElement::srcType >()					\
	{																		\
		return &conversionFunc<												\
			VertexElement::dstType, VertexElement::srcType >;				\
	}																		\

/// define a conversion using an ElementValue based conversion functor
#define ENABLE_CONVERSION_TEMPLATE_FUNCTOR( dstType, srcType,	\
		conversionFunctor )										\
	template <>													\
	VertexConversions::ConversionFunc getConvertFunction<		\
		VertexElement::dstType, VertexElement::srcType >()		\
	{															\
		return &conversionFunctor::convert<						\
			VertexElement::dstType, VertexElement::srcType >;	\
	}															\

/// This macro generates bidirectional specializations of 
/// srcType <-> dstType using ElementValue based conversion functor
#define ENABLE_CONVERSION_TEMPLATE_FUNCTOR_2WAY( dstType, srcType, cfunct ) \
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR( dstType, srcType, cfunct );			\
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR( srcType, dstType, cfunct );

/// This macro generates 4 directional specialization combinations of 
/// srcType[1-4] -> dstType using ElementValue based conversion functor
#define ENABLE_CONVERSION_SRC_X4_TEMPLATE_FUNCTOR( dstType, srcType, cfunct ) \
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR( dstType, srcType##1, cfunct );		\
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR( dstType, srcType##2, cfunct );		\
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR( dstType, srcType##3, cfunct );		\
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR( dstType, srcType##4, cfunct ); 

/// This macro generates 16 directional specialization combinations of 
/// srcType[1-4] -> dstType[1-4] using ElementValue based conversion functor
#define ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR( dstType, srcType, cfunct )	\
	ENABLE_CONVERSION_SRC_X4_TEMPLATE_FUNCTOR( dstType##1, srcType, cfunct ); \
	ENABLE_CONVERSION_SRC_X4_TEMPLATE_FUNCTOR( dstType##2, srcType, cfunct ); \
	ENABLE_CONVERSION_SRC_X4_TEMPLATE_FUNCTOR( dstType##3, srcType, cfunct ); \
	ENABLE_CONVERSION_SRC_X4_TEMPLATE_FUNCTOR( dstType##4, srcType, cfunct ); 

/// This macro generates 32 directional specialization combinations of 
/// srcType[1-4] <-> dstType[1-4] using ElementValue based conversion functor
#define ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR_2WAY( dstType, srcType, cfunct )\
	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR( dstType, srcType, cfunct );		\
	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR( srcType, dstType, cfunct ); 

/// This macro generates bidirectional specializations of 
/// srcType <-> dstType using plain conversion function
#define ENABLE_CONVERSION_PLAIN_FUNC_2WAY( dstType, srcType, cfunc )	\
	ENABLE_CONVERSION_PLAIN_FUNC( dstType, srcType, cfunc )				\
	ENABLE_CONVERSION_PLAIN_FUNC( srcType, dstType, cfunc ) 

/// This macro generates bidirectional specializations of 
/// srcType <-> dstType using VertexElement template conversion function
#define ENABLE_CONVERSION_TEMPLATE_FUNC_2WAY( dstType, srcType, cfunc )		\
	ENABLE_CONVERSION_TEMPLATE_FUNC( dstType, srcType, cfunc )				\
	ENABLE_CONVERSION_TEMPLATE_FUNC( srcType, dstType, cfunc ) 

	//----------------------------------------------------------------
	// Put conversion specializations here to enable them ...
	//----------------------------------------------------------------

	// e.g:
	// ENABLE_CONVERSION_PLAIN_FUNC( DestType, SourceType, reinterpretConvert );

	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR_2WAY( Ubyte, Float, 
		VertexConversions::NormalizedComponentConverter );

	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR_2WAY( Float, Short, 
		VertexConversions::PackedComponentConverter );

	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR( Float, Float, 
		VertexConversions::AssignmentComponentConverter );
	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR( Ubyte, Ubyte, 
		VertexConversions::AssignmentComponentConverter );
	ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR( Short, Short, 
		VertexConversions::AssignmentComponentConverter );
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR_2WAY( Ubyte4, Color, 
		VertexConversions::ReinterpretConverter );
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR_2WAY( Ubyte3, Color, 
		VertexConversions::AssignmentComponentConverter );
	ENABLE_CONVERSION_TEMPLATE_FUNCTOR_2WAY( Ubyte2, Color, 
		VertexConversions::AssignmentComponentConverter );

	ENABLE_CONVERSION_TEMPLATE_FUNC_2WAY( ElementReverseIII, Ubyte3, 
		VertexConversions::assignmentConvert );
	ENABLE_CONVERSION_TEMPLATE_FUNC_2WAY( ElementReversePaddedIII_, Ubyte3, 
		VertexConversions::assignmentConvert );
	ENABLE_CONVERSION_TEMPLATE_FUNC( ElementReverseIII, Float1, 
		VertexConversions::assignmentConvert );
	ENABLE_CONVERSION_TEMPLATE_FUNC( ElementReversePaddedIII_, Float1, 
		VertexConversions::assignmentConvert );
	ENABLE_CONVERSION_TEMPLATE_FUNC( ElementReversePaddedWW_, Ubyte2, 
		VertexConversions::assignmentConvert );
	ENABLE_CONVERSION_TEMPLATE_FUNC( ElementReversePadded_WW_, Ubyte2, 
		VertexConversions::assignmentConvert );
	
	ENABLE_CONVERSION_TEMPLATE_FUNC( Ubyte3, ElementDiv3III, 
		VertexConversions::assignmentConvert );
	

#undef ENABLE_CONVERSION_PLAIN_FUNC
#undef ENABLE_CONVERSION_TEMPLATE_FUNC
#undef ENABLE_CONVERSION_TEMPLATE_FUNCTOR
#undef ENABLE_CONVERSION_TEMPLATE_FUNCTOR_2WAY
#undef ENABLE_CONVERSION_SRC_X4_TEMPLATE_FUNCTOR
#undef ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR
#undef ENABLE_CONVERSION_4X4_TEMPLATE_FUNCTOR_2WAY
#undef ENABLE_CONVERSION_PLAIN_FUNC_2WAY
#undef ENABLE_CONVERSION_TEMPLATE_FUNC_2WAY


/// Double dispatch across from non-templated builtinConvert to 
/// select builtin conversion function
template<class DestType>
VertexConversions::ConversionFunc selectConvertTemplateFunction( 
	VertexElement::StorageType::Value srcType )
{
	// Converting "srcType" runtime param into template param
	switch( srcType )
	{
#define RETURN_SRC_TYPE_SPEC_FUNCTION(storageTypeEnum, runtimeType)			\
	case VertexElement::StorageType::storageTypeEnum:						\
		return getConvertFunction< DestType, VertexElement::runtimeType >();

	FOR_EACH_STORAGE_TYPE( RETURN_SRC_TYPE_SPEC_FUNCTION );
#undef RETURN_SRC_TYPE_SPEC_FUNCTION
	default:
		return 0;
	}

	return 0;
}

/// Select builtin conversion function using runtime types
VertexConversions::ConversionFunc selectConvertTemplateFunction( 
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	// Converting "dstType" runtime param into template param
	switch( dstType )
	{
#define RETURN_DEST_TYPE_SPEC_FUNCTION(storageTypeEnum, runtimeType)	\
	case VertexElement::StorageType::storageTypeEnum:					\
		return selectConvertTemplateFunction<							\
			VertexElement::runtimeType >( srcType );

	FOR_EACH_STORAGE_TYPE( RETURN_DEST_TYPE_SPEC_FUNCTION );
#undef RETURN_DEST_TYPE_SPEC_FUNCTION

	default:
		return 0;
	}

	return 0;
}

/// Fetch the default conversion routine for runtime types
/// that does not take semantic into account.
VertexConversions::ConversionFunc fetchDefaultConvertFunction(
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	if (dstType == VertexElement::StorageType::UNKNOWN ||
		srcType == VertexElement::StorageType::UNKNOWN)
	{
		return NULL;
	}

	VertexConversions::ConversionFunc result = NULL;
	if (dstType == srcType)
	{
		// Dealing with the same type, copy the data across directly
		return &VertexConversions::reinterpretConvert;
	}

	result = selectConvertTemplateFunction( dstType, srcType );
	return result;
}


/// Opportunity to define semantic specific conversion
/// functions by specialising this function.
template <VertexElement::SemanticType::Value Semantic>
VertexConversions::ConversionFunc fetchConvertFunction( 
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	// Specialize this function for semantic specific conversion
	return fetchDefaultConvertFunction( dstType, srcType );
}


// Position Specialization
template <>
VertexConversions::ConversionFunc fetchConvertFunction< 
	VertexElement::SemanticType::POSITION >( 
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	if (dstType == VertexElement::StorageType::FLOAT4)
	{
		if (srcType == VertexElement::StorageType::FLOAT3)
		{
			// float3 -> float4
			return &VertexConversions::positionAddWConversion;
		}
	}
	return fetchDefaultConvertFunction( dstType, srcType );
}


// Normal Specialization
template <>
VertexConversions::ConversionFunc fetchConvertFunction< 
	VertexElement::SemanticType::NORMAL >( 
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	if (dstType == VertexElement::StorageType::FLOAT3)
	{
		if (srcType == VertexElement::StorageType::UBYTE4 ||
			srcType == VertexElement::StorageType::UBYTE4N)
		{
			// UBYTE4 -> FLOAT3
			return &VertexConversions::normalUnpackb4tof3;
		}
		else if (srcType == VertexElement::StorageType::UBYTE4_NORMAL_8_8_8)
		{
			// UBYTE4_NORMAL_8_8_8 -> FLOAT3
			return &VertexConversions::unpackb4Normal888tof3;
		}
	}
	else if (dstType == VertexElement::StorageType::UBYTE4 ||
		dstType == VertexElement::StorageType::UBYTE4N)
	{
		if (srcType == VertexElement::StorageType::FLOAT3)
		{
			// FLOAT3 -> UBYTE4
			return &VertexConversions::normalPackf3tob4;
		}
		else if (srcType == VertexElement::StorageType::UBYTE4_NORMAL_8_8_8)
		{
			// UBYTE4_NORMAL_8_8_8 -> UBYTE4
			return &VertexConversions::convertPackedNormalUbyte4_8_8_8_to_11_11_10;
		}
	}
	else if (dstType == VertexElement::StorageType::UBYTE4_NORMAL_8_8_8)
	{
		if (srcType == VertexElement::StorageType::FLOAT3)
		{
			// FLOAT3 -> UBYTE4_NORMAL_8_8_8
			return &VertexConversions::packf3tob4Normal888;
		}
		else if (srcType == VertexElement::StorageType::UBYTE4 ||
				srcType == VertexElement::StorageType::UBYTE4N)
		{
			// UBYTE4 -> UBYTE4_NORMAL_8_8_8
			return &VertexConversions::convertPackedNormalUbyte4_11_11_10_to_8_8_8;
		}
	}

	return fetchDefaultConvertFunction( dstType, srcType );
}


/// BINORMAL Specialization: use NORMAL's behaviour
template <>
VertexConversions::ConversionFunc fetchConvertFunction< 
	VertexElement::SemanticType::BINORMAL >( 
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	return fetchConvertFunction< 
		VertexElement::SemanticType::NORMAL >( dstType, srcType );
}


/// TANGENT Specialization: use NORMAL's behaviour
template <>
VertexConversions::ConversionFunc fetchConvertFunction< 
	VertexElement::SemanticType::TANGENT >
	( VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	return fetchConvertFunction< 
		VertexElement::SemanticType::NORMAL >( dstType, srcType );
}


/// BLENDINDICES Specialization
template <>
VertexConversions::ConversionFunc fetchConvertFunction< 
	VertexElement::SemanticType::BLENDINDICES>( 
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	if (dstType == VertexElement::StorageType::UBYTE3)
	{
		if (srcType == VertexElement::StorageType::FLOAT1)
		{
			// FLOAT1 -> UBYTE3
			return &VertexElement::copyBlendIndicesUbyte3FromFloat1;
		}
	}
	return fetchDefaultConvertFunction( dstType, srcType );
}

} // namespace detail


void VertexConversions::positionAddWConversion( void* dst, const void* src, 
											   size_t )
{
	// do standard f3->f4 expansion
	VertexConversions::ConversionFunc defaultConvert = 
		detail::fetchDefaultConvertFunction( 
		VertexElement::StorageType::FLOAT3, 
		VertexElement::StorageType::FLOAT4 );
	defaultConvert( dst, src, 0 );

	// set w component
	reinterpret_cast< VertexElement::Float4 * >( dst )->w = 1.0f;
}


/// Select an appropriate conversion function based on 
/// runtime semantic and src,dst types.
VertexConversions::ConversionFunc VertexConversions::fetchConvertFunction( 
	VertexElement::SemanticType::Value semantic,
	VertexElement::StorageType::Value dstType, 
	VertexElement::StorageType::Value srcType )
{
	// Converting "semantic" runtime param into template param
	switch (semantic)
	{
#define RETURN_SEMANTIC_TYPE_SPEC_FUNCTION( value )						\
	case VertexElement::SemanticType::value:							\
		return detail::fetchConvertFunction<							\
			VertexElement::SemanticType::value >( dstType, srcType );

	FOR_EACH_SEMANTIC_TYPE( RETURN_SEMANTIC_TYPE_SPEC_FUNCTION );
#undef RETURN_SEMANTIC_TYPE_SPEC_FUNCTION
	}
	return NULL;
}


/// Perform conversion of a single value based upon
/// compiletime semantic, and runtime dst/src types
bool VertexConversions::convertValue( 
	VertexElement::SemanticType::Value Semantic, 
	void* dstData, VertexElement::StorageType::Value dstType, 
	const void* srcData, VertexElement::StorageType::Value srcType )
{
	VertexConversions::ConversionFunc convert = 
		fetchConvertFunction( Semantic, dstType, srcType );

	if (convert == NULL)
	{
		return false;
	}

	// Perform conversion
	(*convert)( dstData, srcData, typeSize( dstType ) );

	return true;
}


#undef FOR_EACH_SEMANTIC_TYPE
#undef FOR_EACH_STORAGE_TYPE

} // namespace Moo

BW_END_NAMESPACE

