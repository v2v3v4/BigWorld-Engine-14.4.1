
#include "pch.hpp"
#include "texture_explorer.hpp"
#include "geometrics.hpp"

#include "moo/texture_manager.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TextureExplorer
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TextureExplorer::TextureExplorer()
:	enabled_( false ),
	width_( 0 ),
	height_( 0 ),
	currentIndex_( 0 ),
	index_( 0 ),
	fitToScreen_( false ),
	preserveAspect_( true ),
	doReload_( false ),
	maxMipLevel_( 0 ),
	alpha_( false )
{
	// Texture explorer enabled?
	MF_WATCH( "Debug/TextureExplorer/enabled", enabled_ );
	// Texture index
	MF_WATCH( "Debug/TextureExplorer/textureIndex", index_ );
	// Fit to screen?
	MF_WATCH( "Debug/TextureExplorer/fitToScreen", fitToScreen_ );
	// Preserve texture aspect ratio?
	MF_WATCH( "Debug/TextureExplorer/preserveAspectRatio", preserveAspect_ );
	// Texture name
	MF_WATCH( "Debug/TextureExplorer/name", textureName_, Watcher::WT_READ_ONLY );
	// Texture path
	MF_WATCH( "Debug/TextureExplorer/path", texturePath_, Watcher::WT_READ_ONLY );
	// Texture format
	MF_WATCH( "Debug/TextureExplorer/format", format_, Watcher::WT_READ_ONLY );
	// Texture width
	MF_WATCH( "Debug/TextureExplorer/width", width_, Watcher::WT_READ_ONLY );
	// Texture height
	MF_WATCH( "Debug/TextureExplorer/height", height_, Watcher::WT_READ_ONLY );
	// Memory used by texture
	MF_WATCH( "Debug/TextureExplorer/memory", memoryUsage_ );
	// Reload texture
	MF_WATCH( "Debug/TextureExplorer/reload", doReload_ );
	// Maximum mip map level
	MF_WATCH( "Debug/TextureExplorer/maxMipLevel", maxMipLevel_ );
	// Show alpha channel
	MF_WATCH( "Debug/TextureExplorer/showAlpha", alpha_ );

	surfaceFormats_[D3DFMT_A8R8G8B8] = "D3DFMT_A8R8G8B8";
    surfaceFormats_[D3DFMT_X8R8G8B8] = "D3DFMT_X8R8G8B8";
    surfaceFormats_[D3DFMT_R5G6B5] = "D3DFMT_R5G6B5";
    surfaceFormats_[D3DFMT_X1R5G5B5] = "D3DFMT_X1R5G5B5";
    surfaceFormats_[D3DFMT_A1R5G5B5] = "D3DFMT_A1R5G5B5";
    surfaceFormats_[D3DFMT_A4R4G4B4] = "D3DFMT_A4R4G4B4";
    surfaceFormats_[D3DFMT_A8] = "D3DFMT_A8";
    surfaceFormats_[D3DFMT_A8B8G8R8] = "D3DFMT_A8B8G8R8";
    surfaceFormats_[D3DFMT_B8G8R8A8] = "D3DFMT_B8G8R8A8";
    surfaceFormats_[D3DFMT_R4G4B4A4] = "D3DFMT_R4G4B4A4";
    surfaceFormats_[D3DFMT_R5G5B5A1] = "D3DFMT_R5G5B5A1";
    surfaceFormats_[D3DFMT_R8G8B8A8] = "D3DFMT_R8G8B8A8";
    surfaceFormats_[D3DFMT_R8B8] = "D3DFMT_R8B8";
    surfaceFormats_[D3DFMT_G8B8] = "D3DFMT_G8B8";
    surfaceFormats_[D3DFMT_P8] = "D3DFMT_P8";
    surfaceFormats_[D3DFMT_L8] = "D3DFMT_L8";
    surfaceFormats_[D3DFMT_A8L8] = "D3DFMT_A8L8";
    surfaceFormats_[D3DFMT_AL8] = "D3DFMT_AL8";
    surfaceFormats_[D3DFMT_L16] = "D3DFMT_L16";
    surfaceFormats_[D3DFMT_V8U8] = "D3DFMT_V8U8";
    surfaceFormats_[D3DFMT_L6V5U5] = "D3DFMT_L6V5U5";
    surfaceFormats_[D3DFMT_X8L8V8U8] = "D3DFMT_X8L8V8U8";
    surfaceFormats_[D3DFMT_Q8W8V8U8] = "D3DFMT_Q8W8V8U8";
    surfaceFormats_[D3DFMT_V16U16] = "D3DFMT_V16U16";
	surfaceFormats_[D3DFMT_D16_LOCKABLE] = "D3DFMT_D16_LOCKABLE";
    surfaceFormats_[D3DFMT_D16] = "D3DFMT_D16";
    surfaceFormats_[D3DFMT_D24S8] = "D3DFMT_D24S8";
    surfaceFormats_[D3DFMT_F16] = "D3DFMT_F16";
    surfaceFormats_[D3DFMT_F24S8] = "D3DFMT_F24S8";
    surfaceFormats_[D3DFMT_UYVY] = "D3DFMT_UYVY";
    surfaceFormats_[D3DFMT_YUY2] = "D3DFMT_YUY2";
    surfaceFormats_[D3DFMT_DXT1] = "D3DFMT_DXT1";
    surfaceFormats_[D3DFMT_DXT2] = "D3DFMT_DXT2";
    surfaceFormats_[D3DFMT_DXT3] = "D3DFMT_DXT3";
    surfaceFormats_[D3DFMT_DXT4] = "D3DFMT_DXT4";
    surfaceFormats_[D3DFMT_DXT5] = "D3DFMT_DXT5";
    surfaceFormats_[D3DFMT_LIN_A1R5G5B5] = "D3DFMT_LIN_A1R5G5B5";
    surfaceFormats_[D3DFMT_LIN_A4R4G4B4] = "D3DFMT_LIN_A4R4G4B4";
    surfaceFormats_[D3DFMT_LIN_A8] = "D3DFMT_LIN_A8";
    surfaceFormats_[D3DFMT_LIN_A8B8G8R8] = "D3DFMT_LIN_A8B8G8R8";
    surfaceFormats_[D3DFMT_LIN_A8R8G8B8] = "D3DFMT_LIN_A8R8G8B8";
    surfaceFormats_[D3DFMT_LIN_B8G8R8A8] = "D3DFMT_LIN_B8G8R8A8";
    surfaceFormats_[D3DFMT_LIN_G8B8] = "D3DFMT_LIN_G8B8";
    surfaceFormats_[D3DFMT_LIN_R4G4B4A4] = "D3DFMT_LIN_R4G4B4A4";
    surfaceFormats_[D3DFMT_LIN_R5G5B5A1] = "D3DFMT_LIN_R5G5B5A1";
    surfaceFormats_[D3DFMT_LIN_R5G6B5] = "D3DFMT_LIN_R5G6B5";
    surfaceFormats_[D3DFMT_LIN_R6G5B5] = "D3DFMT_LIN_R6G5B5";
    surfaceFormats_[D3DFMT_LIN_R8B8] = "D3DFMT_LIN_R8B8";
    surfaceFormats_[D3DFMT_LIN_R8G8B8A8] = "D3DFMT_LIN_R8G8B8A8";
    surfaceFormats_[D3DFMT_LIN_X1R5G5B5] = "D3DFMT_LIN_X1R5G5B5";
    surfaceFormats_[D3DFMT_LIN_X8R8G8B8] = "D3DFMT_LIN_X8R8G8B8";
    surfaceFormats_[D3DFMT_LIN_A8L8] = "D3DFMT_LIN_A8L8";
    surfaceFormats_[D3DFMT_LIN_AL8] = "D3DFMT_LIN_AL8";
    surfaceFormats_[D3DFMT_LIN_L16] = "D3DFMT_LIN_L16";
    surfaceFormats_[D3DFMT_LIN_L8] = "D3DFMT_LIN_L8";
    surfaceFormats_[D3DFMT_LIN_D24S8] = "D3DFMT_LIN_D24S8";
    surfaceFormats_[D3DFMT_LIN_F24S8] = "D3DFMT_LIN_F24S8";
    surfaceFormats_[D3DFMT_LIN_D16] = "D3DFMT_LIN_D16";
    surfaceFormats_[D3DFMT_LIN_F16] = "D3DFMT_LIN_F16";
    surfaceFormats_[D3DFMT_VERTEXDATA] = "D3DFMT_VERTEXDATA";
    surfaceFormats_[D3DFMT_INDEX16] = "D3DFMT_INDEX16";

	pColourMaterial_ = new Moo::Material;
	Moo::TextureStage ts;
	Moo::TextureStage ts2;
	ts.colourOperation( Moo::TextureStage::SELECTARG1 );
	pColourMaterial_->fogged(false);
	pColourMaterial_->zBufferRead( false );
	pColourMaterial_->zBufferWrite( false );
	pColourMaterial_->addTextureStage( ts );
	pColourMaterial_->addTextureStage( ts2 );

	pAlphaMaterial_ = new Moo::Material;

	ts.colourOperation( Moo::TextureStage::SELECTARG1, Moo::TextureStage::TEXTURE_ALPHA, Moo::TextureStage::DIFFUSE );
	pAlphaMaterial_->fogged(false);
	pAlphaMaterial_->zBufferRead( false );
	pAlphaMaterial_->zBufferWrite( false );
	pAlphaMaterial_->addTextureStage( ts );
	pAlphaMaterial_->addTextureStage( ts2 );


/*	pMaterial_ = new Moo::Material;
	pMaterial_->fogged( false );
	Moo::TextureStage ts;
	ts.colourOperation( Moo::TextureStage::SELECTARG1 );
	pMaterial_->addTextureStage( ts );
	pMaterial_->addTextureStage( Moo::TextureStage() );*/
}


/**
 *	Destructor.
 */
TextureExplorer::~TextureExplorer()
{
}


void TextureExplorer::tick()
{
	const Moo::TextureManager::TextureList& texList = Moo::TextureManager::instance()->textureList();

	typedef Moo::TextureManager::TextureList::const_iterator TexIt;

	if (!pTexture_.hasObject())
	{
		if (texList.size())
		{
			pTexture_ = texList.front();
			maxMipLevel_ = pTexture_->maxMipLevel();
		}
	}

	if (pTexture_.hasObject())
	{
		if (index_ != currentIndex_)
		{
			int indexDiff = index_ - currentIndex_;
			TexIt it = std::find( texList.begin(), texList.end(), &*pTexture_);

			if (it != texList.end())
			{
				if (indexDiff > 0)
				{
					while (indexDiff != 0)
					{
                        it++;
						if (it == texList.end())
							it = texList.begin();
						indexDiff--;
					}
				}
				else
				{
					while (indexDiff != 0)
					{
						if (it == texList.begin())
							it = texList.end();
						it--;
						indexDiff++;
					}
				}

				pTexture_ = *it;
				maxMipLevel_ = pTexture_->maxMipLevel();
			}
			else
			{
				pTexture_ = texList.front();
				maxMipLevel_ = pTexture_->maxMipLevel();
			}


			BW::string resID = pTexture_->resourceID();
			int lastSlash = resID.find_last_of( "/\\" );
			if (lastSlash < int(resID.length()) && 
				lastSlash > 0)
			{
				texturePath_ = resID.substr( 0, lastSlash );
				textureName_ = resID.substr( lastSlash + 1 );
			}
			else
			{
				texturePath_ = "";
				textureName_ = resID;
			}
		}

		width_ = pTexture_->width();
		height_ = pTexture_->height();
		memoryUsage_ = pTexture_->textureMemoryUsed();

		DX::BaseTexture* pTexture = pTexture_->pTexture();
		if (pTexture)
		{
			uint32 format = (pTexture->Format >> 8) & 0xff;
			Formats::iterator it = surfaceFormats_.find( format );
			if (it != surfaceFormats_.end())
			{
				format_ = it->second;
			}
			else 
				format_ = "";
		}
		else
			format_ = "";

		if (doReload_)
		{
			doReload_ = false;
			if (pTexture_)
				pTexture_->reload();
		}
		if (maxMipLevel_ != pTexture_->maxMipLevel())
		{
			pTexture_->maxMipLevel( maxMipLevel_ );
			maxMipLevel_ = pTexture_->maxMipLevel();
		}
	}
	currentIndex_ = index_;
}


void TextureExplorer::draw()
{
	if (enabled_ && pTexture_.hasObject())
	{
		if (alpha_)
			pAlphaMaterial_->set();
		else
			pColourMaterial_->set();
		Moo::rc().setTexture( 0, pTexture_->pTexture() );
		Moo::rc().device()->SetPalette( 0, pTexture_->pPalette() );
		Vector2 bottomRight( Moo::rc().screenWidth() * 0.95f, Moo::rc().screenHeight() * 0.95f );
        Vector2 topLeft( bottomRight.x - width_, bottomRight.y - height_ );

		if (fitToScreen_ && preserveAspect_)
		{
			float viewX = Moo::rc().screenWidth() * 0.9f;
			float viewY = Moo::rc().screenHeight() * 0.9f;
			float aspectX = viewX / float(width_);
			float aspectY = viewY / float(height_);

			float aspect = min( aspectX, aspectY );
			topLeft.set( bottomRight.x - float(width_) * aspect, bottomRight.y - float(height_) * aspect );
		}
		else if (fitToScreen_)
		{
			topLeft.set( Moo::rc().screenWidth() * 0.05f, Moo::rc().screenHeight() * 0.05f );
		}

		Moo::VertexTUV verts[4];
		for (uint32 i = 0; i < 4; i++)
		{
			verts[i].pos_.z = 0;
			verts[i].pos_.w = 1;
			verts[i].pos_.x = (i & 1) ? bottomRight.x : topLeft.x;
			verts[i].pos_.y = (i & 2) ? bottomRight.y : topLeft.y;
			verts[i].uv_.x = (i & 1) ? 1.f : 0.f;
			verts[i].uv_.y = (i & 2) ? 1.f : 0.f;
		}

		Moo::rc().setVertexShader( Moo::VertexTUV::fvf() );
		Moo::rc().drawVerticesUP(D3DPT_TRIANGLESTRIP, 4, verts, sizeof(Moo::VertexTUV) );
	}
}

BW_END_NAMESPACE

// texture_explorer.cpp
