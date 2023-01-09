#include "pch.hpp"
#include "moo/texture_detail_level.hpp"
#include "moo/format_name_map.hpp"
#include "cstdmf/string_builder.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{


/**
 *	This method loads the matching and conversion criteria from
 *	a data section.
 */
void TextureDetailLevel::init( DataSectionPtr pSection )
{
	BW_GUARD;
	pSection->readStrings( "prefix", prefixes_ );
	pSection->readStrings( "postfix", postfixes_ );
	pSection->readStrings( "contains", contains_ );
	lodMode_ = pSection->readInt( "lodMode", lodMode_ );
	mipCount_ = pSection->readInt( "mipCount", mipCount_ );
	bool horizMipchain = false;
	horizMipchain = pSection->readBool( "horizontalMips", horizMipchain );
	if (horizMipchain)
	{
		sourceMipType_ = SOURCE_HORIZONTAL_MIPMAPS;
	}
	bool vertMipchain = false;
	vertMipchain = pSection->readBool( "verticalMips", vertMipchain );
	if (vertMipchain)
	{
		sourceMipType_ = SOURCE_VERTICAL_MIPMAPS;
	}

	noResize_ = pSection->readBool( "noResize", noResize_ );	
	noFilter_ = pSection->readBool( "noFilter", noFilter_ );	
	normalMap_ = pSection->readBool( "normalMap", normalMap_ );	
	streamable_ = pSection->readBool( "streamable", streamable_ );	
	streamingPriority_ = pSection->readInt( "streamingPriority", streamingPriority_ );
	mipSize_ = pSection->readInt( "mipSize", mipSize_ );
	maxExtent_ = pSection->readInt( "maxExtent", maxExtent_ );

	// look for the name of the current D3DFormat in format_
	//  if not found, set it to A8R8G8B8
	// read in the "format" name & if DNE use previous formatname 

	BW::string formatName = FormatNameMap::formatString( format_ );
	if ( formatName == "D3DFMT_UNKNOWN" )
	{
		formatName = "A8R8G8B8";
	}
	formatName = pSection->readString( "format", formatName );

	// assign format_ to the updated formatName.
	//	if we cant find it, leave it to what it was before.

	D3DFORMAT newFormat = FormatNameMap::formatType( formatName );
	if ( newFormat != D3DFMT_UNKNOWN )
	{
		format_ = newFormat;
	}

	// read in the formatcompressed string, & if not found set
	//	it to "UNKNOWN"
	formatName = "UNKNOWN";
	formatName = pSection->readString( "formatCompressed", formatName );
	
	
	newFormat = FormatNameMap::formatType( formatName );
	if ( newFormat != D3DFMT_UNKNOWN )
	{
		formatCompressed_ = newFormat;
	}


}

/**
 *	Initialises this TextureDetailLevel object, matching the given file name to the
 *	provided texture format. All other parameters are left with their default values.
 */
void TextureDetailLevel::init( const BW::StringRef & fileName, D3DFORMAT format )
{
	BW_GUARD;
	contains_.push_back( fileName.to_string() );
	format_ = format;
}

/**
 * Writes a TextureDetailLevel object to a data section for serialisation.
 */
void TextureDetailLevel::write( DataSectionPtr pSection )
{
	BW_GUARD;
	// clean the data section
	pSection->delChildren();

	// save the texture detail level version
	DataSectionPtr pVersion = pSection->newSection( "version" );
	pVersion->isAttribute( true );
	pVersion->setInt( TEXTURE_DETAIL_LEVEL_VERSION );

	// write out the properties
	pSection->writeStrings( "prefix", prefixes_ );
	pSection->writeStrings( "postfix", postfixes_ );
	pSection->writeStrings( "contains", contains_ );
	pSection->writeInt( "lodMode", lodMode_ );
	pSection->writeInt( "mipCount", mipCount_ );
	pSection->writeBool( "horizontalMips", sourceMipType_ == SOURCE_HORIZONTAL_MIPMAPS );
	pSection->writeBool( "verticalMips", sourceMipType_ == SOURCE_VERTICAL_MIPMAPS );

	pSection->writeBool( "noResize", noResize_ );	
	pSection->writeBool( "noFilter", noFilter_ );	
	pSection->writeBool( "normalMap", normalMap_ );	
	pSection->writeBool( "streamable", streamable_ );	
	pSection->writeInt( "streamingPriority", streamingPriority_ );
	pSection->writeInt( "mipSize", mipSize_ );
	pSection->writeInt( "maxExtent", maxExtent_ );

	pSection->writeString( "format", FormatNameMap::formatString( format_ ) );
	pSection->writeString( "formatCompressed", FormatNameMap::formatString( formatCompressed_ ) );
}

/**
 * Returns true if string starts with substring.
 */
bool startsWith( const BW::StringRef& string, const BW::StringRef& substring )
{
	BW_GUARD;
	bool res = false;
	if (substring.length() < string.length())
	{
		res = strncmp(substring.data(), string.data(), substring.length()) == 0;
	}
	return res;
}

/**
 * Returns true if string ends with substring.
 */
bool endsWith( const BW::StringRef& string, const BW::StringRef& substring )
{
	BW_GUARD;
	bool res = false;
	BW::StringRef::size_type length = substring.length();
	BW::StringRef::size_type length2 = string.length();

	if (length2 >= length)
	{
		res = substring == string.substr( length2 - length, length );
	}
	return res;
}

/**
 * This function wraps BW::string.find()
 */
bool containsString( const BW::StringRef& string, const BW::StringRef& substring )
{
	BW_GUARD;
	return string.find( substring ) != BW::StringRef::npos;
}

/**
 * A function templated on checker type.
 */
template<class Checker>
bool stringCheck( const BW::StringRef& mainString, 
				 const TextureDetailLevel::StringVector& subStrings,
				Checker checker )
{
	BW_GUARD;
	bool ret = !subStrings.size();
	TextureDetailLevel::StringVector::const_iterator it = subStrings.begin();
	while (it != subStrings.end() && ret == false)
	{
		if(checker( mainString, *it ))
			ret = true;
		it++;
	}
	return ret;
}

/**
 *	This method checks to see if a resource name matches this detail level
 *	@param resourceID the resourceID to check
 *	@return true if this detail level matches the resourceID
 */
bool TextureDetailLevel::check( const BW::StringRef& resourceID )
{
	BW_GUARD;
	bool match = false;
	match = stringCheck( resourceID, prefixes_, startsWith );
	if (match)
		match = stringCheck( resourceID, postfixes_, endsWith );
	if (match)
		match = stringCheck( resourceID, contains_, containsString );
	return match;
}



/**
 * Default constructor.
 */
TextureDetailLevel::TextureDetailLevel() : 
	format_( D3DFMT_A8R8G8B8 ),
	formatCompressed_( D3DFMT_UNKNOWN ),
	lodMode_( 0 ),
	mipCount_( 0 ),
	mipSize_( 0 ),
	streamingPriority_( 0 ),
	maxExtent_( 0 ),
	sourceMipType_( SOURCE_NO_MIPMAPS ),
	noResize_( false ),
	noFilter_( false ),
	normalMap_( false ),
	streamable_( true )
{

}

/**
 * Destructor
 */
TextureDetailLevel::~TextureDetailLevel()
{

}

/**
 * Clear this object.
 */
void TextureDetailLevel::clear( )
{
	prefixes_.clear();
	postfixes_.clear();
	contains_.clear();
	lodMode_ = 0;
	format_ = D3DFMT_A8R8G8B8;
	formatCompressed_ = D3DFMT_UNKNOWN;
	mipCount_ = 0;
	mipSize_ = 0;
	maxExtent_ = 0;
	sourceMipType_ = SOURCE_NO_MIPMAPS;
	noResize_ = false;
	noFilter_ = false;
	normalMap_ = false;
	streamable_ = true;
	streamingPriority_ = 0;
}

} // namespace Moo

BW_END_NAMESPACE
