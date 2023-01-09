#pragma once

#include "moo/device_callback.hpp"
#include "moo/base_texture.hpp"
#include "bsp_tree_2d.hpp"

BW_BEGIN_NAMESPACE

//-- Texture atlas may collect power of two textures in much bigger texture to prevent memory
//-- fragmentation with manual DX API creation of these textures and to minimize state changes during
//-- rendering. You may dynamically add and delete sub-textures to\from the texture atlas. The atlas
//-- texture located in the un-managed pool so it doesn't waste any additional system memory but
//-- TextureAtlas has to take a look at each loaded textures and reload them and recreate atlas every
//-- time when we lost DX device.
//--------------------------------------------------------------------------------------------------
class TextureAtlas : public Moo::BaseTexture
{
private:
	//-- make non-copyable.
	TextureAtlas(const TextureAtlas&);
	TextureAtlas& operator = (const TextureAtlas&);

public:

	//-- Single entry in the decal texture atlas.
	struct SubTexture : public SafeReferenceCount
	{
		SubTexture() : m_loaded(false), m_valid(true), m_rect(0,0,1,1) { }

		bool	m_loaded;
		bool	m_valid;
		Rect2D	m_rect;
	};
	typedef SmartPointer<SubTexture> SubTexturePtr;

	//--
	struct SubTextureData
	{
		SubTextureData(const BW::string& name, const Moo::BaseTexturePtr& source, const SubTexturePtr& subTexture)
			:	m_name(name), m_source(source), m_subTexture(subTexture) { }

		BW::string			m_name;
		Moo::BaseTexturePtr	m_source;
		SubTexturePtr		m_subTexture;
	};
	typedef BW::vector<SubTextureData>	SubTextures;

public:
	//-- Warning: currently supported only DXT3 texture format.
	TextureAtlas(uint32 width, uint32 height, const BW::string& name);

	//-- Note:		returns an existing sub-texture and automatically increment refs counter or a
	//--			new one if desired sub-texture doesn't exist.
	//-- Note:		If <source> is a valid pointer then it's used as a source for uploading into
	//--			the atlas. Operation performs in the caller thread and async flag is ignored.
	//-- Warning:	Be sure that pair <name, source> always unique because if not you will have
	//--			the same texture in the atlas but with different names.
	SubTexturePtr				subTexture(const BW::string& name, const Moo::BaseTexturePtr& source = NULL, bool async = false);

	//-- interface Moo::BaseTexture.
	virtual void				reload();
	virtual DX::BaseTexture*	pTexture()			{ return m_texture.pComObject(); }
	virtual uint32				width() const		{ return m_width; }
	virtual uint32				height() const		{ return m_height; }
	virtual D3DFORMAT			format() const		{ return D3DFMT_DXT3; }
	virtual uint32				textureMemoryUsed()	const;
	virtual const BW::string&	resourceID() const	{ return m_name; }
	virtual uint32				maxMipLevel() const { return m_numMips; }
	uint32						mipLevels() const	{ return m_numMips; }
	
	//-- have to be called by the owner whenever create/destroy unmanaged resources needed.
	void						createUnmanagedObjects();
	void						deleteUnmanagedObjects();
	bool						recreateForD3DExDevice() const;

	//-- copy desired texture data into the atlas. Copying is performed on the main thread.
	bool copySubTexture(const SubTexturePtr& subTexture, const Moo::BaseTexturePtr& texture);

private:
	virtual void destroy() const;
	~TextureAtlas();

	void evictUnused( bool doClearAtlasMap = true );
	void clearAtlasMap();
	bool validateTexture(const Moo::BaseTexturePtr& tex);

private:
	ComObjectWrap<DX::Texture>	m_texture;
	BW::string					m_name;
	uint32						m_width;
	uint32						m_height;
	uint32						m_numMips;
	SubTextures					m_subTextures;
	BSPTree2D<SubTexture>		m_bspTree;
};

BW_END_NAMESPACE
