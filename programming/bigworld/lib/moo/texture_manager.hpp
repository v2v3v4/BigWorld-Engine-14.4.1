#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP


#include "cstdmf/bw_list.hpp"
#include "cstdmf/stringmap.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"
#include "resmgr/datasection.hpp"
#include "device_callback.hpp"
#include "moo_dx.hpp"
#include "graphics_settings.hpp"
#include "resmgr/resource_modification_listener.hpp"
#include "moo/texture_detail_level.hpp"
#include "moo/texture_detail_level_manager.hpp"


BW_BEGIN_NAMESPACE

#if ENABLE_ASSET_PIPE
class AssetClient;
#endif

namespace Moo
{

class BaseTexture;
typedef SmartPointer<BaseTexture> BaseTexturePtr;

class TextureStreamingManager;

/**
 *	This singleton class keeps track of and loads ManagedTextures.
 */
class TextureManager :
	public Moo::DeviceCallback,
	public ResourceModificationListener
{
public:

	class ProgressListener
	{
	public:
		enum ProgressType
		{
			PURGE,
			RELOAD
		};

		virtual void onTextureManagerProgress( ProgressType progressType, 
						int totalSteps, int numCompleted, int step = 1 ) = 0;
	};

	typedef BW::StringRefMap< BaseTexture * > TextureMap;

	~TextureManager();

	static TextureManager*	instance();
	TextureStreamingManager * streamingManager() { return streamingManager_.get(); }
	TextureDetailLevelManager * detailManager() { return pDetailLevels_; }

	static const BW::string & notFoundBmp();

	HRESULT					initTextures( );
	void					releaseTextures( );

	void					deleteManagedObjects( );
	void					createManagedObjects( );

	uint32					textureMemoryUsed( ) const;

	void					fullHouse( bool noMoreEntries = true );

	static bool				writeDDS( DX::BaseTexture* texture, 
								const BW::StringRef& ddsName, D3DFORMAT format, int numDestMipLevels = 0 );
	void					reloadAllTextures( ProgressListener* pCallBack = NULL );
	void					useDummyTexture( bool useDummyTexture ) { useDummyTexture_ = useDummyTexture; }
	
	BaseTexturePtr	get( const BW::StringRef & resourceID,
		bool allowAnimation = true, bool mustExist = true,
		bool loadIfMissing = true, const char* = "texture/unknown texture" );
	BaseTexturePtr	get( DataSectionPtr data, const BW::StringRef & resourceID,
		bool mustExist = true, bool loadIfMissing = true, bool refresh = true,
		const char* allocator = "texture/unknown texture" );
	void getAnimationSet( const BW::string & pathWithPrefix, Moo::BaseTexturePtr** dst, uint32 number,
		bool allowAnimation = true, bool mustExist = true,
		bool loadIfMissing = true);
	BaseTexturePtr	getUnique( 
		const BW::StringRef& resourceID,
		const BW::StringRef & sanitisedResourceID = "",
		bool allowAnimation = true, bool mustExist = true,
		const char* = "texture/unknown texture" );
	bool isTextureFile( const BW::StringRef & resourceID, bool checkExists = true ) const;

	BaseTexturePtr	getSystemMemoryTexture( const BW::StringRef& resourceID );

	int configurableMipFilter() const;
	int configurableMinMagFilter() const;
	int configurableMaxAnisotropy() const;

	const TextureMap&		textureMap() const { return textures_; };

	static void init();
	static void fini();

	static const char ** validTextureExtensions();
	bool isCompressed(TextureDetailLevelPtr lod) const;

	static void setTextureMemoryBlock(int minMemory);
	static size_t textureMemoryBlock() { return minMemoryForHighTextures; };

	void saveMemoryUsagePerTexture( const char* filename );

	void prepareResource( const StringRef & resourceID ) const;

	TextureDetailLevelManager* getDetailsManager() 
	{
		return pDetailLevels_; 
	};	

	static const BW::string & getFormatName( D3DFORMAT format );

#if ENABLE_ASSET_PIPE
	void setAssetClient( AssetClient * pAssetClient )
	{
		pAssetClient_ = pAssetClient;
	}
#endif

protected:
	virtual void onResourceModified( const BW::StringRef& basePath,
									const BW::StringRef& resourceID,
									ResourceModificationListener::Action modType );

private:
	TextureManager();
	TextureManager(const TextureManager&);
	TextureManager& operator=(const TextureManager&);

	static void add( BaseTexture * pTexture );
	static void tryDestroy( const BaseTexture * pTexture );

	BW::StringRef prepareResource( const BW::StringRef & resourceName,
		char * outPtr, size_t size, bool forceConvert = false ) const;
	BW::StringRef prepareResourceName( const BW::StringRef & resourceName, 
		char * outPtr, size_t maxSize ) const;

	void addInternal( BaseTexture * pTexture, const BW::StringRef& resourceID = "" );
	void delInternal( const BaseTexture * pTexture );
	bool delFromDirtyTextures( const BaseTexture * pTexture );

	
	void checkLoadedResourceName( const BW::StringRef & requested, const BW::StringRef & found ) const;

	BaseTexturePtr find( const BW::StringRef & resourceID );
	void setQualityOption(int optionIndex);
	void setCompressionOption(int optionIndex);
	void setFilterOption(int);
	void reloadDirtyTextures();
		
	TextureMap				textures_;
	RecursiveMutex			texturesLock_;


	std::auto_ptr< TextureStreamingManager > streamingManager_;

	bool					fullHouse_;
	int						lodMode_;

	
	bool					useDummyTexture_;

	static	size_t			minMemoryForHighTextures;

	GraphicsSetting::GraphicsSettingPtr		qualitySettings_;
	GraphicsSetting::GraphicsSettingPtr		compressionSettings_;
	GraphicsSetting::GraphicsSettingPtr		filterSettings_;

	typedef std::pair<BW::string, BW::string> TextureStringPair;
	typedef BW::vector<TextureStringPair> TextureStringVector;
	TextureStringVector dirtyTextures_;

	friend class BaseTexture;
	static TextureManager*		pInstance_;
	TextureDetailLevelManager*	pDetailLevels_;
#if ENABLE_ASSET_PIPE
	AssetClient * pAssetClient_;
#endif
};


} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_MANAGER_HPP
