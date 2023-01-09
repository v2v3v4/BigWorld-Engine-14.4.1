#ifndef TEXTURE_FEEDS_HPP
#define TEXTURE_FEEDS_HPP

#include "moo/base_texture.hpp"
#include "cstdmf/init_singleton.hpp"
#include "py_texture_provider.hpp"


BW_BEGIN_NAMESPACE

/// This token should be referenced to link in the TextureFeeds script functions.
extern int TextureFeeds_token;

typedef SmartPointer<PyTextureProvider>	PyTextureProviderPtr;

/**
 * TODO: to be documented.
 */
class TextureFeeds : public Singleton< TextureFeeds >
{
	typedef TextureFeeds This;
public:
	static void addTextureFeed( const BW::string& identifier, PyTextureProviderPtr texture );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, addTextureFeed, ARG( BW::string, ARG( PyTextureProviderPtr, END ) ) )
	static void delTextureFeed( const BW::string& identifier );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, delTextureFeed, ARG( BW::string, END ) )
	static PyTextureProviderPtr getTextureFeed( const BW::string& identifier );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETDATA, getTextureFeed, ARG( BW::string, END ) )
	static Moo::BaseTexturePtr get( const BW::string& identifier );

private:	
	typedef StringHashMap< PyTextureProviderPtr > Feeds;
	Feeds feeds_;
};

/**
 *	This class delegates the pTexture accessor between
 *	the base texture
 */
class TextureFeed : public Moo::BaseTexture
{
public:
	TextureFeed( const BW::string& resourceID, Moo::BaseTexturePtr defaultTexture );

	//methods inherited from BaseTexture
	virtual DX::BaseTexture*	pTexture( );
	virtual uint32			width( ) const;
	virtual uint32			height( ) const;
	virtual D3DFORMAT		format( ) const;
	virtual uint32			textureMemoryUsed( ) const;

private:
	const Moo::BaseTexturePtr current() const
	{
		Moo::BaseTexturePtr found = TextureFeeds::instance().get( resourceID_ );
		return found ? found : default_;
	}
	Moo::BaseTexturePtr		default_;
	BW::string				resourceID_;
};

BW_END_NAMESPACE

#endif // TEXTURE_FEEDS_HPP

