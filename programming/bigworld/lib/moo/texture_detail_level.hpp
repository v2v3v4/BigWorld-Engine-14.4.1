#ifndef TEXTURE_DETAIL_LEVEL_HPP
#define TEXTURE_DETAIL_LEVEL_HPP

#define TEXTURE_DETAIL_LEVEL_VERSION 1

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stdmf.hpp"

#include "resmgr/datasection.hpp"

#include <d3d9types.h>

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 * This class represents the global texture detail filters. Each detail level
 * specifies a string that texture should match to have that detail level.
 * See bigworld\res\system\data\texture_detail_levels.xml for example detail
 * levels.
 */
class TextureDetailLevel : public SafeReferenceCount
{
public:
	enum SourceMipmapsType
	{
		SOURCE_NO_MIPMAPS,
		SOURCE_HORIZONTAL_MIPMAPS,
		SOURCE_VERTICAL_MIPMAPS
	};

	TextureDetailLevel();
	~TextureDetailLevel();
	void		init( DataSectionPtr pSection );
	void		init( const BW::StringRef & fileName, D3DFORMAT format );
	void		write( DataSectionPtr pSection );
	bool		check( const BW::StringRef& resourceID );

	void		clear();

	D3DFORMAT	format() const { return format_; }
	void		format( D3DFORMAT format ) { format_ = format; }
	D3DFORMAT	formatCompressed() const { return formatCompressed_; }
	void		formatCompressed( D3DFORMAT formatCompressed ) { formatCompressed_ = formatCompressed; }
	uint32		lodMode() const { return lodMode_; }
	void		lodMode( uint32 lodMode ) { lodMode_ = lodMode; }
	void		mipCount( uint32 count ) { mipCount_ = count; }
	uint32		mipCount() const { return mipCount_; }
	void		mipSize( uint32 size ) { mipSize_ = size; }
	uint32		mipSize() const { return mipSize_; }
	SourceMipmapsType	sourceMipmaps() const { return sourceMipType_; }
	void		sourceMipmaps( SourceMipmapsType type ) { sourceMipType_ = type; }
	uint		maxExtent() const { return maxExtent_; }
	void		maxExtent( uint extent ) { maxExtent_ = extent; }

	void		noResize( bool val ) { noResize_ = val; }
	bool		noResize( ) const { return noResize_; }
	void		noFilter( bool val ) { noFilter_ = val; }
	bool		noFilter( ) const { return noFilter_; }
	void		normalMap( bool val ) { normalMap_ = val; }
	bool		normalMap( ) const { return normalMap_; }

	void		streamable( bool val ) { streamable_ = val; }
	bool		streamable( ) const { return streamable_; }

	void		streamingPriority( uint32 val ) { streamingPriority_ = val; }
	uint32		streamingPriority() const { return streamingPriority_; }


	typedef BW::vector< BW::string > StringVector;

	StringVector& postfixes() { return postfixes_; }
	StringVector& prefixes() { return prefixes_; }
	StringVector& contains() { return contains_; }

	const bool	isCompressed() { return formatCompressed() != D3DFMT_UNKNOWN; }
	
private:

	StringVector	prefixes_;
	StringVector	postfixes_;
	StringVector	contains_;
	uint32			lodMode_;
	uint32			mipCount_;
	uint32			mipSize_;
	uint32			streamingPriority_;
	uint			maxExtent_;
	SourceMipmapsType	sourceMipType_;
	D3DFORMAT		format_;
	D3DFORMAT		formatCompressed_;
	bool			noResize_;
	bool			noFilter_;
	bool			normalMap_;
	bool			streamable_;
};

} // namespace Moo

BW_END_NAMESPACE


#endif // TEXTURE_DETAIL_LEVEL_HPP
