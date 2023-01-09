#include "pch.hpp"
#include "complex_effect_material.hpp"
#include "managed_effect.hpp"
#include "renderer.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

    //----------------------------------------------------------------------------------------------
    ComplexEffectMaterial::ComplexEffectMaterial()
        :   m_instanced(false), m_skinned(false),
            m_material(new EffectMaterial()),
            m_channelType( NULL_CHANNEL )
    {
        for (uint i = 0; i < RENDERING_PASS_COUNT; ++i)
        {
            m_techniques[i][0] = NULL;
            m_techniques[i][1] = NULL;
        }
    }

    //----------------------------------------------------------------------------------------------
    ComplexEffectMaterial::~ComplexEffectMaterial()
    {

    }

    //----------------------------------------------------------------------------------------------
    ComplexEffectMaterial::ComplexEffectMaterial(const ComplexEffectMaterial& other)
    {
        *this = other;
    }

    //----------------------------------------------------------------------------------------------
    ComplexEffectMaterial& ComplexEffectMaterial::operator = (const ComplexEffectMaterial& other)
    {
        m_instanced = other.m_instanced;
        m_skinned   = other.m_skinned;
        m_channelType    = other.m_channelType;
        m_material  = new EffectMaterial(*other.m_material); 

        for (uint i = 0; i < RENDERING_PASS_COUNT; ++i)
        {
            m_techniques[i][0] = other.m_techniques[i][0];
            m_techniques[i][1] = other.m_techniques[i][1];
        }

        return *this;
    }

    //----------------------------------------------------------------------------------------------
    bool ComplexEffectMaterial::load(const DataSectionPtr& section, bool addDefault/* = true*/)
    {
        BW_GUARD;
        bool res = m_material->load(section, addDefault);
        if (res)
        {
            res &= this->finishInit();
        }
        return res;
    }

    //----------------------------------------------------------------------------------------------
    bool ComplexEffectMaterial::initFromEffect(
        const BW::string& effect, const BW::string& diffuseMap /* =  */, int doubleSided /* = -1 */)
    {
        BW_GUARD;
        bool res = m_material->initFromEffect(effect, diffuseMap, doubleSided);
        if (res)
        {
            res &= this->finishInit();
        }

        return res;

    }

    //----------------------------------------------------------------------------------------------
    /**
     *  Finish initialising the material.
     */ 
    bool ComplexEffectMaterial::finishInit()
    {
        //-- check materials status.
        const ManagedEffectPtr& effect = m_material->pEffect();
        if (!effect)
        {   
            return false;
        }

        //-- If we are here that finishInit successed and now we can parse all techniques.
        for (uint i = 0; i < effect->techniques().size(); ++i)
        {
            const TechniqueInfo& info      = effect->techniques()[i];
            int                  passType  = info.renderPassType_;
            uint                 instanced = info.instanced_ ? 1 : 0;

            //-- fill techniques.
            switch (passType)
            {
            case RENDERING_PASS_COLOR:
            case RENDERING_PASS_REFLECTION:
            case RENDERING_PASS_SHADOWS:
            case RENDERING_PASS_DEPTH:
                {
                    m_techniques[passType][instanced] = info.handle_;
                    break;
                }
            default:
                ERROR_MSG("Invalid rendering pass type.");
            };
        }

        //-- now validate loaded techniques.
        m_skinned    = m_material->skinned();
        m_instanced  = m_techniques[RENDERING_PASS_COLOR][1] != NULL;
        m_channelType = m_material->channelType();
        MF_ASSERT( m_channelType != NOT_SUPPORTED_CHANNEL );
        return true;
    }

} //-- Moo

BW_END_NAMESPACE
