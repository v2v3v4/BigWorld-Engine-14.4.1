#ifndef VERTEX_FORMAT_CACHE_HPP
#define VERTEX_FORMAT_CACHE_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/stringmap.hpp"
#include "vertex_format.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
/**
 * This class tracks a collection of vertex formats and caches them for
 * retrieval by name. If a requested format is not being tracked, this
 * class will try to load it first.
 */
class VertexFormatCache
{
public:

	/**
	 * This method returns a vertex format object given a name. 
	 * It will attempt to load the vertex format if not yet cached.
	 *
	 *	@param	formatName	The name of the format to get
	 *	@return	Valid pointer if found or loaded successfully, else NULL ptr.
	 */
	static const VertexFormat * get( const BW::StringRef & formatName );
	
	/**
	 * Non-loading version of get.
	 *	@param	formatName	The name of the format to find
	 *	@return	Valid pointer if found, else NULL ptr.
	 */
	static const VertexFormat * find( const BW::StringRef & formatName );
	
	/**
	 * This method adds a given vertex format with a given name. This can
	 * be used to add vertex formats that are dynamically created/combined.
	 * Will not overwrite existing names.
	 *
	 *	@param	formatName	The name to assign the provided vertex format
	 *	@param	format	The vertex format object to store in the cache,
	 *	@return	Valid pointer if formatName added successfully, else NULL ptr.
	 */
	static const VertexFormat * add( const BW::StringRef & formatName, 
		const VertexFormat & format );

	/// This function returns the filepath for the corresponding vertex format
	static BW::string getResourcePath( const BW::StringRef & formatName );

	/**
	 * This method returns a vertex format object for the give source S
	 * and target name T. If a format TF is defined for the format S, will 
	 * return TF (loading it if necessary). Otherwise, if no target format is 
	 * specified for the target, or the target format failed to load, will
	 * return a value based on the fallbackToSource parameter.
	 *
	 * This method does the target format work used by other wrapper 
	 * functions in VertexFormatCache.
	 *
	 *	@param	sourceFormat		The source vertex format.
	 *
	 *	@param	targetKey			The targeting (device) name for lookup
	 *								of the actual target vertex format.
	 *
	 *	@param	fallbackToSource	If true, return the source format 
	 *								(and source name if provided) if
	 *								a target format was not found.
	 *								Otherwise, return NULL. (default=true)
	 */
	static const VertexFormat * getTarget( const VertexFormat & sourceFormat, 
		const BW::StringRef & targetKey, bool fallbackToSource = true );


	/**
	 * Get the vertex format definition name corresponding to a vertex type
	 * (e.g. the Vertex types defined in vertex_formats.hpp)
	 */ 
	template <class VertexType>
	static const BW::StringRef getFormatName();

	/// Get the vertex format object corresponding to a vertex type
	template <class VertexType>
	static const VertexFormat * get();

	/// Get the targeted vertex format object corresponding to a source vertex type
	template <class SourceVertexType>
	static const VertexFormat * getTarget( 
		const BW::StringRef & targetKey, bool fallbackToSource = true );

	/// Get the targeted vertex format name corresponding to a source vertex type
	/// Return empty StringRef if no target found
	template <class SourceVertexType>
	static const BW::StringRef getTargetName( 
		const BW::StringRef & targetKey, bool fallbackToSource = true );
	
	/**
	 * Create and return a ConversionContext object for converting
	 * a given source vertex type to its target type.
	 */
	template <typename SourceVertexType>
	static VertexFormat::ConversionContext createTargetConversion(
		const BW::StringRef & targetKey );
};

} // namespace Moo

BW_END_NAMESPACE

// template definitions
#include "vertex_format_cache.ipp"

#endif // VERTEX_FORMAT_CACHE_HPP
