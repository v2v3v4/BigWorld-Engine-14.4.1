#include "vertex_format_cache.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

	template <class VertexType>
	const BW::StringRef VertexFormatCache::getFormatName()
	{
		return VertexFormatDefinitionTypeTraits<VertexType>::name();
	}


	template <class VertexType>
	const VertexFormat * VertexFormatCache::get()
	{
		return get( getFormatName<VertexType>() );
	}


	template <class SourceVertexType>
	const VertexFormat * VertexFormatCache::getTarget( 
		const BW::StringRef & targetKey, bool fallbackToSource )
	{
		const VertexFormat* sourceFormat = get<SourceVertexType>();
		if (sourceFormat)
		{
			return VertexFormatCache::getTarget( *sourceFormat, targetKey, fallbackToSource	);
		}
		else
		{
			WARNING_MSG( "VertexFormatCache::getTarget<>: "
				"could not get the source format with name '%s'.\n", 
				getFormatName<SourceVertexType>().to_string().c_str()	);
		}
		return NULL;
	}

	template <class SourceVertexType>
	const BW::StringRef VertexFormatCache::getTargetName( 
		const BW::StringRef & targetKey, bool fallbackToSource )
	{
		const VertexFormat * targetFormat = 
			getTarget<SourceVertexType>( targetKey, fallbackToSource );
		if (targetFormat)
		{
			return targetFormat->name();
		}
		return BW::StringRef();
	}

	template <typename SourceVertexType>
	/* static */ VertexFormat::ConversionContext VertexFormatCache::createTargetConversion(
		const BW::StringRef & targetKey )
	{
		return VertexFormat::ConversionContext( 
			getTarget<SourceVertexType>( targetKey, false ),
			get<SourceVertexType>() );
	}

} // namespace Moo

BW_END_NAMESPACE