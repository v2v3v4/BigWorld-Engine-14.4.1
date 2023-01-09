#ifndef BASE_TEXTURE_HPP
#define BASE_TEXTURE_HPP

#include <iostream>


#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo_dx.hpp"
#include "moo_math.hpp"
#include "moo/texture_loader.hpp"
#include "moo/texture_manager.hpp"
#include "com_object_wrap.hpp"


BW_BEGIN_NAMESPACE

class TaskManager;

namespace Moo
{
typedef SmartPointer< class BaseTexture > BaseTexturePtr;

class TextureStatusHandler;
/**
 *	Reference counted base D3D texture class. All texture variants should be
 *	derived from here.
 */
class BaseTexture : public SafeReferenceCount
{
public:
	enum TextureType
	{
		UNKNOWN,
		STREAMING
	};

	enum Status
	{
		STATUS_NOT_READY,
		STATUS_LOADING,
		STATUS_READY,
		STATUS_FAILED_TO_LOAD
	};

	/**
	 * The failure policy specifies the behaviour when a texture fails to load.
	 */
	enum FailurePolicy
	{
		/**
		 * Do not attempt to load a replacement texture, just return NULL.
		 */
		FAIL_RETURN_NULL = 0,
		/**
		 *	Attempt to load the "not found" texture to replace the texture that
		 *	has failed to load.
		 *	@note this can still return NULL if the "not found" texture also
		 *		fails to load.
		 */
		FAIL_ATTEMPT_LOAD_PLACEHOLDER = 1
	};

	explicit BaseTexture(const BW::StringRef& allocator = "texture/unknown base texture");

	virtual DX::BaseTexture*	pTexture( ) = 0;
	virtual TextureType     type( ) const { return UNKNOWN; };
	virtual uint32			width( ) const = 0;
	virtual uint32			height( ) const = 0;
	virtual D3DFORMAT		format( ) const = 0;
	virtual uint32			textureMemoryUsed( ) const = 0;
	virtual const BW::string& resourceID( ) const;

	virtual void			load();
	virtual void			release();
	virtual void			reload();
	virtual void			reload( const BW::StringRef & resourceID );

	virtual uint32	maxMipLevel() const { return 0; }
	virtual void	maxMipLevel( uint32 level ) {}

	virtual uint32	mipSkip() const { return 0; }
	virtual void	mipSkip( uint32 mipSkip ) {}

	virtual bool	isCubeMap() { return false; }
	virtual bool	isAnimated() { return false; }

	virtual Moo::Colour samplePoint( const Vector2& uv ) { return Colour(0,0,0,0); }

	virtual bool	save( const BW::string& filename );
	Status	status() const;

	void	setStatusHandler( TextureStatusHandler* pHandler );

protected:

	friend void TextureManager::tryDestroy( const BaseTexture * );
	virtual ~BaseTexture();
	void addToManager();
	void tryDestroy() const;

	static uint32	textureMemoryUsed( DX::BaseTexture* pTex );
	void			status( Status status);
	void processLoaderOutput( const TextureLoader& texLoader,
		const FailurePolicy& notFoundPolicy,
		BW::string& inoutQualifiedResourceID,
		TextureLoader::Output& outResult ) const;
	virtual void reportMissingTexture(
		const BW::StringRef& qualifiedResourceID ) const;

private:

	BaseTexture(const BaseTexture&);
	BaseTexture& operator=(const BaseTexture&);
	void onStatusChange(Status newStatus );

	friend std::ostream& operator<<(std::ostream&, const BaseTexture&);

	Status		status_;
	TextureStatusHandler* statusHandler_;

#if ENABLE_RESOURCE_COUNTERS
protected:
	BW::string			allocator_;
#endif
};

class TextureStatusHandler
{
	public:
		virtual void onStatusChange(BaseTexture::Status newstatus) = 0;
};

BW::StringRef removeTextureExtension(const BW::StringRef& resourceID);
BW::string canonicalTextureName(const BW::StringRef& resourceID);

} // namespace Moo

#ifdef CODE_INLINE
#include "base_texture.ipp"
#endif

BW_END_NAMESPACE


#endif // BASE_TEXTURE_HPP
