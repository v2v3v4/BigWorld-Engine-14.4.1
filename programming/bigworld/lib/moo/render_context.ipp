
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

extern uint32 g_setRSCallCount;
extern uint32 g_setRSChangeCount;
extern uint32 g_setTSSCallCount;
extern uint32 g_setTSSChangeCount;
extern uint32 g_setTextureCallCount;
extern uint32 g_setTextureChangeCount;

namespace Moo
{

/**
 *	This method returns the d3d device created by the render context.
 */
INLINE
DX::Device* RenderContext::device() const
{
	return device_;
}

/**
 *	This method returns the d3d Ex device created by the render context.
 */
INLINE
DX::DeviceEx* RenderContext::deviceEx() const
{
	return deviceEx_;
}

/**
 * This method returns whether the display is running in windowed mode or not.
 */
INLINE
bool RenderContext::windowed( ) const
{
	return windowed_;
}


/**
 * This method returns whether a stencil buffer is available
 */
INLINE
bool RenderContext::stencilAvailable( ) const
{
	return stencilAvailable_;
}

/**
 * This method returns whether a 32 bits index buffer is available
 */
INLINE
DWORD RenderContext::maxVertexIndex( ) const
{
	return devices_[ 0 ].caps_.MaxVertexIndex;
}

/**
 * This method returns the correct adapter format currently in use.
 */
INLINE
D3DFORMAT RenderContext::adapterFormat( ) const
{
	return windowed_ ? devices_[ deviceIndex_ ].windowedDisplayMode_.Format :
						devices_[ deviceIndex_ ].displayModes_[ modeIndex_ ].Format;
}

/**
 *	This method returns the index of the current device,
 */
INLINE
uint32 RenderContext::deviceIndex( ) const
{
	return deviceIndex_;
}


/**
 *	This method returns the index of the current displaymode.
 */
INLINE
uint32 RenderContext::modeIndex( ) const
{
	return modeIndex_;
}

/**
 *	This method returns a reference to the current camera.
 */
INLINE
const Camera& RenderContext::camera( ) const
{
	return camera_;
}

/**
 *	@see camera
 */
INLINE
Camera& RenderContext::camera( )
{
	return camera_;
}

/**
 *	@see camera
 */
INLINE
void RenderContext::camera( const Camera& cam )
{
	camera_ = cam;
}


/**
 *	This method returns the current projection matrix.
 */
INLINE
const Matrix& RenderContext::projection( ) const
{
	return projection_;
}

/**
 *	@see projection
 */
INLINE
Matrix& RenderContext::projection( )
{
	return projection_;
}

/**
 *	@see projection
 */
INLINE
void RenderContext::projection( const Matrix& m )
{
	projection_ = m;
	viewProjection_.multiply( view_, projection_ );
}

/**
 *	This method returns the current view matrix.
 */
INLINE
const Matrix& RenderContext::view( ) const
{
	return view_;
}

/**
 *	@see view
 */
INLINE
Matrix& RenderContext::view( )
{
	return view_;
}

/**
 *	@see view
 */
INLINE
void RenderContext::view( const Matrix& m )
{
	view_ = m;
	viewProjection_.multiply( view_, projection_ );
}

/**
 *	This method returns the current lod view matrix.
 */
INLINE
const Matrix& RenderContext::lodView( ) const
{
	return lodView_;
}

/**
 *	This method returns the current invert lod view matrix.
 */
INLINE
const Matrix& RenderContext::lodInvView() const
{
	return lodInvView_;
}

/**
 *	This method updated current lod camera params.
 */
INLINE
void RenderContext::updateLodViewMatrix()
{
	lodView_ = view_;
	lodInvView_ = invView_;
	lodZoomFactor_ = zoomFactor_;
}

/**
 *	This method returns the current world matrix.
 */
INLINE
const Matrix& RenderContext::world() const
{
	return world_.back();
}

/**
 *	@see world.
 */
INLINE
Matrix& RenderContext::world()
{
	return world_.back();
}

/**
 *	@see world.
 */
INLINE
void RenderContext::world( const Matrix& m )
{
	world_.back() = m;
}

/**
 *	This method multiplies the input by the world matrix ( m * world )
 */
INLINE
void RenderContext::preMultiply( const Matrix& m )
{
	world_.back().preMultiply( m );
}

/**
 *	This method multiplies the world matrix by the input ( world * m )
 */
INLINE
void RenderContext::postMultiply( const Matrix& m )
{
	world_.back().postMultiply( m );
}


/**
 *	Push the world matrix.
 */
INLINE
void RenderContext::push( )
{
	MF_ASSERT_DEV( world_.size() > 0 );
	world_.push_back( world_.back() );
}

/**
 *	Pop the world matrix.
 */
INLINE
void RenderContext::pop( )
{
	IF_NOT_MF_ASSERT_DEV( world_.size() > 1 )
	{
		return;
	}
	world_.pop_back();
}

/**
 *	This method returns the view * projection matrix.
 */
INLINE
const Matrix& RenderContext::viewProjection( ) const
{
	return viewProjection_;
}


/**
 *	This method returns the view * projection matrix.
 */
INLINE
const Matrix& RenderContext::lastViewProjection( ) const
{
	return lastViewProjection_;
}

INLINE
void RenderContext::lastViewProjection( const Matrix& m  )
{
	lastViewProjection_ = m;
}

/**
 *	This method returns the inverse view matrix.
 */
INLINE
const Matrix& RenderContext::invView( ) const
{
	return invView_;
}

/**
 *	This method returns the number of enumerated devices.
 */
INLINE
uint32 RenderContext::nDevices( ) const
{
	return static_cast<uint32>( devices_.size() );
}

/**
 *	This method returns the device info of a device.
 *	@param i the index of the device.
 */
INLINE
const Moo::DeviceInfo& RenderContext::deviceInfo( uint32 i ) const
{
	IF_NOT_MF_ASSERT_DEV( devices_.size() > i )
	{
		MF_EXIT( "index out of range of devices" );
	}

	return devices_[ i ];
}


/**
 *	This method returns the device info of the current device. May be called only after device creation.
 */
INLINE
const Moo::DeviceInfo& RenderContext::currentDeviceInfo( ) const
{
	MF_ASSERT(device_);
	return devices_[deviceIndex_];
}


/**
 *	This method resets the world matrix to identity and the world matrix stack.
 */
INLINE
void RenderContext::reset( )
{
	// Specifically not using clear to avoid dealloc
	// (verified with VC2012 implementation)
	while (!world_.empty())
	{
		world_.pop_back();
	}
	world_.push_back( Matrix::identity );
}

/**
 *	This method resets the world matrix to the supplied matrix and the world matrix stack.
 *	@param m the matrix to reset the world matrix to.
 */
INLINE
void RenderContext::reset( const Matrix& m )
{
	// Specifically not using clear to avoid dealloc
	// (verified with VC2012 implementation)
	while (!world_.empty())
	{
		world_.pop_back();
	}
	world_.push_back( m );
}

/**
 *	This method returns the description of the back buffer.
 */
INLINE
const D3DSURFACE_DESC& RenderContext::backBufferDesc( ) const
{
	return backBufferDesc_;
}

/**
 *	This method returns the current presentation paramaters.
 */
INLINE
const D3DPRESENT_PARAMETERS& RenderContext::presentParameters( ) const
{
	return presentParameters_;
}

/**
 *	This method returns the current frametimestamp,
 *	the frametimestamp is increased every time nextframe is called.
 */
INLINE
uint32 RenderContext::frameTimestamp( ) const
{
	return currentFrame_;
}

/**
 *	This method let's you query whether a frame has been drawn or not.
 *	@param frame gets updated if it's not equal to the current frametimestamp.
 *	@return true if frame is equal to the frame timestamp.
 */
INLINE
bool RenderContext::frameDrawn( uint32& frame ) const
{
	bool res = frame == currentFrame_;
	frame = currentFrame_;

	return res;
}

/**
 *	This method returns the current light container.
 */
INLINE
const LightContainerPtr & RenderContext::lightContainer( ) const
{
	return lightContainer_;
}

/**
 *	@see lightContainer.
 */
INLINE
void RenderContext::lightContainer( const LightContainerPtr & pLC )
{
	lightContainer_ = pLC;
}

/**
 *	This method returns the current light container of specular lights.
 */
INLINE
const LightContainerPtr & RenderContext::specularLightContainer( ) const
{
	return specularLightContainer_;
}

/**
 *	@see specularLightContainer.
 */
INLINE
void RenderContext::specularLightContainer( const LightContainerPtr & pLC )
{
	specularLightContainer_ = pLC;
}

/**
 *	This method returns the object alpha override, if this value is set,
 *	the objects that get rendered should be rendered with an opacity.
 */
INLINE
uint32 RenderContext::objectAlphaOverride() const
{
	return alphaOverride_;
}

/**
 *	@see objectAlphaOverride
 */
INLINE
void RenderContext::objectAlphaOverride( float alpha )
{
	if ( alpha < 0.999f )
	{
		alphaOverride_ = ( (uint32)( alpha * 255.f ) ) << 24;
	}
	else
	{
		alphaOverride_ = 0xff000000;
	}
}

/**
 *	TODO
 */
INLINE
bool RenderContext::depthOnly() const
{
	return depthOnly_;
}

/**
 *	@see depthOnly.
 */
INLINE
void RenderContext::depthOnly( bool depthOnly )
{
	depthOnly_ = depthOnly;
}

/**
 *	TODO
 */
INLINE
float RenderContext::lodValue() const
{
	return lodValue_;
}

/**
 *	@see lodValue.
 */
INLINE
void RenderContext::lodValue( float value)
{
	lodValue_ = max( 0.f, min( 1.f, value ) );
}

/**
 * TODO
 */
INLINE
float RenderContext::lodFar() const
{
	return lodFar_;
}

/**
 * TODO
 */
INLINE
void RenderContext::lodFar( float value )
{
	lodFar_ = max( value, 0.f );
}

/**
 * TODO
 */
INLINE
float RenderContext::lodPower() const
{
	return lodPower_;
}

/**
 * TODO
 */
INLINE
void RenderContext::lodPower( float value )
{
	lodPower_ = max( value, 0.f );
}

/**
 *	TODO
 */
INLINE
float RenderContext::zoomFactor() const
{
	return zoomFactor_;
}

INLINE 
float RenderContext::lodZoomFactor() const
{
	return lodZoomFactor_;
}

/**
 * TODO
 */
INLINE
int RenderContext::RenderTargetStack::nStackItems()
{
	return static_cast<int>( stackItems_.size() );
}

/**
 * This method pushes the current renderTarget.
 */
INLINE
bool RenderContext::pushRenderTarget()
{
	return renderTargetStack_.push( this );
}

/**
 * This method pops the current renderTarget.
 */
INLINE
bool RenderContext::popRenderTarget()
{
	return renderTargetStack_.pop( this );
}

/**
 * This method sets the current render target.
 */
INLINE
HRESULT	RenderContext::setRenderTarget(uint32 index, ComObjectWrap<DX::Surface> surface)
{
	MF_ASSERT_DEBUG( index < MAX_CONCURRENT_RTS); //currently max of MAX_CONCURRENT_RTS MRT's
	
	if ( index==0 || mrtSupported_ )
	{
		renderTarget_[index] = surface;
		renderTargetCount_++;
		return device_->SetRenderTarget(index, surface.pComObject());
	}
	return D3DERR_NOTAVAILABLE;	
}

/**
 * This method retrieves the current render target.
 */
INLINE
ComObjectWrap<DX::Surface> RenderContext::getRenderTarget(uint32 index)
{
	MF_ASSERT_DEBUG( index < MAX_CONCURRENT_RTS ); //currently max of MAX_CONCURRENT_RTS MRT's

	// render target 0 should never be NULL
	if (index == 0 && !renderTarget_[index].hasComObject())
	{
		if (FAILED(device_->GetRenderTarget(0, &renderTarget_[index])))
			return NULL;
	}
	return renderTarget_[index];
}

/**
 * This method retrieves the 2nd render target texture, if in use.
 */
INLINE
ComObjectWrap<DX::Texture> RenderContext::secondRenderTargetTexture()
{
	return secondRenderTargetTexture_;
}

/**
 *	This method returns the render target count.
 */
INLINE
uint32 RenderContext::renderTargetCount() const
{
	return renderTargetCount_;
}

/**
 *	This methode returns the screen width in pixels.
 */
INLINE
float RenderContext::screenWidth() const
{
	return halfScreenWidth_ + halfScreenWidth_;
}

/**
 *	This methode returns the screen height in pixels.
 */
INLINE
float RenderContext::screenHeight() const
{
	return halfScreenHeight_ + halfScreenHeight_;
}


/**
 * This method returns the half screen width in pixels.
 */
INLINE
float RenderContext::halfScreenWidth() const
{
	return halfScreenWidth_;
}

/**
 *	This method returns the half screen height in pixels.
 */
INLINE
float RenderContext::halfScreenHeight() const
{
	return halfScreenHeight_;
}

/**
 *	This method returns the full screen aspect ratio.
 */
INLINE
float RenderContext::fullScreenAspectRatio() const
{
	return fullScreenAspectRatio_;
}

/**
 *	This method sets the full screen aspect ratio.
 */
INLINE
void RenderContext::fullScreenAspectRatio( float ratio )
{
	fullScreenAspectRatio_ = ratio;
}

/**
 *	This method sets the gamma correction value.
 */
INLINE
void RenderContext::gammaCorrection( float gammaCorrection )
{
	gammaCorrection_ = gammaCorrection;
	setGammaCorrection();
}

/**
 *	This method gets the gamma correction value.
 */
INLINE
float RenderContext::gammaCorrection( ) const
{
	return gammaCorrection_;
}


/**
 *	TODO
 */
INLINE
bool RenderContext::waitForVBL() const
{
	return waitForVBL_;
}

/**
 *	TODO
 */
INLINE
void RenderContext::waitForVBL( bool wait )
{
	waitForVBL_ = wait;
}

/**
 *	Returns maximum vertex shader version supported by host system.
 */
INLINE uint16 RenderContext::vsVersion() const
{
	MF_ASSERT( vsVersion_ != SHADER_VERSION_NOT_INITIALISED );
	return vsVersion_;
}

/**
 *	Returns maximum pixel shader version supported by host system.
 */
INLINE uint16 RenderContext::psVersion() const
{
	MF_ASSERT( psVersion_ != SHADER_VERSION_NOT_INITIALISED );
	return psVersion_;
}

INLINE bool	RenderContext::isHWInstancingAvailable() const
{
	if (psVersion_ == RenderContext::SHADER_VERSION_NOT_INITIALISED)
	{
		return false;
	}
	return ((psVersion_ & 0xff00) >> 8) >= 3;
}
/**
 *	Returns maximum simultaneous texture stages supported by host system.
 */
INLINE uint16 RenderContext::maxSimTextures() const
{
	return maxSimTextures_;
}

/**
 *	Returns maximum anisotropy value supported by host system.
 */
INLINE uint16 RenderContext::maxAnisotropy() const
{
	return maxAnisotropy_;
}

INLINE HRESULT RenderContext::setViewport( DX::Viewport* viewport )
{
	DX::Viewport newvp = *viewport;
	newvp.MaxZ = min( newvp.MaxZ, 1.0f );
	maxZ_ = viewport->MaxZ;

	HRESULT hr = device_->SetViewport( &newvp );
	if( FAILED( hr ) )
	{
		WARNING_MSG( "Moo::RenderContext::setViewport: couldn't set viewport %lx\n", hr );
	}
	return hr;
}

INLINE HRESULT RenderContext::getViewport( DX::Viewport* viewport ) const
{
	HRESULT hr = device_->GetViewport( viewport );
	if( FAILED( hr ) )
	{
		WARNING_MSG( "Moo::RenderContext::getViewport: couldn't get viewport %lx\n", hr );
	}

	viewport->MaxZ = maxZ_;

	return hr;
}

INLINE uint32 RenderContext::setRenderState(D3DRENDERSTATETYPE state, uint32 value)
{
	BW_GUARD;

	MF_ASSERT_DEBUG( state < ARRAY_SIZE(rsCache_) );

	if ( cacheValidityId_ != rsCache_[state].Id || value != rsCache_[state].currentValue )
	{
		rsCache_[state].Id = cacheValidityId_;
		rsCache_[state].currentValue = value;
		return device_->SetRenderState( state, value );
	}

	return D3D_OK;
}

INLINE uint32 RenderContext::setTextureStageState( uint32 stage, D3DTEXTURESTAGESTATETYPE type, uint32 value)
{
	MF_ASSERT_DEBUG( stage < ARRAY_SIZE(tssCache_) );
	MF_ASSERT_DEBUG( type < ARRAY_SIZE(tssCache_[stage]) );

    if( cacheValidityId_ != tssCache_[stage][type].Id || value != tssCache_[stage][type].currentValue )
    {
		tssCache_[stage][type].Id = cacheValidityId_;
		tssCache_[stage][type].currentValue = value;
		return device_->SetTextureStageState( stage, type, value );
	}
    return D3D_OK;
}

INLINE uint32 RenderContext::setSamplerState( uint32 stage, D3DSAMPLERSTATETYPE type, uint32 value)
{
	MF_ASSERT_DEBUG( stage < ARRAY_SIZE(sampCache_) );
	MF_ASSERT_DEBUG( type < ARRAY_SIZE(sampCache_[stage]) );

    if( cacheValidityId_ != sampCache_[stage][type].Id ||  value != sampCache_[stage][type].currentValue )
    {
		sampCache_[stage][type].Id = cacheValidityId_;
		sampCache_[stage][type].currentValue = value;
		return device_->SetSamplerState( stage, type, value );
	}
    return D3D_OK;
}

INLINE uint32 RenderContext::setVertexShader( DX::VertexShader* pVS )
{
	return device_->SetVertexShader( pVS );
}

INLINE uint32 RenderContext::setPixelShader( DX::PixelShader* pPS )
{
	return device_->SetPixelShader( pPS );
}

INLINE HRESULT RenderContext::setIndices( DX::IndexBuffer* pIB )
{
	return device_->SetIndices( pIB );
}

INLINE ComObjectWrap<DX::IndexBuffer> RenderContext::getIndices() const
{
	ComObjectWrap<DX::IndexBuffer> pIB;
	device_->GetIndices( &pIB );
	return pIB;
}

INLINE uint32 RenderContext::setVertexDeclaration( DX::VertexDeclaration* pVD )
{
	if ( cacheValidityId_ != vertexDeclarationId_ || pVD != vertexDeclaration_ )
	{
		vertexDeclarationId_ = cacheValidityId_;
		vertexDeclaration_ = pVD;
		fvf_ = 0;
		return device_->SetVertexDeclaration( pVD );
	}
    return D3D_OK;
}

INLINE uint32 RenderContext::setFVF( uint32 fvf )
{
	if ( cacheValidityId_ != vertexDeclarationId_ || fvf != fvf_ )
	{
		vertexDeclarationId_ = cacheValidityId_;
		fvf_ = fvf;
		vertexDeclaration_ = NULL;
		return device_->SetFVF( fvf_ );
	}

	return D3D_OK;
}

INLINE uint32 RenderContext::setTexture(DWORD stage, DX::BaseTexture* pTexture)
{
	MF_ASSERT_DEBUG( stage < ARRAY_SIZE(textureCache_) );

    if( cacheValidityId_ != textureCache_[stage].Id || pTexture != textureCache_[stage].pCurrentTexture )
    {
		textureCache_[stage].Id = cacheValidityId_;
        textureCache_[stage].pCurrentTexture = pTexture;
		return device_->SetTexture( stage, pTexture );
	}
	return D3D_OK;
}

INLINE uint32 RenderContext::setWriteMask( uint i, uint32 value )
{
	static uint targetMap[] = { D3DRS_COLORWRITEENABLE, D3DRS_COLORWRITEENABLE1, D3DRS_COLORWRITEENABLE2, D3DRS_COLORWRITEENABLE3 };
	MF_ASSERT_DEBUG( i < ARRAY_SIZE(targetMap)  );
	if ( i==0 || mrtSupported_ )
	{
		return setRenderState( (D3DRENDERSTATETYPE)targetMap[i], value );
	}
	return D3DERR_NOTAVAILABLE;
}

INLINE uint32 RenderContext::drawIndexedPrimitiveUP( D3DPRIMITIVETYPE primitiveType,
													 UINT minIndex,
													 UINT numVertices,
													 UINT primitiveCount,
													 CONST void* pIndexData,
													 D3DFORMAT indexDataFormat,
													 CONST void* pVertexStreamZeroData,
													 UINT vertexStreamZeroStride )
{
	++liveProfilingData_.nDrawcalls_;
	liveProfilingData_.nPrimitives_ += primitiveCount;
	return device_->DrawIndexedPrimitiveUP( primitiveType, minIndex,numVertices,
											primitiveCount, pIndexData, indexDataFormat,
											pVertexStreamZeroData, vertexStreamZeroStride );
}


INLINE uint32 RenderContext::drawIndexedPrimitive( D3DPRIMITIVETYPE type,
														  INT  baseVertexIndex,
														  UINT minIndex,
														  UINT numVertices,
														  UINT startIndex,
														  UINT primitiveCount )
{
	++liveProfilingData_.nDrawcalls_;
	liveProfilingData_.nPrimitives_ += primitiveCount;
	return device_->DrawIndexedPrimitive( type, baseVertexIndex, minIndex, numVertices,
										  startIndex, primitiveCount );
}

INLINE uint32 RenderContext::drawIndexedInstancedPrimitive( D3DPRIMITIVETYPE type,
														  	INT baseVertexIndex,
														  	UINT minIndex,
														  	UINT numVertices,
														  	UINT startIndex,
														  	UINT primitiveCount,
														  	UINT instanceCount )
{
	++liveProfilingData_.nDrawcalls_;
	liveProfilingData_.nPrimitives_ += primitiveCount * instanceCount;
	return device_->DrawIndexedPrimitive( type, baseVertexIndex, minIndex, numVertices,
		startIndex, primitiveCount );
}

INLINE uint32 RenderContext::drawPrimitive( D3DPRIMITIVETYPE primitiveType,
												   UINT startVertex,
												   UINT primitiveCount )
{
	++liveProfilingData_.nDrawcalls_;
	liveProfilingData_.nPrimitives_ += primitiveCount;
	return device_->DrawPrimitive( primitiveType, startVertex, primitiveCount );
}


INLINE uint32 RenderContext::drawPrimitiveUP( D3DPRIMITIVETYPE primitiveType,
													 UINT primitiveCount,
													 CONST void* pVertexStreamZeroData,
													 UINT vertexStreamZeroStride )
{
	++liveProfilingData_.nDrawcalls_;
	liveProfilingData_.nPrimitives_ += primitiveCount;
	return device_->DrawPrimitiveUP( primitiveType, primitiveCount,
								 pVertexStreamZeroData, vertexStreamZeroStride );
}

INLINE SimpleMutex&	RenderContext::getD3DXCreateMutex()
{
	return d3dxCreateMutex_;
}

INLINE HWND	RenderContext::windowHandle()
{
	return windowHandle_;
}


/**
 *	This method returns the dynamic index buffer interface, which can be later
 *	used to obtain the appropriate dynamic index buffer.
 *
 *	@return		A DynamicIndexBufferInterface reference.
 */
INLINE DynamicIndexBufferInterface & RenderContext::dynamicIndexBufferInterface() const
{
	// Should never be called before it's initialised
	IF_NOT_MF_ASSERT_DEV( pDynamicIndexBufferInterface_ != NULL )
	{
		MF_EXIT( "trying to get interface when not initialised" );
	}

	return *pDynamicIndexBufferInterface_;
}

/**
 *	This static method converts D3DPOOL_MANAGED to D3DPOOL_DEFAULT
 *  if we are using IDirect3DDevice9Ex.
 *  Ex Device only available on Windows Vista +
*/
INLINE
D3DPOOL		RenderContext::patchD3DPool(D3DPOOL pool)
{
	if (Moo::rc().usingD3DDeviceEx() && pool == D3DPOOL_MANAGED)
	{
		pool = D3DPOOL_DEFAULT;
	}
	return pool;
}

INLINE
bool RenderContext::usingD3DDeviceEx() const
{
	return usingD3DDeviceEx_;
}

INLINE
void RenderContext::forceNoExDevice()
{ 
	forceDisableExDevice_ = true;
}

} // namespace Moo

/*render_context.ipp*/
