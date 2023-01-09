#include "pch.hpp"

#include "texture_atlas.hpp"
#include "moo/texture_manager.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- minimum and maximum acceptable sub-texture sizes which may be used in texture atlas.
    const uint32 g_minSubTextureSize  = 64;
    const uint32 g_maxSubTextureSize  = 1024;
    const uint32 g_compressedTileSize = 16;

    //----------------------------------------------------------------------------------------------
    VOID WINAPI fillTextureDebugColour( D3DXVECTOR4* pOut, 
        CONST D3DXVECTOR2* pTexCoord, 
        CONST D3DXVECTOR2* pTexelSize, 
        LPVOID pData )
    {
        *pOut = D3DXVECTOR4(1.0f,0,1.0f,1.0f); // Pink
    }

    //----------------------------------------------------------------------------------------------
    bool isPowerOfTwo(uint32 x)
    {
        return x && ((x & (x - 1)) == 0);
    }

    //-- ToDo: reconsider.
    //-- Fill block of texture data with DXT3 compression.
    //----------------------------------------------------------------------------------------------
    void blockFill(void* destination, uint32 size)
    {
        uint8* dest = (uint8*)destination;
        const uint32 block[4] = { 0x0, 0x0, 0x0, 0x0 };
        size = size / g_compressedTileSize;
        for (uint32 i = 0; i < size; i++)
        {
            CopyMemory( dest, block, 16 );
            dest += 16;
        }
    }

    //-- ToDo: reconsider.
    //-- save texture on disk.
    //----------------------------------------------------------------------------------------------
    bool saveTextureAtlas(const BW::string& name, Moo::BaseTexture* texture)
    {
        BW_GUARD;

        //-- 1. firstly create in memory texture with the same format size and mip-map count.
        ComObjectWrap<DX::Texture> inmemTexture = Moo::rc().createTexture(
            texture->width(), texture->height(), texture->maxMipLevel(), 0, texture->format(), D3DPOOL_SYSTEMMEM, ""
            );

        if (!inmemTexture.hasComObject())
            return false;

        //-- 2. iterate over the whole set of mip-map levels and copy them from un-managend input
        //--    texture into system texture.
        ComObjectWrap<DX::Surface> inmemSurface;
        ComObjectWrap<DX::Surface> srcSurface;

        for (uint i = 0; i < texture->maxMipLevel(); ++i)
        {
            HRESULT hr  = static_cast<DX::Texture*>(texture->pTexture())->GetSurfaceLevel(i, &srcSurface);
            hr          = inmemTexture->GetSurfaceLevel(i, &inmemSurface);
            hr          = Moo::rc().device()->StretchRect(srcSurface.pComObject(), NULL, inmemSurface.pComObject(), NULL, D3DTEXF_LINEAR);

            if (
                FAILED(static_cast<DX::Texture*>(texture->pTexture())->GetSurfaceLevel(i, &srcSurface)) ||
                FAILED(inmemTexture->GetSurfaceLevel(i, &inmemSurface)) ||
                FAILED(Moo::rc().device()->StretchRect(srcSurface.pComObject(), NULL, inmemSurface.pComObject(), NULL, D3DTEXF_LINEAR))
                )
            { 
                return false;
            }
        }

        //-- 3. and finally save this texture on disk.
        bool success = SUCCEEDED(D3DXSaveTextureToFileA(
            name.c_str(), D3DXIFF_DDS, inmemTexture.pComObject(), NULL
            ));

        return success;
    }

    //-- This is the functor class for TileTexture::SubTexture type.
    //----------------------------------------------------------------------------------------------
    class SubTextureFinder
    {
    public:
        SubTextureFinder(const BW::string& name) : m_name(name) { }

        bool operator ()(const TextureAtlas::SubTextureData& subTex) 
        {
            return m_name == subTex.m_name;
        }

    private:
        BW::string m_name;
    };


    //-- sub-texture background loader for the desired texture atlas.
    //----------------------------------------------------------------------------------------------
    class DecalLoader : public BackgroundTask
    {
    public:
        DecalLoader(TextureAtlas& ta, const BW::string& name, 
            const Moo::BaseTexturePtr& source, 
            const TextureAtlas::SubTexturePtr& subTexture, bool async ) :   
            BackgroundTask( "DecalLoader" ), 
            m_subTexture( subTexture ), 
            m_tex( source ), 
            m_async( async ), 
            m_ta( ta ), 
            m_name( name )
          {
              BW_GUARD;

              //-- force disable async load in case if we already have loaded texture.
              if (m_tex)
              {
                  m_async = false;
              }

              //--
              if (m_async)
              {
                  BgTaskManager::instance().addBackgroundTask(this);
              }
              else
              {
                  doBackgroundTask(BgTaskManager::instance());
                  doMainThreadTask(BgTaskManager::instance());
              }
          }

    private:

        void doBackgroundTask(TaskManager& mgr)
        {
            BW_GUARD;
            
            //-- do loading texture only in case if we don't have it already loaded.
            if (!m_tex)
            {
                m_tex = Moo::TextureManager::instance()->getSystemMemoryTexture(m_name);
            }

            //--
            if (m_async)
            {
                mgr.addMainThreadTask(this);
            }       
        }

        void doMainThreadTask(TaskManager& mgr)
        {       
            BW_GUARD_PROFILER( DecalLoader_doMainThreadTask );

            //--
            if (!m_tex)
            {
                ERROR_MSG("[DECAL] Can't load '%s' texture file.\n", m_name.c_str());
            }

            //-- mark as loaded.
            m_subTexture->m_loaded = true;
            //-- mark as valid.
            m_subTexture->m_valid  = m_tex && m_ta.copySubTexture(m_subTexture, m_tex);
        }

    private:
        bool                         m_async;
        BW::string                   m_name;
        TextureAtlas&                m_ta;
        Moo::BaseTexturePtr          m_tex;
        TextureAtlas::SubTexturePtr  m_subTexture;
    };
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


//--------------------------------------------------------------------------------------------------
TextureAtlas::TextureAtlas(uint32 width, uint32 height, const BW::string& name)
    :   m_width(width), m_height(height), m_numMips(0), m_name(name), m_bspTree(width, height)
{
    BW_GUARD;
    MF_ASSERT(isPowerOfTwo(width) && isPowerOfTwo(height));

    m_subTextures.reserve(100);

    //-- add this texture to the texture manager to receive some messages like reload or change
    //-- texture quality.
    addToManager();
}

//--------------------------------------------------------------------------------------------------
TextureAtlas::~TextureAtlas()
{
    BW_GUARD;
}


//--------------------------------------------------------------------------------------------------
void TextureAtlas::destroy() const
{
    BW_GUARD;
    this->tryDestroy();
}


//--------------------------------------------------------------------------------------------------
void TextureAtlas::reload()
{
    BW_GUARD;

    deleteUnmanagedObjects();
    createUnmanagedObjects();
}

//--------------------------------------------------------------------------------------------------
bool TextureAtlas::recreateForD3DExDevice() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void TextureAtlas::createUnmanagedObjects()
{
    BW_GUARD;

    //-- calculate maximum of mip-map level acceptable for texture atlas.
    //-- Note: We have to calculate maximum number of mip-map lod level for the smallest sub-texture
    //--       acceptable for the texture atlas to prevent incorrect sampling from the higher mip-map
    //--       lod level. For example 64x64 texture has 7 mip levels, while 512x512 has 10 levels. So
    //--       it's clear error if we try sample from the 8-th level for the 64x64 sub-texture. Thus
    //--       to eliminate this incorrect behavior we have to limit minimum (highest) lod level for
    //--       the whole texture atlas.
    //-- ToDo: reconsider this formula.
    m_numMips = 0;
    uint32 mipMapTiles = (g_minSubTextureSize * g_minSubTextureSize) / g_compressedTileSize;

    while (mipMapTiles != 0)
    {
        ++m_numMips;
        mipMapTiles /= 4;
    }

    m_texture = Moo::rc().createTexture(
        m_width, m_height, m_numMips, 0, D3DFMT_DXT3, D3DPOOL_DEFAULT, "texture/atlas"
        );

    if (m_texture)
    {
        clearAtlasMap();

        //-- load all sub-textures synchronously, if sub-texture has flag m_loaded in false it means
        //-- that texture currently is being load in background thread, so we should skip it.
        for (SubTextures::iterator it = m_subTextures.begin(); it != m_subTextures.end(); ++it)
        {
            const SubTexturePtr& st = it->m_subTexture;

            if (st->m_loaded && st->m_valid)
            {
                DecalLoader dl(*this, it->m_name, it->m_source, st, false /*async*/);           
            }
        }
    }

    MF_ASSERT(m_texture && "Can't create texture atlas map.");
}

//--------------------------------------------------------------------------------------------------
void TextureAtlas::deleteUnmanagedObjects()
{
    BW_GUARD;

    //-- ToDo: try save texture on disk.
#if 0
    saveTextureAtlas(m_name + ".dds", this);
#endif

    evictUnused( false );
    m_bspTree.clear();
    m_texture = NULL;
}

//-- Copy supplied texture into sub-texture at given index.
//--------------------------------------------------------------------------------------------------
bool TextureAtlas::copySubTexture(
        const TextureAtlas::SubTexturePtr& subTexture, const Moo::BaseTexturePtr& texture)
{
    BW_GUARD;

    if (!validateTexture(texture))
        return false;

    DX::Texture* pTex = (DX::Texture*)texture->pTexture();

    if (!m_texture.pComObject())
    {
        createUnmanagedObjects();
    }
    if (m_texture.pComObject())
    {
        //-- try to insert sub-texture into texture atlas via bspTree.
        if (!m_bspTree.insert(subTexture->m_rect, texture->width(), texture->height(), subTexture.get()))
        {
            ERROR_MSG("[DECAL] Not enough space in the texture atlas to insert %s decal with size %d x %d.\n",
                texture->resourceID().c_str(), texture->width(), texture->height()
                );
            return false;
        }

        //--
        RECT srcRect;
        srcRect.left   = 0;
        srcRect.right  = texture->width();
        srcRect.top    = 0;
        srcRect.bottom = texture->height();

        //--
        POINT destPt;
        destPt.x = subTexture->m_rect.m_left;
        destPt.y = subTexture->m_rect.m_top;

        //-- iterate over the whole mip-map chain and copy mip-map data from sub-texture into the atlas.
        for (uint32 mip = 0; mip < m_numMips; ++mip)
        {
            ComObjectWrap<DX::Surface> pDestSurface;
            HRESULT hr = m_texture->GetSurfaceLevel(mip, &pDestSurface);
            if (SUCCEEDED(hr))
            {
                ComObjectWrap<DX::Surface> pSrcSurface;
                hr = pTex->GetSurfaceLevel(mip, &pSrcSurface);
                if (SUCCEEDED(hr))
                {
                    //-- copy data from small map's mip-map to large map's mip-map.
                    hr = Moo::rc().device()->UpdateSurface(pSrcSurface.pComObject(),
                        &srcRect, 
                        pDestSurface.pComObject(), 
                        &destPt);
                }
                else
                {
                    ERROR_MSG( "TextureAtlas::copySubTexture: m_texture->GetSurfaceLevel failed: %s\n",
                        DX::errorAsString(hr).c_str() );

                    //-- fill the mip with a pink colour so that the problem will get noticed.
                    D3DSURFACE_DESC surfaceDesc;
                    pDestSurface->GetDesc( &surfaceDesc );

                    ComObjectWrap<DX::Texture> pColTex;
                    Moo::rc().device()->CreateTexture(
                        srcRect.right-srcRect.left, srcRect.bottom-srcRect.top,
                        1, 0, surfaceDesc.Format, D3DPOOL_SYSTEMMEM, &pColTex, NULL );
                    if (pColTex)
                    {
                        hr = D3DXFillTexture( pColTex.pComObject(), fillTextureDebugColour, NULL );
                        if (SUCCEEDED(pColTex->GetSurfaceLevel( 0, &pSrcSurface )))
                        {
                            Moo::rc().device()->UpdateSurface( pSrcSurface.pComObject(),
                                NULL, pDestSurface.pComObject(), &destPt );
                        }
                    }
                }
            }
            else
            {
                ERROR_MSG( "TextureAtlas::copySubTexture: pTex->GetSurfaceLevel failed: %s\n",
                    DX::errorAsString(hr).c_str() );
            }

            //-- advance to next mip-map.
            destPt.x /= 2;  
            destPt.y /= 2;  
            srcRect.right  = max((srcRect.right  / 2), 1L);
            srcRect.bottom = max((srcRect.bottom / 2), 1L);
        }
    }
    return true;
}

//-- This method validates that the passed in texture is suitable for use by the texture atlas.
//--------------------------------------------------------------------------------------------------
bool TextureAtlas::validateTexture(const Moo::BaseTexturePtr& texture)
{
    BW_GUARD;

    //-- check texture format.
    DX::BaseTexture* pBase = texture->pTexture();
    if (pBase->GetType() != D3DRTYPE_TEXTURE)
    {
        ERROR_MSG("[DECAL] decal's texture '%s' has the wrong format, it must be of type D3DRTYPE_TEXTURE.\n",
            texture->resourceID().c_str()
            );
        return false;
    }
        
    DX::Texture* pTex = (DX::Texture*)pBase;
    D3DSURFACE_DESC desc;
    pTex->GetLevelDesc(0, &desc);

    //-- check texture pixel format.
    if (desc.Format != D3DFMT_DXT3)
    {
        ERROR_MSG("[DECAL] decal's texture '%s' has the wrong format, it should be DXT3.\n",
            texture->resourceID().c_str()
            );
        return false;
    }

    //-- check texture size.
    if (
        desc.Width  > g_maxSubTextureSize || desc.Width  < g_minSubTextureSize ||
        desc.Height > g_maxSubTextureSize || desc.Height < g_minSubTextureSize
        )
    {
        ERROR_MSG("[DECAL] decal's texture '%s' has the wrong size, it should in the range (%d, %d).\n",
            texture->resourceID().c_str(), g_minSubTextureSize, g_maxSubTextureSize
            );
        return false;
    }
    
    //-- check is texture a power of two texture.
    if (!isPowerOfTwo(desc.Width) || !isPowerOfTwo(desc.Height))
    {
        ERROR_MSG("[DECAL] decal's texture '%s' has the wrong size, it should be a power of two size texture.\n",
            texture->resourceID().c_str()
            );
        return false;
    }

    return true;
}

//-- This method adds a sub-texture to the TextureAtlas. It returns the index straight away, however
//-- any texture loading that occurs as a result will be done asynchronously.
//--------------------------------------------------------------------------------------------------
TextureAtlas::SubTexturePtr TextureAtlas::subTexture(const BW::string& name, const Moo::BaseTexturePtr& source, bool async)
{
    BW_GUARD;

    //-- evict unused textures every time when we try create a new one.
    evictUnused();
    
    //-- firstly try to find sub-texture in the cache.
    SubTextures::iterator it = std::find_if(
        m_subTextures.begin(), m_subTextures.end(), SubTextureFinder(name)
        );

    if (it != m_subTextures.end())
    {
        return it->m_subTexture;
    }
    else
    {
        //-- add to the list.
        m_subTextures.push_back(SubTextureData(name, source, new SubTexture()));
        SubTextureData& data = m_subTextures.back();

        if (async)
        {
            //-- start background loading.
            //-- Note: the lifetime of this class is handled by the background loader.
            new DecalLoader(*this, data.m_name, data.m_source, data.m_subTexture, true/*async*/);
            return data.m_subTexture;
        }
        else
        {
            DecalLoader loader(*this, data.m_name, data.m_source, data.m_subTexture, false);
            return data.m_subTexture->m_valid ? data.m_subTexture : NULL;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void TextureAtlas::evictUnused( bool doClearAtlasMap /*= true*/ )
{
    BW_GUARD;

    //-- iterate over the whole set of sub-textures and delete unused sub-textures from the texture atlas.
    for (int i = 0; i < (int)m_subTextures.size(); ++i)
    {
        const SubTexture* st = m_subTextures[i].m_subTexture.get();

        if (st->refCount() == 1 || (!st->m_valid && st->m_loaded))
        {
            //-- delete from BSP tree.
            m_bspTree.remove(st);

            //-- remove from container.
            m_subTextures[i--] = m_subTextures.back();
            m_subTextures.pop_back();
        }
    }

    //-- if has been evicted all sub-textures then clear bsp tree.
    if (m_subTextures.empty())
    {
        m_bspTree.clear();

        //-- clear texture map
        if (doClearAtlasMap && m_texture)
        {
            clearAtlasMap();
        }
    }
}

//--------------------------------------------------------------------------------------------------
void TextureAtlas::clearAtlasMap()
{
    BW_GUARD;
    MF_ASSERT(m_texture);

    ComObjectWrap<DX::Texture> sysCopy = Moo::rc().createTexture(m_width, m_height, m_numMips, 0, 
        D3DFMT_DXT3, D3DPOOL_SYSTEMMEM, "texture/atlas"
        );

    if (sysCopy)
    {
        //-- fill our atlas with default data.
        uint32 nLevels = sysCopy->GetLevelCount();
        for (uint32 i = 0; i < nLevels; ++i)
        {
            D3DSURFACE_DESC sd;
            sysCopy->GetLevelDesc(i, &sd);
            D3DLOCKED_RECT lr;
            sysCopy->LockRect(i, &lr, NULL, 0);

            //-- DXT3 surfaces use one byte per texel, so width x height gives us the surface size
            blockFill(lr.pBits, sd.Width * sd.Height);

            //-- This fixes a bug on nvidia cards, they don't like compressed dxt3 textures 
            //-- to be initialized to all full alpha...
            *(char*)lr.pBits = 0;

            sysCopy->UnlockRect(i);
        }
        Moo::rc().device()->UpdateTexture(sysCopy.pComObject(), m_texture.pComObject());
    }
}

//--------------------------------------------------------------------------------------------------
uint32 TextureAtlas::textureMemoryUsed() const
{
    DX::Texture* pTex = const_cast<DX::Texture*>( m_texture.pComObject() );
    return Moo::BaseTexture::textureMemoryUsed( pTex ); 
}

BW_END_NAMESPACE
