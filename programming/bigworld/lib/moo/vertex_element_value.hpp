#ifndef VERTEX_ELEMENT_VALUE_HPP
#define VERTEX_ELEMENT_VALUE_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"
#include "moo_math.hpp"
#include "vertex_element.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
namespace VertexElement
{
// -----------------------------------------------------------------------------
// Declare some stuff from VertexConversions, so we do not need to include it
// -----------------------------------------------------------------------------

typedef void (*ConversionFunc)( void *, const void *, size_t );
typedef bool (*ValidationFunc)( const void * );

template<class DestinationType, class SourceType>		
static void assignmentConvert( void * dst, const void * src, size_t )
{
	reinterpret_cast< DestinationType * >(dst)->operator=( 
		*(reinterpret_cast< const SourceType* >(src)) );
}


// -----------------------------------------------------------------------------
// Section: MultiComponentValue
// -----------------------------------------------------------------------------

/**
 * This class represents a value that is made up of a homogeneous
 * array of N "component" values. The size and type of component is
 * known at compile time.
 * Provides convenience named access variables [xyzw], [rgba] for N=[1-4].
 */
template < class T, unsigned int ComponentCount >
struct MultiComponentValue
{
	T components_[ComponentCount];
};

// for [1-4] components, provide [x,y,z,w] style named access.
// Also provides [b,g,r,a] aliases matching D3DCOLOR representation.

template < class T >
struct MultiComponentValue< T, 1 >
{
	union {
		T components_[1];
		struct { T x; };
		struct { T b; };
	};
};

template < class T >
struct MultiComponentValue< T, 2 >
{
	union {
		T components_[2];
		struct { T x, y; };
		struct { T b, g; };
	};
};

template < class T >
struct MultiComponentValue< T, 3 >
{
	union {
		T components_[3];
		struct { T x, y, z; };
		struct { T b, g, r; };
	};
};

template < class T >
struct MultiComponentValue< T, 4 >
{
	union {
		T components_[4];
		struct { T x, y, z, w; };
		struct { T b, g, r, a; };
	};
};


// -----------------------------------------------------------------------------
// Section: ElementValue and typedefs
// -----------------------------------------------------------------------------

/**
 * This class represents the storage and access of a vertex element value, and the
 * components each vertex element consists of (e.g. x,y,z as floats, or r,g,b,a as color).
 * It is essentially a fixed array with type and size known at compile time.
 *
 * However, it provides component-wise named and array access (both compile 
 * time, and run time), and has various functions for casting between different
 * Component types and sizes, for supporting conversion between different formats
 * and storage/packing representations.
 */
/// Value type that provides a component-based writeable view of arbitrary data.
template < class ComponentType, unsigned int ComponentCount >
struct ElementValue : public MultiComponentValue< ComponentType, ComponentCount >
{
	typedef ComponentType value_type;
	typedef ElementValue< ComponentType, ComponentCount > self_type;

	static const unsigned int component_size = sizeof(ComponentType);
	static const unsigned int component_count = ComponentCount;
	static const unsigned int size_bytes = component_size * component_count;

	/// Copies bytes from passed in objects to itself, clamping the copy range
	/// to not exceed the size of target component data
	template < class OtherType >
	void copyBytesFrom( const OtherType & srcData )
	{
		BW_STATIC_ASSERT( component_count > 0 && component_size > 0, ElementValue_zero_sized_instantiation_is_invalid );
		unsigned int bytesToCopy = std::min( size_bytes, (unsigned int)sizeof(srcData) );
		memcpy( &components_, &srcData, bytesToCopy );
	}

	/// Compile time component indexing
	template < unsigned int ComponentIndex >
	value_type & get()
	{
		BW_STATIC_ASSERT( ComponentIndex < component_count, ElementValue_index_out_of_bounds );
		return components_[ComponentIndex];
	}

	template < unsigned int ComponentIndex >
	const value_type & get() const
	{
		return const_cast< self_type* >(this)->get<ComponentIndex>();
	}

	/// Run time component indexing
	value_type & operator[]( uint32 index )
	{
		MF_ASSERT(index < component_count);
		return components_[index];
	}

	const value_type & operator[]( uint32 index ) const
	{
		return (*const_cast<self_type*>(this))[index];
	}

	bool operator==( const self_type & other ) const
	{
		return memcmp( components_, other.components_, size_bytes ) == 0;
	}

	/// Fill final N components with a value, int version uses memset
	void fillEndComponents( unsigned int count, int fillVal )
	{
		count = std::min( count, component_count );
		memset( &components_[component_count-count], fillVal, count*component_size );
	}

	/// Fill final N components with a value
	void fillEndComponents( unsigned int count, const value_type & fillVal )
	{
		count = std::min( count, component_count );
		unsigned int i = component_count - count;
		for (; i < component_count; ++i)
		{
			components_[i] = fillVal;
		}
	}

	/// Fill all components with a value
	void fillComponents( int fillval )
	{
		return fillEndComponents( component_count, fillval );
	}

	void fillComponents( const value_type & fillVal )
	{
		return fillEndComponents( component_count, fillval );
	}
};


// -----------------------------------------------------------------------------
// Section: Standalone copy/conversion functions
// -----------------------------------------------------------------------------

/**
 * Copy components of different type (and potentially count).
 * User can provide a ConversionFunc compatible function that will be
 * applied per source component.
 */
template < class DestElementVal, class SourceElementVal >
static void copyComponents(
	DestElementVal * dst, const SourceElementVal * src, size_t copyCount,
	ConversionFunc componentConversionFunc = 
		&assignmentConvert< DestElementVal::value_type, 
		SourceElementVal::value_type > )
{
	if (copyCount == 0)
	{
		copyCount = DestElementVal::component_count;
	}

	// The final amount of components we will copy is the maximum available 
	// from source (respecting the copyCount) that we can fit in the dest
	unsigned int componentsToCopy = std::min( std::min( 
		DestElementVal::component_count, SourceElementVal::component_count), 
		static_cast<unsigned int>(copyCount) );

	for (unsigned int i = 0; i < componentsToCopy; ++i)
	{
		// pass the total bytes to write as the last param if it is used.
		componentConversionFunc( &(*dst)[i], &(*src)[i], 
			DestElementVal::component_size );
	}

	unsigned int componentsLeft = 
		DestElementVal::component_count - componentsToCopy;
	if (componentsLeft)
	{
		dst->fillEndComponents( componentsLeft, 0 );
	}
}

/**
 * Copy components between ElementValues with the same component type 
 * (but possibly different count). In this case components are directly 
 * memcpy-ed in one go, and no conversion function is used. If there is 
 * a case where a same-component type copy is desired, with a custom 
 * conversion step, the calling code should supply a 4th argument which 
 * will call the different type variant of this function. Not that this 
 * might not be safe for non-trivial component types with custom ctors.
 */
template < template < class, unsigned int > class ElementVal, 
	class ElementType, unsigned int DstComponentCount, 
	unsigned int SrcComponentCount >
static void copyComponents( ElementVal< ElementType, DstComponentCount > * dst, 
	const ElementVal< ElementType, SrcComponentCount > * src, size_t copyCount )
{
	typedef ElementVal< ElementType, DstComponentCount > DestElementVal;

	if (copyCount == 0)
	{
		copyCount = DstComponentCount;
	}

	unsigned int componentsToCopy = std::min( std::min( 
		DstComponentCount, SrcComponentCount), 
		static_cast<unsigned int>(copyCount) );
	memcpy( dst, src, componentsToCopy * DestElementVal::component_size );

	unsigned int componentsLeft = 
		DestElementVal::component_count - componentsToCopy;
	if (componentsLeft)
	{
		dst->fillEndComponents( componentsLeft, 0 );
	}
}

/**
 * Validate all components of an ElementValue using the provided 
 * validation function. Returns true if all components are valid.
 */
template <class SourceElementVal>
static bool validateComponents( const SourceElementVal* src, ValidationFunc validationFunc )
{
	for (unsigned int i = 0; i < SourceElementVal::component_count; ++i)
	{
		if (!validationFunc( &(*src)[i] ))
		{
			return false;
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
// Section: ElementValue typedefs
// -----------------------------------------------------------------------------

typedef ElementValue< uint8, 1 > Ubyte1;
typedef ElementValue< uint8, 2 > Ubyte2;
typedef ElementValue< uint8, 3 > Ubyte3;
typedef ElementValue< uint8, 4 > Ubyte4;

typedef ElementValue< float, 1 > Float1;
typedef ElementValue< float, 2 > Float2;
typedef ElementValue< float, 3 > Float3;
typedef ElementValue< float, 4 > Float4;

typedef ElementValue< int16, 1 > Short1;
typedef ElementValue< int16, 2 > Short2;
typedef ElementValue< int16, 3 > Short3;
typedef ElementValue< int16, 4 > Short4;

class Color : public Ubyte4 {};

class UByte4Normal_8_8_8 : public Moo::PackedVector3NormalUByte4_8_8_8 
{
public:
	// Provide "zeroing" construction for conversions only.
	// More heavyweight than default (incorrect) zeroing since packed type.
	// see VertexConversions::DefaultValueFunc
	UByte4Normal_8_8_8();
};

class Short2Texcoord : public Moo::PackedVector2TexcoordInt16_X2 {};


} // namespace VertexElement


} // namespace Moo

BW_END_NAMESPACE

#endif // VERTEX_ELEMENT_VALUE_HPP
