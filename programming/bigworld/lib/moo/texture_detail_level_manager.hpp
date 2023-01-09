#ifndef TEXTURE_DETAIL_LEVEL_MANAGER_HPP
#define TEXTURE_DETAIL_LEVEL_MANAGER_HPP

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/stringmap.hpp"

#include <d3d9types.h>

#include <memory>

BW_BEGIN_NAMESPACE

class AssetClient;
class HierarchicalConfig;

namespace Moo
{
class TextureDetailLevel;
typedef SmartPointer< TextureDetailLevel > TextureDetailLevelPtr;

class TextureDetailLevelManager
{
public:
	TextureDetailLevelManager();
	~TextureDetailLevelManager();
	void init();
	void init( const BW::StringRef & textureDetailsName );
	void fini();
	TextureDetailLevelPtr detailLevel( const BW::StringRef & originalName );
	void setFormat( const BW::StringRef & fileName, D3DFORMAT format );
	bool isSourceFile( BW::StringRef filename );

	const BW::string & getTextureDetailLevelsName();

#if ENABLE_ASSET_PIPE
	void setAssetClient( AssetClient * pAssetClient );
#endif

private:
	void loadDetailLevels( const BW::StringRef & textureDetailsName );

	std::auto_ptr< HierarchicalConfig >	pTextureDetails_;
#if ENABLE_ASSET_PIPE
	AssetClient * pAssetClient_;
#endif

	typedef StringRefMap<TextureDetailLevelPtr> DetailLevelsCache;
	DetailLevelsCache detailLevelsOverrides_;
	DetailLevelsCache detailLevelsCache_;
};


} // namespace Moo

BW_END_NAMESPACE


#endif // TEXTURE_DETAIL_LEVEL_MANAGER_HPP
