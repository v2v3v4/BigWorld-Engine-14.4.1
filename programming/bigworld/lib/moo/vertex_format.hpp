#ifndef VERTEX_FORMAT_HPP
#define VERTEX_FORMAT_HPP

#include "moo_dx.hpp"
#include "com_object_wrap.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/concurrency.hpp"

#include "vertex_element.hpp"
#include "vertex_element_value.hpp"
#include "vertex_element_special_cases.hpp"
#include "vertex_format_conversions.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
typedef ComObjectWrap< DX::VertexDeclaration > D3DVertexDeclarationPtr;
class Vertices;

/**
 * This class defines a mapping between access types
 * and a VertexElement storage type.
 */
template <typename AccessType>
class VertexElementTypeTraits
{
public:
	/// Storage type that best represents this type
	static const VertexElement::StorageType::Value s_storageType = VertexElement::StorageType::UNKNOWN;
};

/**
 * This class defines a mapping between storage type
 * and the size that it takes up in a stream.
 */
template <VertexElement::StorageType::Value storageType>
class VertexElementStorageTypeTraits
{
public:
	static const size_t s_size = 0;
};

#define DECLARE_VERTEX_TYPE_MAPPING( type, storageType, componentCount )\
	template <> \
	class VertexElementTypeTraits< type >\
	{\
	public:\
		static const VertexElement::StorageType::Value s_storageType = VertexElement::StorageType::storageType;\
	};\
	DECLARE_VERTEX_TYPE_SIZE( storageType, sizeof( type ), componentCount )
#define DECLARE_VERTEX_TYPE_SIZE( storageType, size, componentCount )\
	template <> \
	class VertexElementStorageTypeTraits< VertexElement::StorageType::storageType >\
	{\
	public:\
		static const size_t s_size = size;\
		static const size_t s_components = componentCount;\
	}

// declare, but instead of also declaring a type size, check against existing one
#define DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( type, storageType, componentCount )\
	template <> \
	class VertexElementTypeTraits< type >\
	{\
	public:\
		static const VertexElement::StorageType::Value s_storageType = \
			VertexElement::StorageType::storageType;\
	private:\
		static void verify_alternate_mapping()\
		{\
			BW_STATIC_ASSERT(sizeof( type ) == \
				VertexElementStorageTypeTraits< \
					VertexElement::StorageType::storageType >::s_size, \
					VertexFormat_storage_type_alternate_declaration_size_mismatch); \
			BW_STATIC_ASSERT(componentCount == \
					VertexElementStorageTypeTraits< \
					VertexElement::StorageType::storageType >::s_components, \
					VertexFormat_storage_type_alternate_declaration_components_count_mismatch); \
		}\
	}


DECLARE_VERTEX_TYPE_MAPPING( float, FLOAT1, 1 );
DECLARE_VERTEX_TYPE_MAPPING( Vector2, FLOAT2, 2 );
DECLARE_VERTEX_TYPE_MAPPING( Vector3, FLOAT3, 3 );
DECLARE_VERTEX_TYPE_MAPPING( Vector4, FLOAT4, 4 );
DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( VertexElement::Float1, FLOAT1, 1 );
DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( VertexElement::Float2, FLOAT2, 2 );
DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( VertexElement::Float3, FLOAT3, 3 );
DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( VertexElement::Float4, FLOAT4, 4 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Ubyte1, UBYTE1, 1 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Ubyte2, UBYTE2, 2 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Ubyte3, UBYTE3, 3 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Ubyte4, UBYTE4, 4 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::UByte4Normal_8_8_8, UBYTE4_NORMAL_8_8_8, 3 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Short1, SHORT1, 1 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Short2, SHORT2, 2 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Short3, SHORT3, 3 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Short4, SHORT4, 4 );
DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( VertexElement::Short2Texcoord, SHORT2, 2);
DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE( uint32, UBYTE4, 4 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::Color, COLOR, 4 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::ElementDiv3III, SC_DIV3_III, 3 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::ElementReverseIII, SC_REVERSE_III, 3 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::ElementReversePaddedIII_, SC_REVERSE_PADDED_III_, 3 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::ElementReversePadded_WW_, SC_REVERSE_PADDED__WW_, 2 );
DECLARE_VERTEX_TYPE_MAPPING( VertexElement::ElementReversePaddedWW_, SC_REVERSE_PADDED_WW_, 2 );

DECLARE_VERTEX_TYPE_SIZE( UBYTE4N, 4, 4 );
DECLARE_VERTEX_TYPE_SIZE( SHORT2N, 4, 2 );
DECLARE_VERTEX_TYPE_SIZE( SHORT4N, 8, 4 );
DECLARE_VERTEX_TYPE_SIZE( USHORT2N, 4, 2 );
DECLARE_VERTEX_TYPE_SIZE( USHORT4N, 8, 4 );
DECLARE_VERTEX_TYPE_SIZE( UDEC3, 4, 3 );
DECLARE_VERTEX_TYPE_SIZE( DEC3N, 4, 3 );
DECLARE_VERTEX_TYPE_SIZE( HALF2, 4, 2 );
DECLARE_VERTEX_TYPE_SIZE( HALF4, 8, 4 );

#undef DECLARE_VERTEX_TYPE_MAPPING_ALTERNATE
#undef DECLARE_VERTEX_TYPE_MAPPING
#undef DECLARE_VERTEX_TYPE_SIZE


/**
 * This class is used to access an element of a stream
 * as any type and perform the conversion implicitly.
 * This allows some algorithms to be written without
 * knowledge of the underlying data format.
 */
template <VertexElement::SemanticType::Value Semantic>
class VertexElementRef 
{
public:
	typedef VertexElementRef<Semantic> SelfType;
	typedef VertexElement::StorageType StorageType;

	VertexElementRef( void* buffer, VertexElement::StorageType::Value type )
		: buffer_( buffer )
		, type_( (VertexElement::StorageType::Value)type )
	{

	}

	template <class AccessType>
	operator AccessType() const
	{
		AccessType destData;
		StorageType::Value destType = VertexElementTypeTraits<AccessType>::s_storageType;
		
		// Check that the conversion is valid
		MF_VERIFY( VertexConversions::convertValue( Semantic, 
			&destData, destType, buffer_, type_ ) );

		return destData;
	}

	template <class AccessType>
	const AccessType& operator=( const AccessType& value)
	{
		StorageType::Value sourceType =  VertexElementTypeTraits<AccessType>::s_storageType;

		// Check that the conversion is valid
		MF_VERIFY( VertexConversions::convertValue( Semantic, 
			buffer_, type_, &value, sourceType ) );

		return value;
	}

	const SelfType& operator=( const SelfType& value)
	{
		StorageType::Value sourceType = value.type_;

		// Check that the conversion is valid
		MF_VERIFY( VertexConversions::convertValue( Semantic, 
			buffer_, type_, &value, sourceType ) );

		return value;
	}

private:
	StorageType::Value type_;
	void* buffer_;
};

/**
 * This class allows elements in a stream to
 * be accessed using a specific type. No validation
 * of the access is performed, but iteration of
 * verts is taken care of with stride and element
 * offset taken into account.
 */
template < class Type >
class RawElementAccessor
{
public:
	typedef RawElementAccessor<Type> SelfType;

	RawElementAccessor( void* buffer, size_t size, size_t offset, size_t stride );

	const Type& operator[]( uint32 index ) const;
	Type& operator[]( uint32 index );
	
	size_t size() const;
	bool isValid() const;

protected:
	uint8* buffer_;
	size_t size_;
	size_t offset_;
	size_t stride_;
};

/**
 * This class allows elements in a stream to be 
 * iterated and accessed as any type that a conversion
 * exists for. This allows algorithms to be written
 * to operate on the data without needing to know
 * the underlying data format.
 */
template < VertexElement::SemanticType::Value Semantic >
class ProxyElementAccessor
{
public:
	typedef VertexElementRef<Semantic> ElementRefType;
	typedef ProxyElementAccessor<Semantic> SelfType;
	typedef VertexElement::StorageType StorageType;

	ProxyElementAccessor();
	ProxyElementAccessor( void* buffer, size_t size, size_t offset, size_t stride, 
		StorageType::Value type );

	bool isValid() const;
	size_t size() const;
	const ElementRefType operator[]( uint32 index ) const;
	ElementRefType operator[]( uint32 index );

private:
	uint8* buffer_;
	size_t offset_;
	size_t stride_;
	size_t size_;
	StorageType::Value type_;
};

/**
 * This class defines the vertex data format of 
 * a set of vertex streams. Streams are made up of
 * a number of elements.
 */
class VertexFormat
{
public:
	typedef VertexElement::StorageType StorageType;
	typedef VertexElement::SemanticType SemanticType;
	static const uint32 DEFAULT = uint32(-1);

	VertexFormat();
	~VertexFormat();

	VertexFormat( const BW::StringRef & name );

	/// equality comparison
	bool equals( const VertexFormat & other ) const;
	bool operator==( const VertexFormat & other ) const;

	/// Get name of format definition this format was loaded from
	const BW::StringRef name() const;

	/// Load XML data into the Vertex format
	static bool load( VertexFormat& format, DataSectionPtr pSection,
		const BW::StringRef & name );

	/// Merge another format into the current vertex format
	static bool merge( VertexFormat& format, const VertexFormat& otherFormat );

	/// Add a new stream to the format.
	uint32 addStream();

	/// Add an element to a stream.
	void addElement( uint32 streamIndex, SemanticType::Value semantic, 
		StorageType::Value storageType, uint32 semanticIndex = DEFAULT, size_t offset = DEFAULT );

	/// Find the Nth element in the format and return information on it
	bool findElement( uint32 index, SemanticType::Value* semantic, 
		uint32* streamIndex, uint32* semanticIndex, size_t* offset, StorageType::Value* storageType ) const;
	
	/// Find the Nth element in the given stream and return information on it
	bool findElement( uint32 streamIndex, uint32 index, SemanticType::Value* semantic, 
		uint32* semanticIndex, size_t* offset, StorageType::Value* storageType ) const;

	/// Find the Nth element in the format of a specific semantic and return info on it
	bool findElement( SemanticType::Value semantic, uint32 semanticIndex,
		uint32* streamIndex, size_t* offset, StorageType::Value* storageType ) const;

	/// Find the Nth element in the given stream of a specific semantic and return info on it
	bool findElement( uint32 streamIndex, SemanticType::Value semantic, uint32 semanticIndex,
		size_t* offset, StorageType::Value* storageType ) const;

	/// Check if an element exists in the format
	bool containsElement( SemanticType::Value semantic, uint32 semanticIndex ) const;

	/// Count the total number of elements in the format
	uint32 countElements( ) const;

	/// Count the total number of elements in the stream
	uint32 countElements( uint32 streamIndex ) const;

	/// Count the total number of elements with the given semantic
	uint32 countElements( SemanticType::Value semantic ) const;

	/// Count the total number of elements in the stream with the semantic
	uint32 countElements( uint32 streamIndex, SemanticType::Value semantic ) const;

	/// Get the stride of a particular stream
	uint32 streamStride( uint32 streamIndex ) const;

	/// Get the number of streams in the format
	uint32 streamCount() const;

	/// Get the size of a given storage type
	static uint32 typeSize( StorageType::Value type );

	/// Check if the vertex format contains all N provided semantics in the
	/// given stream. If no stream index or semantic index is DEFAULT, it 
	/// checks any available stream index or semantic index respectively.
	bool containsSemantics( const SemanticType::Value * semantics, 
		size_t numSemantics, uint32 semanticIndex, uint32 streamIndex ) const;
	
	/// Create a proxy element accessor to access the element with the given semantic and index
	template <SemanticType::Value Semantic>
	ProxyElementAccessor<Semantic> createProxyElementAccessor( void* buffer, 
		size_t bufferSize, uint32 streamIndex, uint32 semanticIndex ) const;

	/// Create a const proxy element accessor to access the element with the given semantic and index
	template <SemanticType::Value Semantic>
	const ProxyElementAccessor<Semantic> createProxyElementAccessor( const void* buffer, 
		size_t bufferSize, uint32 streamIndex, uint32 semanticIndex ) const;

	/// Create a raw element accessor to access the element with the given data offset
	template <class AccessType>
	RawElementAccessor<AccessType> createRawElementAccessor( 
		void* buffer, size_t bufferSize, uint32 streamIndex, size_t offset ) const;

	/// A definition of a buffer for conversion purposes
	struct Buffer
	{
		Buffer( void* data, size_t size );

		void* data_;
		size_t size_;
	};

	/// A definition of a buffer set for conversion purposes
	struct BufferSet
	{
		BufferSet( const VertexFormat& format );

		BW::vector<Buffer> buffers_;
		const VertexFormat& format_;
	};
	
	/// Fill elements using the value returned by the provided function.
	bool fillElements( void* dstData, size_t dstSize, uint32 streamIndex, 
		uint32 index, VertexConversions::DefaultValueFunc defaultValFunc ) const;

	/// Determine if the src format can be converted to the dst format.
	static bool canConvert( const BufferSet & dstBuffers, 
		const BufferSet & srcBuffers, bool createFailureMessages = false );

	/**
	 * Perform conversion between the src buffers to the dst buffers.
	 *	@param	dstBuffers		The destination buffer set
	 *	@param	srcBuffers		The source buffer set
	 *	@param	initialiseMissing	For destination elements which do not have 
	 *								a corresponding source element, fill using 
	 *								the type's default value function.
	 */
	static bool convert( const BufferSet& dstBuffers, 
		const BufferSet& srcBuffers, bool initialiseMissing = true );

	/// Perform a conversion between a src and dst buffer.
	/// Will fail if conversion involves multiple streams.
	static bool convertBuffer( 
		const VertexFormat & srcFormat, const void * srcData, size_t srcSize,
		const VertexFormat & dstFormat, void * dstData, size_t dstSize,
		bool createFailureMessages = false );

	/**
	 * Perform a conversion between a src and dst buffer for
	 * a specific supplied source and destination stream.
	 * Will fail if conversion involves multiple streams.
	 */
	static bool convertBufferStream( 
		const VertexFormat & srcFormat, const void * srcData, size_t srcSize, uint32 srcStreamIndex,
		const VertexFormat & dstFormat, void * dstData, size_t dstSize, uint32 dstStreamIndex,
		bool createFailureMessages = false );


	/**
	 * Is the source data suitable for the conversion?
	 * Runs validation functions on the source data to ensure that it meets
	 * the data requirements (e.g. range) of relevant element conversions.
	 *
	 *	@param	failOnFirstInvalid		stop processing on first invalid vertex.
	 *	@param	failOnFirstInvalidPerElement	stop processing future vertices
	 *											for this element index, but move
	 *											on to next element index so that
	 *											we get warnings on all bad elements 
	 *
	 *	If both fail params are set true, failOnFirstInvalid takes precedence, 
	 */
	static bool isSourceDataValid( const VertexFormat & dstFormat, const BufferSet & srcBuffers,
		bool failOnFirstInvalid, bool failOnFirstInvalidPerElement );

	/// Get the name of the target format for the given target.
	BW::StringRef getTargetFormatName( const BW::StringRef & targetName ) const;


	/**
	 * This class executes conversions between two distinct formats. 
	 * Its primary utility is syntactic convenience to reduce boilerplate
	 * checks and setup for the most common conversion cases.
	 *
	 * If a null dest format is supplied, or dest format equiv to source.
	 * format, then does a fast copy instead of conversion.
	 */
	class ConversionContext
	{
	public:
		ConversionContext();
		ConversionContext( const VertexFormat * dstFormat, const VertexFormat * srcFormat );

		const VertexFormat * dstFormat() const;
		const VertexFormat * srcFormat() const;

		/// get the vertex size for the first source stream
		const uint32 srcVertexSize( uint32 streamIndex=0 ) const;

		/// get the vertex size for the first destination stream
		const uint32 dstVertexSize( uint32 streamIndex=0 ) const;

		/// are the source and dest formats valid
		bool isValid() const;

		/// is a conversion actually required
		bool isConversionRequired() const;

		/// perform a conversion that uses a single stream of data
		bool convertSingleStream( void * dstData, const void * srcData,  
								size_t numVertices ) const;

		/// perform a conversion that uses a single stream of data
		/// on specific stream indexes.
		bool convertSingleStream( 
			void * dstData, uint32 dstStreamIndex, 
			const void * srcData, uint32 srcStreamIndex,  
			size_t numVertices ) const;

	protected:
		const VertexFormat * dstFormat_;
		const VertexFormat * srcFormat_;

	};

protected:
	typedef BW::StringRefMap< BW::string > TargetFormatMapping;

	void addFormatTarget( const BW::StringRef & targetName, 
		const BW::StringRef & formatName );

	struct ElementDefinition
	{
		ElementDefinition( SemanticType::Value semantic, 
			uint32 semanticIndex,
			StorageType::Value storage,
			size_t offset );

		SemanticType::Value semantic_;
		uint32 semanticIndex_;
		StorageType::Value storage_;
		size_t offset_;
	};

	struct StreamDefinition
	{
		StreamDefinition();

		BW::vector<ElementDefinition> elements_;
		uint32 stride_;
	};

	/// Set of streams in this format.
	BW::vector<StreamDefinition> streams_;

	/// Mapping of target name to vertex format name
	TargetFormatMapping targets_;

	/// Name of this format based on the definition it was loaded from
	BW::string name_;
};

} // namespace Moo

#include "vertex_format.ipp"

BW_END_NAMESPACE

#endif // VERTEX_FORMAT_HPP
