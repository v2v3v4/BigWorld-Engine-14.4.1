#include "pch.hpp"
#include "moo/texture_detail_level.hpp"
#include "moo/texture_detail_level_manager.hpp"
#include "moo/base_texture.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/hierarchical_config.hpp"
#if ENABLE_ASSET_PIPE
#include "asset_pipeline/asset_client.hpp"
#endif

BW_BEGIN_NAMESPACE

namespace Moo
{
	static AutoConfigString s_textureDetailLevels( "system/textureDetailLevels" );

	TextureDetailLevelManager::TextureDetailLevelManager()
		: pTextureDetails_( NULL )
#if ENABLE_ASSET_PIPE
		, pAssetClient_(NULL)
#endif
	{

	}

	TextureDetailLevelManager::~TextureDetailLevelManager()
	{

	}

	const BW::string& TextureDetailLevelManager::getTextureDetailLevelsName()
	{
		BW_GUARD;
		MF_ASSERT( !s_textureDetailLevels.value().empty() );

		return s_textureDetailLevels; 
	}

	bool TextureDetailLevelManager::isSourceFile( BW::StringRef filename )
	{
		return ( filename == s_textureDetailLevels.value() );
	}
	
	void TextureDetailLevelManager::init()
	{
		loadDetailLevels( getTextureDetailLevelsName() );
	}

	void TextureDetailLevelManager::init( const BW::StringRef & textureDetailsName )
	{
		BW_GUARD;
		MF_ASSERT( !textureDetailsName.empty() );

		loadDetailLevels( textureDetailsName );
	}
	
	void TextureDetailLevelManager::fini()
	{
		pTextureDetails_.reset();
	}

	void TextureDetailLevelManager::loadDetailLevels( const BW::StringRef & textureDetailsName )
	{
		BW_GUARD;
		DataSectionPtr pDetailLevels = BWResource::openSection( textureDetailsName );
		pTextureDetails_.reset( new HierarchicalConfig( pDetailLevels ) );
		detailLevelsCache_.clear();
	}

	TextureDetailLevelPtr TextureDetailLevelManager::detailLevel( const BW::StringRef& originalName )
	{
		BW_GUARD;

		// check for any ovverides first
		DetailLevelsCache::iterator it = detailLevelsOverrides_.find( originalName );
		if (it != detailLevelsOverrides_.end())
		{
			return it->second;
		}

		// now check the cache
		it = detailLevelsCache_.find( originalName );
		if (it != detailLevelsCache_.end())
		{
			return it->second;
		}

		TextureDetailLevelPtr pRet = new TextureDetailLevel;

		DataSectionPtr pFmtSect = BWResource::openSection(
			removeTextureExtension( originalName ) + ".texformat" );

		if (!pFmtSect)
		{
			pFmtSect = pTextureDetails_->get( originalName );
		}
		if (pFmtSect)
		{
			pRet->init( pFmtSect );
		}

		detailLevelsCache_[originalName] = pRet;
		return pRet;
	}


	/**
	 *	Sets the texture format to be used when loading the specified texture.
	 *	This format will override all information defined in a texformat file 
	 *	or in the global texture_detail_levels file. Other fields, such as 
	 *	formatCompressed, lodMode, will be set 
	 *	to their default values. Call this function before loading the texture
	 *	for the first time. The format will not be changed if the texture has
	 *	already been loaded. 
	 *
	 *	Also, this function will not work if the texture has already been 
	 *	converted into a DDS in the filesystem. For this reason, when using 
	 *	this function, make sure this same format is used when batch converting 
	 *	this texture map for deployment (in the absence of the original file,
	 *	the format used to write the DDS will be the one used).
	 *	
	 *	@param	fileName	full pach to the texture whose format should be set
	 *						set this will be used like the contains field of the 
	 *						texture_detail_levels, potentially affecting more 
	 *						files if the file name provided is not the fullpath.
	 *
	 *	@param	format		the texture format to be used.
	 */
	void TextureDetailLevelManager::setFormat( const BW::StringRef & fileName, D3DFORMAT format )
	{
		BW_GUARD;
		TextureDetailLevelPtr pDetail = new TextureDetailLevel;
		pDetail->init(fileName, format);

		detailLevelsOverrides_[fileName] = pDetail;
	}

#if ENABLE_ASSET_PIPE
	void TextureDetailLevelManager::setAssetClient( AssetClient * pAssetClient )
	{
		BW_GUARD;

		pAssetClient_ = pAssetClient;
		if (pAssetClient_ != NULL)
		{
			pAssetClient_->requestAsset( getTextureDetailLevelsName(), true );
		}

		loadDetailLevels( getTextureDetailLevelsName() );
	}
#endif // ENABLE_ASSET_PIPE

} // namespace Moo

BW_END_NAMESPACE
