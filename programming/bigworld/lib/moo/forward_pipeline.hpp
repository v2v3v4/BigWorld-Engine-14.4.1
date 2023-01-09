#pragma once

#include "renderer.hpp"

BW_BEGIN_NAMESPACE

//-- ToDo: document.
//----------------------------------------------------------------------------------------------
class ForwardPipeline : public IRendererPipeline
{
public:
    ForwardPipeline();
    virtual ~ForwardPipeline();

    virtual bool                init();
    virtual void                tick(float dt);

    virtual void                begin();
    virtual void                end();

    virtual void                drawDebugStuff();

    virtual void                beginCastShadows( Moo::DrawContext& shadowDrawContext ) { }
    virtual void                endCastShadows() { }

    //-- begin/end drawing opaque objects.
    virtual void                beginOpaqueDraw();
    virtual void                endOpaqueDraw();

    //-- begin/end drawing semitransparent objects.
    virtual void                beginSemitransparentDraw();
    virtual void                endSemitransparentDraw();

    //-- begin/end gui drawing.
    virtual void                beginGUIDraw();
    virtual void                endGUIDraw();

    virtual void                onGraphicsOptionSelected(EGraphicsSetting /*setting*/, int /*option*/) { }

    //-- apply lighting to the scene.
    virtual void                applyLighting() { }

    virtual DX::Texture*        gbufferTexture(uint /*idx*/) const  { return NULL; }
    virtual DX::Surface*        gbufferSurface(uint /*idx*/) const  { return NULL; }

    virtual DX::Texture*        getGBufferTextureCopyFrom(uint /*idx*/) const  { return NULL; }

    //-- color writing usage
    virtual void                setupColorWritingMask(bool flag) const;
    virtual void                restoreColorWritingMask() const;

    //-- interface DeviceCallback.
    virtual bool                recreateForD3DExDevice() const;
    virtual void                createUnmanagedObjects();
    virtual void                deleteUnmanagedObjects();
    virtual void                createManagedObjects();
    virtual void                deleteManagedObjects();
};

BW_END_NAMESPACE
