#include "pch.hpp"
#include "vertex_declaration.hpp"
#include "vertex_format_cache.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: VertexDeclaration
// -----------------------------------------------------------------------------

namespace Moo
{

SimpleMutex	VertexDeclaration::declarationsLock_;

char VertexDeclaration::s_targetDevice[16] = "D3D9";

const char* VertexDeclaration::getTargetDevice() 
{ 
	return &s_targetDevice[0]; 
}

void VertexDeclaration::setTargetDevice( const char * target )
{
	MF_ASSERT( strlen( target ) < (ARRAY_SIZE( s_targetDevice ) - 1) );
	strcpy( &s_targetDevice[0], target );
}


// StorageType -> DeclType 

typedef VertexElement::StorageType Stor;
typedef D3DDECLTYPE DeclType;

typedef std::pair< VertexElement::StorageType::Value, DeclType > DeclTypeEntry;

DeclTypeEntry s_declTypes[] = {
	DeclTypeEntry( Stor::FLOAT1, D3DDECLTYPE_FLOAT1 ),
	DeclTypeEntry( Stor::FLOAT2, D3DDECLTYPE_FLOAT2 ),
	DeclTypeEntry( Stor::FLOAT3, D3DDECLTYPE_FLOAT3 ),
	DeclTypeEntry( Stor::FLOAT4, D3DDECLTYPE_FLOAT4 ),
	DeclTypeEntry( Stor::UBYTE4N, D3DDECLTYPE_UBYTE4N ),
	DeclTypeEntry( Stor::UBYTE4, D3DDECLTYPE_UBYTE4 ),
	DeclTypeEntry( Stor::UBYTE4_NORMAL_8_8_8, D3DDECLTYPE_UBYTE4 ),
	DeclTypeEntry( Stor::SHORT2, D3DDECLTYPE_SHORT2 ),
	DeclTypeEntry( Stor::SHORT4, D3DDECLTYPE_SHORT4 ),
	DeclTypeEntry( Stor::SHORT2N, D3DDECLTYPE_SHORT2N ),
	DeclTypeEntry( Stor::SHORT4N, D3DDECLTYPE_SHORT4N ),
	DeclTypeEntry( Stor::USHORT2N, D3DDECLTYPE_USHORT2N ),
	DeclTypeEntry( Stor::USHORT4N, D3DDECLTYPE_USHORT4N ),
	DeclTypeEntry( Stor::UDEC3, D3DDECLTYPE_UDEC3 ),
	DeclTypeEntry( Stor::DEC3N, D3DDECLTYPE_DEC3N ),
	DeclTypeEntry( Stor::COLOR, D3DDECLTYPE_D3DCOLOR ),
	DeclTypeEntry( Stor::HALF2, D3DDECLTYPE_FLOAT16_2 ),
	DeclTypeEntry( Stor::HALF4, D3DDECLTYPE_FLOAT16_4 ),

	// Special cases
	DeclTypeEntry( Stor::SC_REVERSE_PADDED_III_, D3DDECLTYPE_D3DCOLOR ),
	DeclTypeEntry( Stor::SC_REVERSE_PADDED__WW_, D3DDECLTYPE_D3DCOLOR ),
	DeclTypeEntry( Stor::SC_REVERSE_PADDED_WW_, D3DDECLTYPE_D3DCOLOR )
};


/**
 * This function returns a declaration type based given a string name. Returns
 * "unused" declaration type if string doesn't match.
 */
static DeclType type( const VertexElement::StorageType::Value& type )
{
	for (size_t i = 0; i < ARRAYSIZE( s_declTypes ); ++i)
	{
		const DeclTypeEntry & entry = s_declTypes[i];
		if (entry.first == type)
		{
			return entry.second;
		}
	}

	WARNING_MSG( "VertexDeclaration::type: unable to find declaration type %d (%s)\n", 
		type, VertexElement::StringFromStorage( type ).to_string().c_str() );
	return D3DDECLTYPE_UNUSED;
}

bool VertexDeclaration::isTypeSupportedOnDevice( 
	const VertexElement::StorageType::Value& storageType )
{
#define DECLARE_UNSUPPORTED_TYPE( storageTypeEnum ) \
	case VertexElement::StorageType::storageTypeEnum: return false

	switch( storageType )
	{
		//----------------------------------------------------------------
		// Add unsupported types here...
		//----------------------------------------------------------------
		DECLARE_UNSUPPORTED_TYPE( UBYTE3 );
		DECLARE_UNSUPPORTED_TYPE( UBYTE2 );
		DECLARE_UNSUPPORTED_TYPE( UBYTE1 );
		DECLARE_UNSUPPORTED_TYPE( SHORT1 );
		DECLARE_UNSUPPORTED_TYPE( SHORT3 );
		DECLARE_UNSUPPORTED_TYPE( SC_DIV3_III );
		DECLARE_UNSUPPORTED_TYPE( SC_REVERSE_III );

	default:
		return (type( storageType ) != D3DDECLTYPE_UNUSED);
	}
#undef DECLARE_UNSUPPORTED_TYPE
}

// SemanticType -> DeclUsage 

typedef VertexElement::SemanticType Sem;
typedef D3DDECLUSAGE DeclUsage;

typedef std::pair< VertexElement::SemanticType::Value, DeclUsage > DeclUsageEntry;

DeclUsageEntry s_declUsages[] = 
{
	DeclUsageEntry( Sem::POSITION, D3DDECLUSAGE_POSITION ),
	DeclUsageEntry( Sem::BLENDWEIGHT, D3DDECLUSAGE_BLENDWEIGHT ),
	DeclUsageEntry( Sem::BLENDINDICES, D3DDECLUSAGE_BLENDINDICES ),
	DeclUsageEntry( Sem::NORMAL, D3DDECLUSAGE_NORMAL ),
	DeclUsageEntry( Sem::PSIZE, D3DDECLUSAGE_PSIZE ),
	DeclUsageEntry( Sem::TEXCOORD, D3DDECLUSAGE_TEXCOORD ),
	DeclUsageEntry( Sem::TANGENT, D3DDECLUSAGE_TANGENT ),
	DeclUsageEntry( Sem::BINORMAL, D3DDECLUSAGE_BINORMAL ),
	DeclUsageEntry( Sem::TESSFACTOR, D3DDECLUSAGE_TESSFACTOR ),
	DeclUsageEntry( Sem::POSITIONT, D3DDECLUSAGE_POSITIONT ),
	DeclUsageEntry( Sem::COLOR, D3DDECLUSAGE_COLOR ),
	DeclUsageEntry( Sem::FOG, D3DDECLUSAGE_FOG ),
	DeclUsageEntry( Sem::DEPTH, D3DDECLUSAGE_DEPTH ),
	DeclUsageEntry( Sem::SAMPLE, D3DDECLUSAGE_SAMPLE )
};


/**
 * This function returns a D3D usage value given a string name. It will return
 * (MAXD3DDECLUSAGE + 1) if name match isn't found. 
 */
static D3DDECLUSAGE usage( const VertexElement::SemanticType::Value & usage )
{
	for (size_t i = 0; i < ARRAYSIZE( s_declUsages ); ++i)
	{
		const DeclUsageEntry & entry = s_declUsages[i];
		if (entry.first == usage)
		{
			return entry.second;
		}
	}

	WARNING_MSG( "VertexDeclaration::usage unable to find declaration usage %d (%s)\n", 
		usage, VertexElement::StringFromSemantic( usage ).to_string().c_str() );
	return (D3DDECLUSAGE)(MAXD3DDECLUSAGE + 1);
}


// Only return true if all the storage types in the format are supported
bool VertexDeclaration::isFormatSupportedOnDevice( const VertexFormat & format )
{
	uint32 numElements = format.countElements();
	for (uint32 elementIndex = 0; elementIndex < numElements; ++elementIndex)
	{
		Moo::VertexElement::StorageType::Value storageType;
		Moo::VertexElement::SemanticType::Value semanticType;
		format.findElement( elementIndex, &semanticType, 
			NULL, NULL, NULL, &storageType );

		if (!isTypeSupportedOnDevice( storageType ))
		{
			return false;
		}
	}
	return true;
}


/**
 *	Constructor.
 */
VertexDeclaration::VertexDeclaration( const BW::string& name ) :
	name_( name )
{
}

/**
 *	Destructor.
 */
VertexDeclaration::~VertexDeclaration()
{
}

/**
 *
 *	Take the two existing declarations and merge them to make
 *  this declaration a combination of the two.
 */
bool VertexDeclaration::merge( Moo::VertexDeclaration* orig, 
	Moo::VertexDeclaration* extra )
{
	BW_GUARD;
	MF_ASSERT_DEV( orig && orig->declaration() );
	MF_ASSERT_DEV( extra && extra->declaration() );

	
	// Create a clone of orig format with new name
	BW::string newName = orig->name() + "_" + extra->name();
	format_ = VertexFormat( newName );
	VertexFormat::merge( format_, orig->format_ );

	// Merge the extra format
	if (!VertexFormat::merge( format_, extra->format_ ))
	{
		return false;
	}

	if (!createDeclaration())
	{
		ERROR_MSG( "VertexDeclaration::merge - Unable to combine declarations:"
			" %s and %s\n", orig->name().c_str(), extra->name().c_str() );
		return false;
	}
	return true;
}

/**
 *	Load vertex declarations from a given data section
 *	@returns true if successful, false otherwise.
 */
bool VertexDeclaration::load( DataSectionPtr pSection, const BW::StringRef & declName )
{
	BW_GUARD;

	if (!VertexFormat::load( format_, pSection, declName ))
	{
		return false;
	}

	if (!isFormatSupportedOnDevice( format_ ))
	{
		WARNING_MSG( "VertexDeclaration::load - Attempting to create vertex"
			" declaration on target '%s' with unsupported format: '%s'\n", 
			VertexDeclaration::getTargetDevice(), 
			pSection->sectionName().c_str() );
	}

	if (!createDeclaration())
	{
		ERROR_MSG( "VertexDeclaration::load - Unable to create vertex"
			" declaration on target '%s' with format: '%s'\n", 
			VertexDeclaration::getTargetDevice(), 
			pSection->sectionName().c_str() );
		return false;
	}
	return true;
}

bool VertexDeclaration::createDeclaration()
{
	bool ret = false;
	BW::vector< D3DVERTEXELEMENT9 >	elements;
	D3DVERTEXELEMENT9 currentElement;
	ZeroMemory( &currentElement, sizeof( currentElement ) );

	uint32 numElements = format_.countElements();
	for (uint32 index = 0; index < numElements; ++index)
	{
		VertexElement::SemanticType::Value semantic;
		VertexElement::StorageType::Value storageType;
		uint32 streamIndex;
		uint32 semanticIndex;
		size_t offset;
		format_.findElement(index, &semantic, &streamIndex, &semanticIndex, 
			&offset, &storageType);

		D3DDECLUSAGE u = usage( semantic );
		if (u <= MAXD3DDECLUSAGE)
		{
			currentElement.Usage = u;
			currentElement.UsageIndex = semanticIndex;
			currentElement.Stream = streamIndex;
			currentElement.Offset = (WORD)offset;
			currentElement.Type = type( storageType );
			elements.push_back( currentElement );
			ret = true;
		}
	}

	if (ret)
	{
		const D3DVERTEXELEMENT9 theend = D3DDECL_END();
		DX::Device* pDevice = rc().device();
		elements.push_back( theend );
		IDirect3DVertexDeclaration9* pDecl = NULL;
		if (SUCCEEDED(pDevice->CreateVertexDeclaration( &elements.front(), 
			&pDecl )))
		{
			pDecl_ = pDecl;
			pDecl->Release();
			pDecl = NULL;
		}
		else
		{
			ret = false;
		}
	}
	return ret;
}

/**
 * This type represents a map of names and vertex declarations.
 */
typedef BW::StringRefMap< VertexDeclaration* > DeclMap;
static DeclMap s_declMap;

/**
 * This method returns pointer to a vertex declaration object given a name. It
 * will attempt to load vertex declaration if not found.
 *	@returns true if successful, false if declaration not found and loaded.
 */
VertexDeclaration* VertexDeclaration::get( const BW::StringRef & declName )
{
	BW_GUARD;
	SimpleMutexHolder smh( declarationsLock_ );

	DeclMap::iterator it = s_declMap.find( declName );

	if (it == s_declMap.end())
	{
		DataSectionPtr pSection = BWResource::instance().openSection( 
			VertexFormatCache::getResourcePath( declName ) );
		VertexDeclaration* vd = new VertexDeclaration( declName.to_string() );
		if (pSection && vd->load( pSection, declName ))
		{
			s_declMap[declName] = vd;
			return vd;
		}
		delete vd;
		vd = NULL;
		return NULL;
	}
	return it->second;
}


/**
 *	Factory method for creation of combined declarations.
 */
VertexDeclaration* VertexDeclaration::combine(
	Moo::VertexDeclaration* orig, Moo::VertexDeclaration* extra )
{
	BW_GUARD;
	SimpleMutexHolder smh( declarationsLock_ );

	BW::string newName = orig->name() + "_" + extra->name();

	// see if the new decl has been created before..
	DeclMap::iterator it = s_declMap.find( newName );

	// not found...
	if (it == s_declMap.end())
	{
		// build a new declaration.
		VertexDeclaration* vd = new VertexDeclaration( newName );

		if ( vd->merge( orig, extra ) )
		{
			s_declMap[newName] = vd;
			return vd;
		}
		delete vd;
		return NULL;
	}
	return it->second;
}

/**
 * Finalise object.
 */
void VertexDeclaration::fini()
{
	BW_GUARD;
	DeclMap::iterator it = s_declMap.begin();
	DeclMap::iterator end = s_declMap.end();
	while (it != end)
	{
		delete it->second;
		it->second = NULL;
		++it;
	}
}


} // namespace Moo

BW_END_NAMESPACE

// vertex_declaration.cpp
