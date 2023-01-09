#ifndef IMAGE_TEXTURE_HPP
#define IMAGE_TEXTURE_HPP

#include "moo/base_texture.hpp"
#include "moo/image.hpp"
#include "moo/moo_math.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_lock_wrapper.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	template<typename PIXELTYPE>
	class ImageTexture : public BaseTexture
	{
	public:
		typedef PIXELTYPE			PixelType;
		typedef Image<PixelType>	ImageType;

		ImageTexture
		(
			uint32		width, 
			uint32		height, 			
			D3DFORMAT	format		= recommendedFormat(),
			DWORD		usage		= 0,
			D3DPOOL		pool		= D3DPOOL_MANAGED,
			uint32		mipLevels	= 0
		);
		~ImageTexture();

		DX::BaseTexture *pTexture();

		uint32 width() const;
		uint32 height() const;
		uint32 levels() const;
		D3DFORMAT format() const;
		DWORD usage() const;
		D3DPOOL pool() const;
		uint32 textureMemoryUsed() const;

		void lock(uint32 level = 0, uint32 lockFlags =0);
		ImageType &image();
		ImageType const &image() const;
		void unlock();

		static D3DFORMAT recommendedFormat();

	private:
		uint32						width_;
		uint32						height_;
		uint32						levels_;
		D3DFORMAT					format_;
		DWORD						usage_;
		D3DPOOL						pool_;
		uint32						textureMemoryUsed_;
		ImageType					image_;
		ComObjectWrap<DX::Texture>	texture_;
		uint32						lockCount_;
		uint32						lockLevel_;
		TextureLockWrapper			textureLockWrapper_;
	};

	typedef ImageTexture<Moo::PackedColour>		ImageTextureARGB;
	typedef ImageTexture<uint8>					ImageTexture8;
	typedef ImageTexture<uint16>				ImageTexture16;

	typedef SmartPointer<ImageTextureARGB>		ImageTextureARGBPtr;
	typedef SmartPointer<ImageTexture8>			ImageTexture8Ptr;
	typedef SmartPointer<ImageTexture16>		ImageTexture16Ptr;
}


#include "moo/image_texture.ipp"

BW_END_NAMESPACE

#endif // IMAGE_TEXTURE_HPP
