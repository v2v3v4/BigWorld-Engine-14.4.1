#include "pch.hpp"

#include "speedtree_renderer_common.hpp"
#include "billboard_optimiser.hpp"
#include "speedtree_tree_type.hpp"
#include "moo/renderer.hpp"


BW_BEGIN_NAMESPACE

//-- define singleton.
BW_SINGLETON_STORAGE(speedtree::SpeedTreeRendererCommon)

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- instancing buffer maximum size
    const uint32 g_instanceSizeInBytes      = sizeof(speedtree::TSpeedTreeType::DrawData::PackedGPU);
    const uint32 g_maxInstBufferSizeInBytes = g_instanceSizeInBytes * 3000 * 3;

    //----------------------------------------------------------------------------------------------
    const char* const g_shaders[] =
    {
        "shaders/speedtree/branches.fx",
        "shaders/speedtree/branches.fx",
        "shaders/speedtree/leaves.fx",
        "shaders/speedtree/billboards.fx",
    };

    //----------------------------------------------------------------------------------------------
    const char* const g_specialShader = "shaders/speedtree/semitransparent.fx";
    
    //----------------------------------------------------------------------------------------------
    const char* const g_specialShaderTechniques[][4] = 
    {
        { "BRANCH_DEPTH", "FROND_DEPTH", "LEAF_DEPTH", "BILLBOARD_DEPTH" },
        { "BRANCH_ALPHA", "FROND_ALPHA", "LEAF_ALPHA", "BILLBOARD_ALPHA" }
    };

    //----------------------------------------------------------------------------------------------
    const char* const g_instancingVertDclr = "speedtree_instancing";

    //----------------------------------------------------------------------------------------------
    const char* const g_vertDclrs[] = 
    {
        "speedtree_branch",
        "speedtree_branch",
        "speedtree_leaf",
        "speedtree_billboard"
    };

    //-- Warning: names should be in sync with ConstantsMap::EConstant enumeration.
    //----------------------------------------------------------------------------------------------
    const char* const g_constNames[] =  
    {
        "g_instance",
        "alphaTestEnable",
        "g_texturedTrees",
        "g_diffuseMap",
        "g_normalMap",
        "g_useNormalMap",
        "g_material",
        "g_cullEnabled",
        "g_leafLightAdj",
        "g_leafAngleScalars",
        "g_leafRockFar",
        "g_windMatrices",
        "g_leafAngles",
        "g_useHighQuality",
        "g_useZPrePass"
    };
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.

namespace speedtree
{

    //----------------------------------------------------------------------------------------------
    SpeedTreeSettings::SpeedTreeSettings(const DataSectionPtr& cfg)
    {
        BW_GUARD;

        m_qualitySettings = Moo::makeCallbackGraphicsSetting(
            "SPEEDTREE_QUALITY", "Speedtree Quality", *this, &SpeedTreeSettings::setQualityOption, 
            -1, false, false
            );

        //-- register graphics options.
        m_qualitySettings->addOption("MAX",     "Max",      Moo::rc().psVersion() >= 0x300, true);
        m_qualitySettings->addOption("HIGH",    "High",     true);
        m_qualitySettings->addOption("MEDIUM",  "Medium",   true);
        m_qualitySettings->addOption("LOW",     "Low",      true);

        m_options.push_back(Option(true,  cfg->readFloat("lods/MAX",    1.0f), 7.0f)); //-- MAX
        m_options.push_back(Option(true,  cfg->readFloat("lods/HIGH",   0.8f), 6.0f)); //-- HIGH
        m_options.push_back(Option(false, cfg->readFloat("lods/MEDIUM", 0.6f), 5.0f)); //-- MEDIUM
        m_options.push_back(Option(false, cfg->readFloat("lods/LOW",    0.4f), 4.0f)); //-- LOW

        //-- register speedtree's settings in the global settings list.
        Moo::GraphicsSetting::add(m_qualitySettings);
    }

    //----------------------------------------------------------------------------------------------
    SpeedTreeSettings::~SpeedTreeSettings()
    {
        
    }

    //----------------------------------------------------------------------------------------------
    const SpeedTreeSettings::Option& SpeedTreeSettings::activeOption() const
    {
        BW_GUARD;

        return m_options[m_qualitySettings->activeOption()];
    }

    //----------------------------------------------------------------------------------------------
    void SpeedTreeSettings::setQualityOption(int optionIndex)
    {
        BW_GUARD;
        MF_ASSERT(optionIndex >= 0 && optionIndex < (int)m_options.size());
    }

    //----------------------------------------------------------------------------------------------
    ConstantsMap::ConstantsMap()
    {
        for (uint i = 0; i < CONST_COUNT; ++i)
        {
            m_handles[i] = NULL;
        }
    }

    //----------------------------------------------------------------------------------------------
    ConstantsMap::~ConstantsMap()
    {

    }

    //----------------------------------------------------------------------------------------------
    bool ConstantsMap::getHandles(const Moo::EffectMaterialPtr& effect)
    {
        BW_GUARD;
        MF_ASSERT(effect)

        ID3DXEffect* pEffect = effect->pEffect()->pEffect();

        //-- It is ok if some parameters returns invalid handle because we use the same shaders map
        //-- for different shader types i.e. for braches and leaves.
        for (uint i = 0; i < CONST_COUNT; ++i)
        {
            m_handles[i] = pEffect->GetParameterByName(NULL, g_constNames[i]);
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    SpeedTreeRendererCommon::SpeedTreeRendererCommon(const DataSectionPtr& cfg) : m_settings(cfg)
    {
        BW_GUARD;

        //-- zero vertex declarations.
        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            m_vertDclrs[i][0] = NULL;
            m_vertDclrs[i][1] = NULL;
        }

        //-- Note: we have to invoke these methods implicitly here because SpeedTreeRenderer is
        //--       called after render context creation therefore these method won't be called
        //--       automatically.
        createManagedObjects();
        createUnmanagedObjects();

        //-- ToDo: reconsider this.
        if (!Moo::rc().assetProcessingOnly())
        {
            BillboardOptimiser::initFXFiles();
        }
    }


    //----------------------------------------------------------------------------------------------
    SpeedTreeRendererCommon::~SpeedTreeRendererCommon()
    {

    }

    //----------------------------------------------------------------------------------------------
    bool SpeedTreeRendererCommon::readyToUse() const
    {
        BW_GUARD;

        bool success = true;

        //-- check ready status.
        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            success &= m_materials[i].first->baseMaterial()->pEffect() != NULL;

            //-- special materials.
            for (uint j = 0; j < SPECIAL_DRAWING_STATE_COUNT; ++j)
            {
                success &= m_specialMaterials[j][i].first->pEffect() != NULL;
            }
        }
    
        return success;
    }

    // Make sure all the effects and constant handles that use them are updated
    void SpeedTreeRendererCommon::ensureEffectsRecompiled()
    {
        bool effectRecompiled = false;

        // Find if any of the effects have recompiled. If at least one has
        // then ensure we rebuild all the tables of constant handles that
        // are stored. It is possible that effects are shared and
        // checkEffectRecompiled() will return false when called a second time.
        // Be pessimistic, otherwise could miss constants that need remapped.
        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            effectRecompiled |= m_materials[i].first->checkEffectRecompiled();

            for (uint j = 0; j < SPECIAL_DRAWING_STATE_COUNT; ++j)
            {
                Moo::EffectMaterialPtr mat = m_specialMaterials[j][i].first;
                effectRecompiled |= mat.get()->checkEffectRecompiled();
            }
        }

        if (!effectRecompiled) return;

        // Remap all the constant handles
        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            Moo::EffectMaterialPtr mat = m_materials[i].first->baseMaterial();
            m_materials[i].second.getHandles(mat);

            for (uint j = 0; j < SPECIAL_DRAWING_STATE_COUNT; ++j)
            {
                mat = m_specialMaterials[j][i].first;
                m_specialMaterials[j][i].second.getHandles(mat);
            }
        }
    }

    //-- prepare draw state of each individual tree part for rendering.
    //----------------------------------------------------------------------------------------------
    bool SpeedTreeRendererCommon::pass(Moo::ERenderingPassType type, bool instanced)
    {
        BW_GUARD;

        //-- Check pipeline capabilities.
        instanced = (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING) ?
            instanced : false;
#if !CONSUMER_CLIENT_BUILD
        this->ensureEffectsRecompiled();
#endif      

        //-- prepare tree parts rendering states.
        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            //-- prepare common drawing states.
            DrawingState& ds   = m_drawingStates[i];
            ds.m_material      = m_materials[i].first->pass(type, instanced).get();
            ds.m_constantsMap  = &m_materials[i].second;
            ds.m_instanced     = instanced;
            ds.m_vertexFormat  = m_vertDclrs[i][instanced];

            //-- setup quality setting.
            ds.m_material->pEffect()->pEffect()->SetBool(
                ds.m_constantsMap->handle(ConstantsMap::CONST_USE_HIGH_QUALITY), m_settings.activeOption().m_highQuality
                );

            //-- prepare special drawing states.
            for (uint j = 0; j < SPECIAL_DRAWING_STATE_COUNT; ++j)
            {
                DrawingState& ads  = m_specialDrawingStates[j][i];
                ads.m_material     = m_specialMaterials[j][i].first.get();
                ads.m_constantsMap = &m_specialMaterials[j][i].second;
                ads.m_vertexFormat = m_vertDclrs[i][0];

                ads.m_material->pEffect()->pEffect()->SetBool(
                    ads.m_constantsMap->handle(ConstantsMap::CONST_USE_HIGH_QUALITY), m_settings.activeOption().m_highQuality
                    );
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    const Moo::VertexFormat& SpeedTreeRendererCommon::vertexFormat( 
        ETreePart part, bool instancing )
    {
        return m_vertDclrs[part][instancing]->format();
    }

    //----------------------------------------------------------------------------------------------
    uint32 SpeedTreeRendererCommon::maxInstBufferSizeInBytes() const
    {
        return g_maxInstBufferSizeInBytes;
    }

    //----------------------------------------------------------------------------------------------
    void SpeedTreeRendererCommon::createUnmanagedObjects()
    {
        BW_GUARD;

        bool success = true;

        if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
        {
            //-- create buffer for the instancing drawing.
            //-- ToDo: reconsider memory usage. Currently is 80 * 3000 * 3 -> 720kb -> 0.72Mb
            //-- 80 bytes per instance, maximum 3000 instances per frame (for all passes not only main
            //-- color pass but for shadows passes and reflection pass), and x3 multiplier to be sure
            //-- we won't start rewriting data if CPU will be 3 frames ahead over GPU (that is default
            //-- behavior for almost every commodity graphics drivers, when GPU is really busy CPU doesn't
            //-- wait while GPU will done its work, it just generate data in advance with hope that
            //-- GPU has been busy for a short time and on the next frame it will done both frames,
            //-- previous and current).
            for (uint i = 0; i < TREE_PART_COUNT; ++i)
            {
                success &= SUCCEEDED(m_instVBs[i].create(
                    g_maxInstBufferSizeInBytes, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
                    "vertex buffer/speedtree instancing VB"
                    )); 

                m_drawingStates[i].m_instBuffer = &m_instVBs[i];
            }
        }

        MF_ASSERT(success && "Not all system resources have been loaded correctly.");
    }

    //----------------------------------------------------------------------------------------------
    void SpeedTreeRendererCommon::deleteUnmanagedObjects()
    {
        BW_GUARD;

        if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
        {
            for (uint i = 0; i < TREE_PART_COUNT; ++i)
            {
                m_instVBs[i].release();
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    void SpeedTreeRendererCommon::createManagedObjects()
    {
        BW_GUARD;

        bool success = true;
        bool createMaterials = !Moo::rc().assetProcessingOnly();

        //--
        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            if (createMaterials)
            {
                //-- create common material.
                m_materials[i].first = new Moo::ComplexEffectMaterial();
                success &= m_materials[i].first->initFromEffect(g_shaders[i]);
                success &= m_materials[i].second.getHandles(m_materials[i].first->baseMaterial());

                //-- create special material.
                for (uint j = 0; j < SPECIAL_DRAWING_STATE_COUNT; ++j)
                {
                    m_specialMaterials[j][i].first = new Moo::EffectMaterial();
                    success &= m_specialMaterials[j][i].first->initFromEffect(g_specialShader);
                    m_specialMaterials[j][i].first->hTechnique(g_specialShaderTechniques[j][i]);
                    success &= m_specialMaterials[j][i].second.getHandles(m_specialMaterials[j][i].first);
                }
            }

            //-- load vertex declarations.
            m_vertDclrs[i][0] = Moo::VertexDeclaration::get(g_vertDclrs[i]);
            success &= (m_vertDclrs[i][0] != NULL);
        }

        //-- try to create instancing stream vertex declaration.
        Moo::VertexDeclaration* instancing = Moo::VertexDeclaration::get(g_instancingVertDclr);
        success &= (instancing != NULL);

        //-- create instancing versions.
        if (success)
        {
            for (uint i = 0; i < TREE_PART_COUNT; ++i)
            {
                m_vertDclrs[i][1] = Moo::VertexDeclaration::combine(m_vertDclrs[i][0], instancing);
                success &= (m_vertDclrs[i][1] != NULL);
            }
        }

        if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
        {
            //-- disable billboard optimizer in deferred shading use instancing instead.
            TSpeedTreeType::s_optimiseBillboards_ = false;
        }

        MF_ASSERT(success && "Not all system resources have been loaded correctly.");
    }

    //----------------------------------------------------------------------------------------------
    void SpeedTreeRendererCommon::deleteManagedObjects()
    {
        BW_GUARD;

        for (uint i = 0; i < TREE_PART_COUNT; ++i)
        {
            //-- reset vertex declarations.
            m_vertDclrs[i][0] = NULL;
            m_vertDclrs[i][1] = NULL;

            //-- reset materials.
            m_materials[i].first = NULL;

            for (uint j = 0; j < SPECIAL_DRAWING_STATE_COUNT; ++j)
            {
                m_specialMaterials[j][i].first = NULL;
            }
        }
    }

} //-- speedtree

BW_END_NAMESPACE
