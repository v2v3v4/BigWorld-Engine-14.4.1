#include "pch.hpp"
#include "texture_feeds.hpp"
#include "moo/material_loader.hpp"
#include "moo/effect_material.hpp"
#include "moo/managed_effect.hpp"
#include "moo/material.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/auto_config.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

int TextureFeeds_token = 0;


BW_INIT_SINGLETON_STORAGE( TextureFeeds )


/**
 *	The texture feed property implements a Moo::EffectProperty
 *	for TextureFeeds.  It is exactly the same as TextureProperty,
 *	which is not exposed from src/lib/moo and thus must be duplicated.
 */
class TextureFeedProperty : public Moo::EffectProperty
{
public:
	TextureFeedProperty( const BW::StringRef & name ) : EffectProperty( name ) {}
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		BW_GUARD;
		if (!value_.hasObject())
			return SUCCEEDED( pEffect->SetTexture( hProperty, NULL ) );
		return SUCCEEDED( pEffect->SetTexture( hProperty, value_->pTexture() ) );
	}

	bool be( const Moo::BaseTexturePtr pTex ) { value_ = pTex; return true; }

	void value( Moo::BaseTexturePtr value ) { value_ = value; };
	const Moo::BaseTexturePtr value() const { return value_; }
	virtual void save( DataSectionPtr pDS )
	{
		pDS->writeString( "TextureFeed", value_->resourceID() );
	}
	EffectProperty* clone() const
	{
		TextureFeedProperty* pClone = new TextureFeedProperty( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	Moo::BaseTexturePtr value_;
};


/**
 *	The TextureFeedPropertyFactory is registered with the
 *	Moo::g_effectPropertyProcessors map, and creates texture properties
 *	that use TextureFeeds by name.  They must have a default texture
 *	for when the texture feed is not available. 
 */
class TextureFeedPropertyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;
		TextureFeedProperty* prop = new TextureFeedProperty( name );
		Moo::BaseTexturePtr pDefaultTexture =
			Moo::TextureManager::instance()->get( pSection->readString(
			"default",
			Moo::TextureManager::notFoundBmp() ) );
		Moo::BaseTexturePtr pTexture = new TextureFeed(
			pSection->asString(), pDefaultTexture );
		prop->value( pTexture );
		return prop;
	}
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;
		TextureFeedProperty* prop = new TextureFeedProperty( name );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;
		return (propertyDesc->Class == D3DXPC_OBJECT &&
			(propertyDesc->Type == D3DXPT_TEXTURE ||
			propertyDesc->Type == D3DXPT_TEXTURE1D ||
			propertyDesc->Type == D3DXPT_TEXTURE2D ||
			propertyDesc->Type == D3DXPT_TEXTURE3D ||
			propertyDesc->Type == D3DXPT_TEXTURECUBE ));
	}
private:
};


/**
 *	This method reads a texture feed section in an old-style
 *	Moo::Material section.
 */
bool readTextureFeed( Moo::Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	int numStages = mat.numTextureStages();
	if ( numStages > 0 )
	{
		Moo::TextureStage & ts = mat.textureStage( numStages-1 );
		ts.pTexture( new TextureFeed( pSect->asString(), ts.pTexture() ) );
	}
	return true;
}

/**
 *	This method registers the TextureFeedPropertyFunctor for use
 *	with Moo::EffectMaterials.
 */
void setupTextureFeedPropertyProcessors()
{
	BW_GUARD;
	Moo::SectionProcessors::instance().insert( Moo::SectProcEntry(
		"TextureFeed", &readTextureFeed ) );

	// if it is initialised, don't overwrite it
	if (!Moo::EffectPropertyFactory::findFactory( "TextureFeed" ))
	{
		Moo::EffectPropertyFactory::setFactory( "TextureFeed",
			new TextureFeedPropertyFactory );
	}
}


/*static*/ void TextureFeeds::addTextureFeed( const BW::string & identifier, PyTextureProviderPtr texture )
{
	BW_GUARD;
	instance().feeds_[ identifier ] = texture;
}


/*~ function BigWorld.addTextureFeed
 *	@components{ client, tools }
 *
 *	This adds a new TextureProvider to BigWorld, identified by a given label.  This label can then be used to refer 
 *	to the TextureProvider in other parts of BigWorld.
 *
 *	@param	identifier	A string label to identify this TextureProvider
 *	@param	texture		A PyTextureProviderPtr that is the TextureProvider to link to this label
*/
PY_MODULE_STATIC_METHOD( TextureFeeds, addTextureFeed, BigWorld )


/*static*/ void TextureFeeds::delTextureFeed( const BW::string & identifier )
{
	BW_GUARD;
	if (pInstance() && !instance().feeds_.empty())
	{
		Feeds::iterator it = instance().feeds_.find( identifier );
		if (it != instance().feeds_.end())
		{
			instance().feeds_.erase( it );
		}
	}
}

/*~ function BigWorld.delTextureFeed
 *	@components{ client, tools }
 *
 *	This function removes a previously added TextureProvider.  The TextureProvider can still be used in script, but 
 *	cannot be referred to by its previously associated label anymore.
 *
 *	@param	identifier	A string label to identity the TextureProvider to remove
 */
PY_MODULE_STATIC_METHOD( TextureFeeds, delTextureFeed, BigWorld )


PyTextureProviderPtr TextureFeeds::getTextureFeed( const BW::string& identifier )
{
	BW_GUARD;
	if (!instance().feeds_.empty())
	{
		Feeds::iterator it = instance().feeds_.find( identifier );
		if (it != instance().feeds_.end())
		{
			return it->second;
		}
	}
	return NULL;
}

/*~ function BigWorld.getTextureFeed
 *	@components{ client, tools }
 *
 *	This function returns a previously added TextureProvider.
 *
 *	@param	identifier	A string label to identity the TextureProvider to return
 */
PY_MODULE_STATIC_METHOD( TextureFeeds, getTextureFeed, BigWorld )


Moo::BaseTexturePtr TextureFeeds::get( const BW::string& identifier ) 
{
	BW_GUARD;
	if (!instance().feeds_.empty())
	{
		Feeds::iterator it = instance().feeds_.find( identifier );
		if (it != instance().feeds_.end() && it->second)
		{
			return it->second->texture();
		}
	}
	return NULL;
}



TextureFeed::TextureFeed( const BW::string& resourceID, Moo::BaseTexturePtr defaultTexture ):
	resourceID_( resourceID ),
	default_( defaultTexture )
{
	BW_GUARD;
	MF_ASSERT( default_ );
}


DX::BaseTexture* TextureFeed::pTexture( )
{
	BW_GUARD;
	return current()->pTexture();
}


uint32 TextureFeed::width( ) const
{
	BW_GUARD;
	return current()->width();
}


uint32 TextureFeed::height( ) const
{
	BW_GUARD;
	return current()->height();
}


D3DFORMAT TextureFeed::format( ) const
{
	BW_GUARD;
	return current()->format();
}


uint32 TextureFeed::textureMemoryUsed() const
{
	BW_GUARD;
	return current()->textureMemoryUsed();
}

BW_END_NAMESPACE

// texture_feeds.cpp
