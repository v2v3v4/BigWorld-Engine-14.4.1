#include "vertex_format_conversions.hpp"

#include "vertex_element_value.hpp"
#include "vertex_element_special_cases.hpp"
#include "moo_math.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

namespace VertexConversions
{

template < class DestType, class SourceType >		
inline void assignmentConvert( void *dst, const void *src, size_t )
{
	*((DestType*)dst) = *((const SourceType*)src);
}


template < class DestType, class SourceType >
inline void normalizedConvert( void* dst, const void* src, size_t )
{
	BW_STATIC_ASSERT(false, normalizedConvert_requires_type_specialization);
}


template <>
inline void normalizedConvert<float, uint8>( void* dst, const void* src, 
	size_t )
{
	*((float*)dst) = (*((uint8*)src) / 255.0f);
}


template <>
inline void normalizedConvert<uint8, float>( void* dst, const void* src, 
	size_t )
{
	*((uint8*)dst) = static_cast<uint8>(*((float*)src) * 255.0f);
}


template < class DestType, class SourceType >
inline void packedConvert( void* dst, const void* src, size_t )
{
	BW_STATIC_ASSERT(false, packedConvert_requires_type_specialization);
}

template <>
inline void packedConvert<int16, float>( void* dst, const void* src, 
											size_t )
{
	*((int16*)dst) = packFloatToInt16(*((float*)src));
}


template <>
inline void packedConvert<float, int16>( void* dst, const void* src, 
											size_t )
{
	*((float*)dst) = unpackInt16ToFloat(*((int16*)src));
}


template < class DestType, class SourceType >
void ReinterpretConverter::convert( void *dst, const void *src, size_t )
{
	size_t bytesToCopy = std::min( sizeof(DestType), sizeof(SourceType) );
	VertexConversions::reinterpretConvert( dst, src, bytesToCopy );
}


template <class DestType, class SourceType>
void AssignmentComponentConverter::convert( void* dst, const void* src, 
	size_t copyCount)
{
	VertexElement::copyComponents( (DestType*)dst, (SourceType*)src,
		copyCount, &assignmentConvert<
		typename DestType::value_type, typename SourceType::value_type> );
}


template <class DestType, class SourceType>
void NormalizedComponentConverter::convert( void* dst, const void* src, 
	size_t copyCount)
{
	VertexElement::copyComponents( (DestType*)dst, (SourceType*)src,
		copyCount, &normalizedConvert<
		typename DestType::value_type, typename SourceType::value_type> );
}

template <class DestType, class SourceType>
void PackedComponentConverter::convert( void* dst, const void* src, 
										size_t copyCount )
{
	VertexElement::copyComponents( (DestType*)dst, (SourceType*)src,
		copyCount, &packedConvert<
		typename DestType::value_type, typename SourceType::value_type> );
}

//-----------------------------------------------------------------------------
// Validation

template < class DestType, class SourceType >
inline bool validateValue( const void* src )
{
	BW_STATIC_ASSERT(false, validateValue_requires_type_specialization);
}

template <>
inline bool validateValue<int16, float>( const void* src )
{
	return Moo::validatePackingRangeFloatToInt16( *((float*)src) );
}


template <class DestType, class SourceType>
bool SourceDataComponentValidator::validate( const void* src )
{
	return VertexElement::validateComponents<SourceType>( (const SourceType*)src,
		&validateValue<typename DestType::value_type, typename SourceType::value_type>);
}


} // namespace VertexConversions

} // namespace Moo

BW_END_NAMESPACE

