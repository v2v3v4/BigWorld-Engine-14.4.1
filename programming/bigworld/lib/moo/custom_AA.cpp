#include "pch.hpp"
#include "custom_AA.hpp"
#include "ppaa_support.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
    //----------------------------------------------------------------------------------------------
    CustomAA::CustomAA()
        : m_mode(0), m_modesCount(4), m_ppaa(new PPAASupport())
    {

    }

    //----------------------------------------------------------------------------------------------
    CustomAA::~CustomAA()
    {

    }

    //----------------------------------------------------------------------------------------------
    void CustomAA::mode(uint id)
    {
        if (id < m_modesCount)
        {
            m_mode = id;
        }
        else
        {
            //-- clamp results in case if user's cfg files already have invalid mode indices.
            m_mode = Math::clamp<uint>(0, m_mode, 3);
            ERROR_MSG("Invalid customAA mode index.");
        }
    }

    //----------------------------------------------------------------------------------------------
    void CustomAA::apply()
    {
        //-- if multi-sampling enabled or MRT is disabled it means that we don't want to use custom
        //-- anti-aliasing, so disable it internally but save desired custom AA mode.
        if (Moo::rc().isMultisamplingEnabled())
        {
            m_ppaa->enable(false);
        }
        else
        {
            switch (m_mode)
            {
            case 0:
                {
                    m_ppaa->enable(false);
                    break;
                }
            case 1:
            case 2:
            case 3:
                {
                    m_ppaa->enable(true);
                    m_ppaa->mode(m_mode - 1);   // convert to zero-based index
                    break;
                }
            default:
                {
                    MF_ASSERT("Can't get here.");
                    return;
                }
            }
        }
    }

} //-- Moo

BW_END_NAMESPACE
