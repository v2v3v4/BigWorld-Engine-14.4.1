#include "pch.hpp"
#include "vertex_format_cache.hpp"
#include "vertex_format.hpp"
#include "vertex_formats.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );

BW_BEGIN_NAMESPACE

/**
 * VertexFormatCache is not a singleton by design. The class implementation
 * is hidden in a detail namespace, and the public interface is a static one
 * that calls the appropriate methods on a single private static instance.
 *
 * Ideally we would have an "Environment" instance to attach this instance to.
 * Currently there are no suitable instances to make this class a member of 
 * (without introducing unnecessary dependencies, e.g. render context).
 *
 * When multiple or distinct instances are required in the future, it can be 
 * moved easily by taking the implementation out of the detail namespace and
 * making the public interface non-static.
 */

// -----------------------------------------------------------------------------
// Section: VertexFormatCache private implementation
// -----------------------------------------------------------------------------

namespace Moo
{

namespace detail
{
/**
 * This class tracks a collection of vertex formats and caches them for
 * retrieval by name. If a requested format is not being tracked, this
 * class will try to load it first.
 */
class VertexFormatCacheInternal
{
private:
	typedef BW::StringRefMap<VertexFormat> FormatMap;

// Refer to public interface's function comments in header file.
public:

	const VertexFormat * get( const BW::StringRef & formatName );
	const VertexFormat * find( const BW::StringRef & formatName );
	const VertexFormat * add( const BW::StringRef & formatName, 
		const VertexFormat & format );

	static BW::string getResourcePath( const BW::StringRef & formatName );

	const VertexFormat * getTarget( const VertexFormat & sourceFormat, 
		const BW::StringRef & targetKey, bool fallbackToSource = true );
	

	template <class VertexType>
	const VertexFormat * get();
	
	template <class SourceVertexType>
	const BW::StringRef getTargetName( 
		const BW::StringRef & targetKey, bool fallbackToSource = true );

	template <typename SourceVertexType>
	VertexFormat::ConversionContext createTargetConversion(
		const BW::StringRef & targetKey );

private:
	/**
	 * Internal insert. Does not check input, or overwrite existing entries.
	 *
	 *	@param	formatName	The name to assign the provided vertex format
	 *	@param	format	The vertex format object to store in the cache,
	 *	@return	Valid pointer on successful (non-colliding) insert. Else NULL.
	 */
	const VertexFormat * insert( const BW::StringRef & formatName, 
		const VertexFormat & format );


	FormatMap formats_;
	SimpleMutex formatsLock_;

};


const VertexFormat * VertexFormatCacheInternal::get( const BW::StringRef & formatName )
{
	const VertexFormat* foundFormat = find( formatName );
	if (foundFormat)
	{
		return foundFormat;
	}

	// This name doesn't exist; try to insert if the 
	// vertex format can be loaded successfully.
	DataSectionPtr pSection = 
		BWResource::instance().openSection( getResourcePath( formatName ) );
	VertexFormat vf;
	if (pSection && VertexFormat::load( vf, pSection, formatName ))
	{
		return insert( formatName, vf );
	}
	return NULL;
}

const VertexFormat * VertexFormatCacheInternal::find( const BW::StringRef & formatName )
{
	BW_GUARD;
	SimpleMutexHolder smh( formatsLock_ );

	FormatMap::const_iterator it = formats_.find( formatName );
	if (it != formats_.end())
	{
		return &(it->second);
	}
	return NULL;
}


const VertexFormat * VertexFormatCacheInternal::add( const BW::StringRef & formatName, 
	const VertexFormat& format )
{
	MF_ASSERT(format.countElements() > 0);

	if (!find( formatName ))
	{
		// Try to add only if this name doesn't already exist
		return insert( formatName, format );
	}
	return NULL;
}

const VertexFormat * VertexFormatCacheInternal::insert( const BW::StringRef & formatName, 
	const VertexFormat& format )
{
	BW_GUARD;
	SimpleMutexHolder smh( formatsLock_ );

	std::pair<FormatMap::iterator, bool> insertResult = 
		formats_.insert( FormatMap::value_type( formatName, format ) );
	if (insertResult.second)
	{
		// if we inserted successfully, return a pointer to the stored value
		return &(insertResult.first->second);
	}
	return NULL;
}


const VertexFormat * VertexFormatCacheInternal::getTarget( const VertexFormat & sourceFormat, 
	const BW::StringRef & targetKey, bool fallbackToSource )
{
	// Use target format if found
	BW::StringRef targetFormatName = 
		sourceFormat.getTargetFormatName( targetKey );
	if (!targetFormatName.empty())
	{
		const VertexFormat* targetFormat = get( targetFormatName );
		if (targetFormat)
		{
			return targetFormat;
		}
	}
	else if (fallbackToSource)
	{
		return &sourceFormat;
	}

	return NULL;
}

BW::string VertexFormatCacheInternal::getResourcePath( const BW::StringRef & declName )
{
	return BW::string("shaders/formats/") + declName + ".xml"; 
}

}	// namespace detail


// -----------------------------------------------------------------------------
// Section: VertexFormatCache public/static implementation
// -----------------------------------------------------------------------------

// route all calls to static instance
static detail::VertexFormatCacheInternal s_vertexFormatCache;

const VertexFormat * VertexFormatCache::get( const BW::StringRef & formatName )
{
	return s_vertexFormatCache.get( formatName );
}

const VertexFormat * VertexFormatCache::find( const BW::StringRef & formatName )
{
	return s_vertexFormatCache.find( formatName );
}
const VertexFormat * VertexFormatCache::add( const BW::StringRef & formatName, 
						 const VertexFormat & format )
{
	return s_vertexFormatCache.add( formatName, format );
}

BW::string VertexFormatCache::getResourcePath( const BW::StringRef & formatName )
{
	return s_vertexFormatCache.getResourcePath( formatName );
}

const VertexFormat * VertexFormatCache::getTarget( const VertexFormat & sourceFormat, 
	const BW::StringRef & targetKey, bool fallbackToSource )
{
	return s_vertexFormatCache.getTarget( sourceFormat, targetKey, fallbackToSource );
}


} // namespace Moo

BW_END_NAMESPACE

