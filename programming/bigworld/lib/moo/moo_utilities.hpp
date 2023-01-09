#pragma once

#include "forward_declarations.hpp"

BW_BEGIN_NAMESPACE

//-- File provides some frequently needed Moo's functions in a simplified form and with an additional
//-- error checking.

namespace Moo
{
	//----------------------------------------------------------------------------------------------
	bool createEffect(
		Moo::EffectMaterialPtr& mat, const BW::string& name
		);

	//----------------------------------------------------------------------------------------------
	bool createRenderTarget(
		Moo::RenderTargetPtr& rt, uint width, uint height, D3DFORMAT pixelFormat,
		const BW::string& name, bool useMainZBuffer = false, bool discardZBuffer = true
		);

	//-- Copy content of the current back buffer into the desired render target surface.
	//-- Note: This function is asynchronous, so don't worry much about performance.
	//----------------------------------------------------------------------------------------------
	bool copyBackBuffer(RenderTarget* outRT);
	
	//-- Copy content of the current render target, binded to slot rtSlot, into the desired render
	//-- target surface.
	//-- Note: This function is asynchronous, so don't worry much about performance.
	//----------------------------------------------------------------------------------------------
	bool copyRT(RenderTarget* outRT, uint rtSlot);
	
	// Copy the contents of the source rectangle to the destination rectangle. The source rectangle 
	// can be stretched and filtered by the copy. This function is often used to change the aspect 
	// ratio of a video stream.
	//----------------------------------------------------------------------------------------------
	bool copyRT( RenderTarget* destRT, const RECT* destRect, 
		RenderTarget* srcRT, const RECT* srcRect, D3DTEXTUREFILTERTYPE filter );

	//-- Copy content of the iSurface into the desired render target surface.
	//-- Note: This function is asynchronous, so don't worry much about performance.
	//----------------------------------------------------------------------------------------------
	bool copySurface(RenderTarget* outRT, const ComObjectWrap<DX::Surface>& surface);

    //-- Draw screen-space textured rect.
    //----------------------------------------------------------------------------------------------
    void drawDebugTexturedRect(const Vector2& topLeft, const Vector2& bottomRight, 
        DX::BaseTexture* texture, Moo::Colour color = Moo::Colour(0,0,0,0));

#if ENABLE_RESOURCE_COUNTERS
	BW::string	getAllocator(const char* module, const BW::string& resourceID);
#endif

} //-- Moo

BW_END_NAMESPACE
