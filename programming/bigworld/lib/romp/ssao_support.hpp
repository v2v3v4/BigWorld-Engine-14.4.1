#pragma once

#include "moo/forward_declarations.hpp"
#include "romp/ssao_settings.hpp"

BW_BEGIN_NAMESPACE

//-- Provides support of SSAO (Screen Space Ambient Occlusion).
//----------------------------------------------------------------------------------------------
class SSAOSupport : public Moo::DeviceCallback
{
private:
    //-- make non-copyable.
    SSAOSupport(const SSAOSupport&);
    SSAOSupport& operator = (const SSAOSupport&);

public:
    SSAOSupport();
    ~SSAOSupport();

    void                    settings(const SSAOSettings* settings);
    const SSAOSettings&     settings() const;

    //--
    void                    setQualityOption(int option);

    //-- enabled/disable SSAO Rendering.
    bool                    enable() const;
    void                    enable(bool flag);
    bool                    showBuffer() const;
    void                    showBuffer(bool show);

    //-- resolve SSAO render 
    void                    resolve();

    DX::BaseTexture*        screenSpaceAmbienOcclusionMap();

    //-- interface DeviceCallback.
    virtual void            deleteUnmanagedObjects();
    virtual void            createUnmanagedObjects();
    virtual void            deleteManagedObjects();
    virtual void            createManagedObjects();
    virtual bool            recreateForD3DExDevice() const;

private:
    void setSsaoBufferDownsampleFactor(int value);
    int getSsaoBufferDownsampleFactor() const;

    void setDepthBufferDownsampleFactor(int value);
    int getDepthBufferDownsampleFactor() const;

    bool                    m_enabled;
    Moo::BaseTexturePtr     m_noiseMap;
    Moo::RenderTargetPtr    m_rt;
    Moo::RenderTargetPtr    m_rtBlure;
    Moo::RenderTargetPtr    m_rtDownsampledDepth;
    Moo::EffectMaterialPtr  m_mat;
    Moo::EffectMaterialPtr  m_matBlure;
    Moo::EffectMaterialPtr  m_matDepth;
};

BW_END_NAMESPACE
