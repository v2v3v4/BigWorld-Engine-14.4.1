#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "Loader.hpp"
#include "Manager.hpp"
#include "moo/device_callback.hpp"
#include "moo/sys_mem_texture.hpp"
#include "moo/texture_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
/*#include "MovieDef.hpp"*/

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	GFile* FileOpener::OpenFile(const char *pfilename, SInt flags, SInt mode)
	{
		BW_GUARD;
		BinaryPtr ptr = BWResource::instance().rootSection()->readBinary( pfilename );
		if( !ptr.hasObject() )
			return NULL;

		return new BWGMemoryFile( pfilename, ptr );
	}

	Render::Image *BWToGFxImageLoader::LoadProtocolImage(const GFx::ImageCreateInfo& info, const GString& url)
	{
		BW_GUARD;
		UInt    width, height;
		UByte*  pdata = 0;
		
		BW::string resourceID(url);
		resourceID.replace(0, 6, "");

		bool isCamouflage = false;
		bool isSticker =  false;

		SmartPointer<Moo::SysMemTexture> pTex;
		{
			BW::string strMask;
			Vector4 vWeight;
		
			BW::string strFilePath;
			uint tileWidth, tileHeight;
			uint tileX, tileY;
			if ( ParseStickerUrl( url, width, height, strFilePath, tileWidth, tileHeight, tileX, tileY ) )
			{
				isSticker = true;
			}
		}


		// Resolve width, height, pdata based on the url.
		// ...

		if ( !pTex || pTex->status() == BW::Moo::BaseTexture::STATUS_FAILED_TO_LOAD )
		{
			pTex = new Moo::SysMemTexture(resourceID);
		}

		if (pTex->status() == BW::Moo::BaseTexture::STATUS_FAILED_TO_LOAD)
		{
			ERROR_MSG( "BWToGFxImageLoader::LoadImage: "
					"failed to load image url '%s'\n",
				static_cast< const char * >( url ) );
			return NULL;
		}

		//convert image to format supported by Scaleform if required
		if (pTex->format() != D3DFMT_A8R8G8B8
			&& pTex->format() != D3DFMT_DXT1
			&& pTex->format() != D3DFMT_DXT3
			&& pTex->format() != D3DFMT_DXT5)
		{
			D3DFORMAT defaultFmt = (isCamouflage || isSticker) ? D3DFMT_A8R8G8B8 : D3DFMT_DXT5;
			pTex = new Moo::SysMemTexture(resourceID, defaultFmt);
			
			if (pTex->status() == BW::Moo::BaseTexture::STATUS_FAILED_TO_LOAD)
			{
				ERROR_MSG( "BWToGFxImageLoader::LoadImage: "
						"failed to load image url '%s'\n",
					static_cast< const char * >( url ) );
				return NULL;
			}
		}

		width = pTex->width();
		height = pTex->height();

		Render::ImageFormat fmt = Render::Image_B8G8R8A8;
		switch (pTex->format())
		{
		case D3DFMT_A8R8G8B8: fmt = isCamouflage ? Render::Image_R8G8B8A8 : Render::Image_B8G8R8A8; break;
		case D3DFMT_DXT1: fmt = Render::Image_DXT1; break;
		case D3DFMT_DXT3: fmt = Render::Image_DXT3; break;
		case D3DFMT_DXT5: fmt = Render::Image_DXT5; break;
		default:
			{
				ERROR_MSG( "BWToGFxImageLoader::LoadImage: failed to load "
						"image url '%s', possibly incorrect image format\n",
					static_cast< const char * >( url ) );
				return NULL;
			}
		}

		Ptr<Render::RawImage> pimage = *Render::RawImage::Create(
			fmt, 1, Render::ImageSize(width, height), 0, ::Scaleform::Memory::pGlobalHeap);

		DX::Texture* tex = static_cast<DX::Texture*>(pTex->pTexture());
		if (tex)
		{
			D3DLOCKED_RECT rect;
			if (D3D_OK == tex->LockRect(0, &rect, NULL, D3DLOCK_READONLY))
			{
				Render::ImageData imgData;
				if (pimage->GetImageData(&imgData))
				{
					Render::ImagePlane imgPlane0;
					imgData.GetPlane(0, &imgPlane0);
					memcpy(imgPlane0.pData, rect.pBits, DX::surfaceSize(width, height, pTex->format()));
					pimage->AddRef();
				}
				tex->UnlockRect(0);
			}
		}

		return pimage;
	}

	bool BWToGFxImageLoader::ParseStickerUrl( const char *szUrl, uint &width, uint &height, BW::string &strFilePath,
		uint &tileWidth, uint &tileHeight, uint &tileX, uint &tileY )
	{
		const char *ptr = szUrl;
		if ( strnicmp( ptr, "img://", 6 ) != 0 )
			return false;
		ptr += 6;
		if ( strnicmp( ptr, "sticker,", 8 ) != 0 )
			return false;
		ptr += 8;
		if ( sscanf( ptr, "%d,%d,", &width, &height ) != 2 )
			return false;

		int count = 2;
		while( true )
		{
			if ( !*ptr ) return false;
			if ( *ptr++ == ',' )
			{
				if ( --count == 0 ) break;
			}
		}
		if ( *ptr++ != '\"' )
			return false;
		strFilePath.clear();
		while( true )
		{
			if ( !*ptr ) return false;
			if ( *ptr == '\"' )
			{
				ptr++;
				break;
			}
			strFilePath += *ptr++;
		}
		if ( *ptr++ != ',' ) return false;
		if ( sscanf( ptr, "%d,%d,%d,%d", &tileWidth, &tileHeight, &tileX, &tileY ) != 4 )
			return false;

		return true;
	}

	Loader::Loader():
		GFx::Loader( *new FileOpener )
	{
		BW_GUARD;
		this->SetImageCreator(new BWToGFxImageLoader());

		Ptr<GFx::ImageFileHandlerRegistry> pimgReg = *new GFx::ImageFileHandlerRegistry();
		pimgReg->AddHandler(&Render::JPEG::FileReader::Instance);
		pimgReg->AddHandler(&Render::PNG::FileReader::Instance);
		pimgReg->AddHandler(&Render::TGA::FileReader::Instance);
		pimgReg->AddHandler(&Render::DDS::FileReader::Instance);
		this->SetImageFileHandlerRegistry(pimgReg);
	}


	Loader::~Loader()
	{
		BW_GUARD;
	}

}	//namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT