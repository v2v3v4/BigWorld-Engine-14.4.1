#ifndef CUBE_RENDER_TARGET_HPP
#define CUBE_RENDER_TARGET_HPP

#include <iostream>
#include "base_texture.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	/**
	 *	This class implements functionality to handle a cubic 
	 *	environment render target.
	 */
	class CubeRenderTarget : public BaseTexture, public DeviceCallback
	{
	public:
		CubeRenderTarget( const BW::string& identifier );

		bool					create( uint32 cubeDimensions, 
										const Colour & clearColour = (DWORD)0x00000000 );
		void					release();

		bool					pushRenderSurface( D3DCUBEMAP_FACES face );
		void					setCubeViewMatrix( D3DCUBEMAP_FACES face, 
													const Vector3& centre );
		void					pop( void );

		void					setupProj();
		void					restoreProj();

		// Virtual methods from BaseTexture
		virtual DX::BaseTexture*pTexture( );
		virtual uint32			width( ) const;
		virtual uint32			height( ) const;
		virtual D3DFORMAT		format( ) const;
		virtual uint32			textureMemoryUsed( ) const;
		virtual const BW::string& resourceID( ) const;
		virtual bool	isCubeMap() { return true; }

		//methods inherited from DeviceCallback
		virtual void			deleteUnmanagedObjects( );
		virtual void			createUnmanagedObjects( );

	private:

		virtual void destroy() const;
		~CubeRenderTarget();

		BW::string				identifier_;
		uint32					cubeDimensions_;
		D3DFORMAT				pixelFormat_;
		Matrix					originalProj_;
		Camera					originalCamera_;
		Colour					clearColour_;

		ComObjectWrap< DX::CubeTexture > pRenderTarget_;
		ComObjectWrap< DX::Surface > pDepthStencilTarget_;

		CubeRenderTarget(const CubeRenderTarget&);
		CubeRenderTarget& operator=(const CubeRenderTarget&);

		friend std::ostream& operator<<(std::ostream&, const CubeRenderTarget&);
	};

	typedef SmartPointer< class CubeRenderTarget > CubeRenderTargetPtr;

}

#ifdef CODE_INLINE
#include "cube_render_target.ipp"
#endif

BW_END_NAMESPACE

#endif // CUBE_RENDER_TARGET_HPP
/*cube_render_target.hpp*/
