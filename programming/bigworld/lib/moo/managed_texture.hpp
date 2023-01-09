// managed_texture.hpp

#ifndef MANAGED_TEXTURE_HPP
#define MANAGED_TEXTURE_HPP

#include "base_texture.hpp"


BW_BEGIN_NAMESPACE

class DataSection;
class BinaryBlock;
typedef SmartPointer<DataSection> DataSectionPtr;
typedef SmartPointer<BinaryBlock> BinaryPtr;

namespace Moo
{
typedef SmartPointer< class ManagedTexture > ManagedTexturePtr;

class TextureLoader;
/**
 * TODO: to be documented.
 */
class ManagedTexture : public BaseTexture
{
public:
	typedef ComObjectWrap< DX::BaseTexture > Texture;

	// Constructor that creates a blank lockable managed texture.  The
	// resourceID publishes the texture for external use.
	ManagedTexture( 
		const BW::StringRef& resourceID, uint32 w, uint32 h, int nLevels,
		DWORD usage, D3DFORMAT fmt, const BW::StringRef& allocator = 
		"texture/unknown managed texture" );
	bool resize( uint32 w, uint32 h, int nLevels, DWORD usage, D3DFORMAT fmt );


	DX::BaseTexture*	pTexture( );
	const BW::string&	resourceID( ) const;

	/// Returns the width of the texture, not completely valid if the texture is
	/// a cubetexture, but what can you do...
	uint32				width( ) const;

	/// Returns the height of the texture, not completely valid if the texture is
	/// a cubetexture, but what can you do...
	uint32				height( ) const;

	D3DFORMAT			format( ) const;

	/// Returns the memory used by the texture.
	virtual uint32		textureMemoryUsed() const;

	static void			tick();

	virtual uint32		mipSkip() const;
	virtual void		mipSkip( uint32 mipSkip );

	virtual bool		isCubeMap() { return cubemap_; }

	virtual void		load() {
		return this->load( BaseTexture::FAIL_ATTEMPT_LOAD_PLACEHOLDER ); }
	virtual void		release();

	virtual void		reload( );
	virtual void		reload( const BW::StringRef & resourceID );

	static bool					s_accErrs;
	static BW::string			s_accErrStr;
	static void					accErrs( bool acc );
	static const BW::string&	accErrStr();

	static uint32		totalFrameTexmem_;

private:
	virtual void destroy() const;
	~ManagedTexture();

	uint32				width_;
	uint32				height_;
	D3DFORMAT			format_;
	uint32				mipSkip_;
	bool				loadedFromDisk_;

	bool				noResize_;
	bool				noFilter_;

	uint32				textureMemoryUsed_;

	ManagedTexture( 
		const BW::StringRef& resourceID, const FailurePolicy& notFoundPolicy,
		D3DFORMAT format = D3DFMT_UNKNOWN,
		int mipSkip = 0, bool noResize = false, bool noFilter = false,
		const BW::StringRef& allocator = "texture/unknown managed texture" );
		
	ManagedTexture(
		DataSectionPtr data, const BW::StringRef& resourceID,
		const FailurePolicy& notFoundPolicy,
		int mipSkip = 0, bool noResize = false, bool noFilter = false,
		const BW::StringRef& allocator = "texture/unknown managed texture" );

	void				load( const FailurePolicy& notFoundPolicy );
	bool				loadBin( BinaryPtr texture,
		const FailurePolicy& notFoundPolicy, bool skipHeader = false );

	void processLoaderOutput( const TextureLoader& texLoader,
		const FailurePolicy& notFoundPolicy );
	void reportMissingTexture( const BW::StringRef& qualifiedResourceID ) const;
	
	DX::BaseTexture*			texture_;
	ComObjectWrap< DX::Texture > tex_;
	ComObjectWrap< DX::CubeTexture > cubeTex_;
	BW::string			resourceID_;
	BW::string			qualifiedResourceID_;
	bool				reuseOnRelease_;
	bool				cubemap_;
	uint32				localFrameTimestamp_;
	static uint32		frameTimestamp_;
	SimpleMutex			loadingLock_;

	friend class TextureManager;
	friend class StreamingTexture;

	// task to reload texture on the background thread
	class ReloadTextureTask;
};

} // namespace Moo

#ifdef CODE_INLINE
#include "managed_texture.ipp"
#endif

BW_END_NAMESPACE

#endif
// managed_texture.hpp
