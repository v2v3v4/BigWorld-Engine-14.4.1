
#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "render_context.hpp"

#include "cube_render_target.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "cube_render_target.ipp"
#endif

namespace Moo
{

	CubeRenderTarget::CubeRenderTarget( const BW::string& identifier ) :
		identifier_( identifier ),
		cubeDimensions_( 0 ),
		originalCamera_( rc().camera() )
	{
		this->BaseTexture::addToManager();
	}

	CubeRenderTarget::~CubeRenderTarget()
	{
		BW_GUARD;
		this->release();
	}


	void CubeRenderTarget::destroy() const
	{
		// attempt to safety delete the texture
		this->tryDestroy();
	}

	/**
	 *	This method creates a cubemap and a z/stencil buffer, of the supplied dimensions.
	 *	@param cubeDimensions the size of each side of the cubemap.
	 *	@return true if the method succeeds.
	 */
	bool CubeRenderTarget::create( uint32 cubeDimensions,
								   const Colour & clearColour )
	{
		BW_GUARD;
		cubeDimensions_ = cubeDimensions;
		clearColour_ = clearColour;

		createUnmanagedObjects();

		return ( pRenderTarget_.hasComObject() );
	}

	/**
	 *	This method sets a render surface, it does NOT store the previous render target etc,
	 *	so it is important to call RenderContext::pushRendertarget if you want to restore
	 *	the previous render target.
	 *	@param face the identifier of the cubemap face to set as a render target.
	 *	@return true if the operation was successful
	 */
	bool CubeRenderTarget::pushRenderSurface( D3DCUBEMAP_FACES face )
	{
		BW_GUARD;
		if (!rc().pushRenderTarget())
			return false;

		IF_NOT_MF_ASSERT_DEV( pRenderTarget_.hasComObject() )
		{
			return false;
		}

		ComObjectWrap<DX::Surface> pCubeSurface;
		HRESULT hr = pRenderTarget_->GetCubeMapSurface( face, 0, &pCubeSurface );

		if (!SUCCEEDED( hr ))
		{
			WARNING_MSG( "Couldn't get cube map surface: %s\n",
				DX::errorAsString(hr).c_str() );
			rc().popRenderTarget();
			return false;
		}

		DX::Device* device = rc().device();
		hr = rc().setRenderTarget( 0, pCubeSurface );

		if (!SUCCEEDED( hr ))
		{
			WARNING_MSG( "Unable to set rendertarget: %s\n",
				DX::errorAsString(hr).c_str() );
			rc().popRenderTarget();
			return false;
		}

		hr = device->SetDepthStencilSurface( &*pDepthStencilTarget_ );
		if ( FAILED( hr ) )
		{
			WARNING_MSG( "Unable to set depth target on device: %s\n",
				DX::errorAsString(hr).c_str() );
			rc().popRenderTarget();
			return false;
		}

		rc().setRenderTarget( 1, NULL );

		Moo::rc().screenWidth( cubeDimensions_ );
		Moo::rc().screenHeight( cubeDimensions_ );

		return true;
	}

	/**
	 *	This method pops the RenderTarget, and restores the
	 *	camera, projection matrix and viewport
	 */
	void CubeRenderTarget::pop()
	{
		rc().popRenderTarget();
	}


	/**
	 *
	 */
	void CubeRenderTarget::setupProj()
	{
		BW_GUARD;
		originalProj_ = rc().projection();
		originalCamera_ = rc().camera();

		rc().camera( Camera(originalCamera_.nearPlane(), 
							originalCamera_.farPlane(), 
							MATH_PI / 2.0f,  // 90deg
							1.0f ) );
		rc().updateProjectionMatrix(false);
	}

	/**
	 *
	 */
	void CubeRenderTarget::restoreProj()
	{
		BW_GUARD;
		rc().projection( originalProj_ );
		rc().camera( originalCamera_ );
		rc().updateProjectionMatrix();
	}


	/**
	 *	This method sets up the view matrix for rendering to a specified cubemap face.
	 *	@param face the cubemap face we want to render to.
	 *	@param centre the camera position we want to render from.
	 */
	void CubeRenderTarget::setCubeViewMatrix( D3DCUBEMAP_FACES face, const Vector3& centre )
	{
		BW_GUARD;
		Matrix m = Matrix::identity;

		switch( face )
		{
		case D3DCUBEMAP_FACE_POSITIVE_X:
			m.setRotateY( DEG_TO_RAD( 90 ) );
			break;
		case D3DCUBEMAP_FACE_NEGATIVE_X:
			m.setRotateY( DEG_TO_RAD( -90 ) );
			break;
		case D3DCUBEMAP_FACE_POSITIVE_Y:
			m.setRotateX( DEG_TO_RAD( -90 ) );
			break;
		case D3DCUBEMAP_FACE_NEGATIVE_Y:
			m.setRotateX( DEG_TO_RAD( 90 ) );
			break;
		case D3DCUBEMAP_FACE_NEGATIVE_Z:
			m.setRotateY( DEG_TO_RAD( 180 ) );
			break;
		case D3DCUBEMAP_FACE_POSITIVE_Z:
			// Identity transform.
		//	m.setRotateY( DEG_TO_RAD( 0 ) );
			break;
		case D3DCUBEMAP_FACE_FORCE_DWORD:
			break;
		}

		m.translation( centre );
		m.invert();
		rc().view( m );
		rc().updateViewTransforms();
	}

	/**
	 * This method releases the cubemap and the zbuffer surface.
	 */
	void CubeRenderTarget::release()
	{
		BW_GUARD;
		this->deleteUnmanagedObjects();

		pRenderTarget_ = NULL;
		pDepthStencilTarget_ = NULL;
	}


	/**
	 * This method is called by DeviceCallback when it needs this class to
	 *	delete unmanaged object
	 */
	void CubeRenderTarget::deleteUnmanagedObjects( )
	{
		BW_GUARD;
		if (pRenderTarget_.hasComObject())
		{
			pRenderTarget_ = NULL;
		}
		if (pDepthStencilTarget_.hasComObject())
		{
			pDepthStencilTarget_ = NULL;
		}
	}


	/**
	 * This method is called by DeviceCallback when it needs this class to
	 *	(re)create unmanaged object
	 */
	void CubeRenderTarget::createUnmanagedObjects( )
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV( cubeDimensions_ > 0 )
		{
			return;
		}

		DX::Device* device = rc().device();

		release();

		if (device)
		{
			DX::CubeTexture* pCubeTexture = NULL;

			HRESULT hr = device->CreateCubeTexture( cubeDimensions_,
				1, D3DUSAGE_RENDERTARGET,
				D3DFMT_A8R8G8B8, 
				D3DPOOL_DEFAULT,
				&pCubeTexture, NULL );

			if (SUCCEEDED( hr ))
			{
				pRenderTarget_ = pCubeTexture;
#if ENABLE_RESOURCE_COUNTERS
				pRenderTarget_.addAlloc( BW::StringRef( "CubeRenderTarget" ) );
#endif
				pCubeTexture->Release();

				bool stencilAvailable = false; // we don't care whether we have one
				DX::Surface* pDepthStencil = NULL;
				hr = device->CreateDepthStencilSurface(	
					cubeDimensions_, 
					cubeDimensions_, 
					rc().getMatchingZBufferFormat( D3DFMT_A8R8G8B8, 
						rc().stencilAvailable(), stencilAvailable ),
					D3DMULTISAMPLE_NONE, 0, TRUE,
					&pDepthStencil, NULL );

				if (SUCCEEDED( hr ))
				{
					pDepthStencilTarget_ = pDepthStencil;
#if ENABLE_RESOURCE_COUNTERS
					pDepthStencilTarget_.addAlloc( BW::StringRef("CubeRenderTarget") );
#endif
					pDepthStencil->Release();
				}
				else
				{
					ERROR_MSG( "CubeRenderTarget::create: error creating the zbuffer: %s\n",
						DX::errorAsString( hr ).c_str() );
					release();

					return;
				}
			}
			else
			{
				ASSET_MSG( "CubeRenderTarget::create: error creating the cubetexture: %s\n",
					DX::errorAsString( hr ).c_str() );

				return;
			}
		}

		for ( uint32 face = 0 ; face < 6 ; ++face )
		{
			if ( !this->pushRenderSurface( static_cast< D3DCUBEMAP_FACES >( face ) ) )
				continue;

			rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, clearColour_, 1, 0 );
			this->pop();
		}
	}


	std::ostream& operator<<(std::ostream& o, const CubeRenderTarget& t)
	{
		o << "CubeRenderTarget\n";
		return o;
	}

} // namespace Moo

BW_END_NAMESPACE

/*cube_render_target.cpp*/
