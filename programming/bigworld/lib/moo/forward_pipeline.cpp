#include "pch.hpp"

#include "forward_pipeline.hpp"
#include "speedtree/speedtree_renderer.hpp"

//-- ToDo: reconsider. Try to find better way to do this.
#include "chunk/chunk.hpp"
#include "moo/line_helper.hpp"

BW_BEGIN_NAMESPACE

//--------------------------------------------------------------------------------------------------
ForwardPipeline::ForwardPipeline()
{

}

//--------------------------------------------------------------------------------------------------
ForwardPipeline::~ForwardPipeline()
{

}

//--------------------------------------------------------------------------------------------------
bool ForwardPipeline::init()
{
    BW_GUARD;

    //-- ToDo: reconsider. Try to find better way to do this.
    Chunk::unregisterFactory("staticDecal");
    Chunk::unregisterFactory("directionalLight");
    Chunk::unregisterFactory("omniLight");
    Chunk::unregisterFactory("spotLight");
    Chunk::unregisterFactory("pulseLight");
    Chunk::unregisterFactory("ambientLight");

    return true;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::tick(float /*dt*/)
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::begin()
{
    BW_GUARD;

    DX::Viewport viewport;
    viewport.Width  = (DWORD)Moo::rc().screenWidth();
    viewport.Height = (DWORD)Moo::rc().screenHeight();
    viewport.MinZ   = 0.f;
    viewport.MaxZ   = 1.f;
    viewport.X      = 0;
    viewport.Y      = 0;
    Moo::rc().setViewport(&viewport);

    //--
    uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
    if (Moo::rc().stencilAvailable())
    {
        clearFlags |= D3DCLEAR_STENCIL;
    }

    Moo::rc().device()->Clear(0, NULL, clearFlags, 0x00000000, 1, 0);
    Moo::rc().setWriteMask(0, 0xFF);

    //-- setup stencil.
    resetStencil(customStencilWriteMask());
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::end()
{
    BW_GUARD;
}

void ForwardPipeline::drawDebugStuff()
{
    BW_GUARD;

    LineHelper::instance().purge();
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::beginOpaqueDraw()
{
    BW_GUARD;

    Moo::rc().pushRenderState(D3DRS_COLORWRITEENABLE);
    Moo::rc().setWriteMask(0, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN);

    //-- prepare system stencil usage.
    setupSystemStencil(STENCIL_USAGE_OTHER_OPAQUE);
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::endOpaqueDraw()
{
    BW_GUARD;

    //-- prepare stencil usage.
    restoreSystemStencil();

    Moo::rc().popRenderState();
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::beginSemitransparentDraw()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::endSemitransparentDraw()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::beginGUIDraw()
{
    BW_GUARD;

    resetStencil();
    Moo::rc().pushRenderTarget();
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::endGUIDraw()
{
    BW_GUARD;

    Moo::rc().popRenderTarget();
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::setupColorWritingMask(bool flag) const
{
    BW_GUARD;
    Moo::rc().pushRenderState(D3DRS_COLORWRITEENABLE);
    Moo::rc().setRenderState(D3DRS_COLORWRITEENABLE, flag ? 0xFF : 0x00);
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::restoreColorWritingMask() const
{
    BW_GUARD;
    Moo::rc().popRenderState();
}

//--------------------------------------------------------------------------------------------------
bool ForwardPipeline::recreateForD3DExDevice() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::createUnmanagedObjects()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::deleteUnmanagedObjects()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::createManagedObjects()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void ForwardPipeline::deleteManagedObjects()
{
    BW_GUARD;
}

BW_END_NAMESPACE
