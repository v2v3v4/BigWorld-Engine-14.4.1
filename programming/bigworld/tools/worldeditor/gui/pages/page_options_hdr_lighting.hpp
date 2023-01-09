#ifndef PAGE_OPTIONS_HDR_LIGHTING_HPP
#define PAGE_OPTIONS_HDR_LIGHTING_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "guitabs/guitabs_content.hpp"
#include "controls/color_picker.hpp"
#include "controls/color_timeline.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/dialog_toolbar.hpp"
#include "controls/image_button.hpp"
#include "romp/hdr_settings.hpp"
#include "romp/ssao_settings.hpp"
#include "afxwin.h"

BW_BEGIN_NAMESPACE

class PageOptionsHDRLighting : public CFormView, public GUITABS::Content
{
    IMPLEMENT_BASIC_CONTENT
    (
        L"HDR Lighting",              // short name of page
        L"HDR Lighting",      // long name of page
        290,                        // default width
        500,                        // default height
        NULL                        // icon
    )

    static bool wasSlider( CWnd const & testScrollBar, CWnd const & scrollBar );

public:
    enum { IDD = IDD_PAGE_OPTIONS_HDR_LIGHTING };

    PageOptionsHDRLighting();

    /*virtual*/ ~PageOptionsHDRLighting();

    static PageOptionsHDRLighting *instance();
    void reinitialise();
    static EnviroMinder &getEnviroMinder();

protected:
    enum SliderMovementState
    {
        SMS_STARTED,
        SMS_MIDDLE,
        SMS_DONE
    };

    void InitPage();
    /*virtual*/ void DoDataExchange(CDataExchange *dx);

    afx_msg LRESULT OnUpdateControls(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg LRESULT OnNewSpace(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg void OnHScroll(UINT /*sbcode*/, UINT /*pos*/, CScrollBar *scrollBar);
    afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result);
    afx_msg void OnPaint();
    afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

    //--
#undef  BW_GENERATE_SSAO_ACCESSOR_DECLARATION
#define BW_GENERATE_SSAO_ACCESSOR_DECLARATION(funcName) \
    void funcName##Slider(SliderMovementState sms);     \
    afx_msg void funcName##Edit();

    //-- SSAO: common settings.
    afx_msg void OnSSAOEnableBnClicked();   
    afx_msg void OnSSAOShowBufferBnClicked();   
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFade)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOAmplify)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAORadius)

    //-- SSAO: influence factors.
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFactorSpeedTreeAmbient)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFactorSpeedTreeLighting)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFactorTerrainAmbient)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFactorTerrainLighting)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFactorObjectsAmbient)
    BW_GENERATE_SSAO_ACCESSOR_DECLARATION(OnSSAOFactorObjectsLighting)

#undef BW_GENERATE_SSAO_ACCESSOR_DECLARATION

    //--
#undef  BW_GENERATE_HDR_ACCESSOR_DECLARATION
#define BW_GENERATE_HDR_ACCESSOR_DECLARATION(funcName)  \
    void funcName##Slider(SliderMovementState sms);     \
    afx_msg void funcName##Edit();

    //-- HDR: common settings.
    afx_msg void OnHDREnableBnClicked();    
    afx_msg void OnHDREnableLinearSpaceLightingBnClicked();
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRAdaptationSpeed)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRAvgLumUpdateInterval)

    //-- HDR: bloom settings.
    afx_msg void OnHDRBloomEnableBnClicked();
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRBloomBrightThreshold)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRBloomBrightOffset)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRBloomAmount)

    //-- HDR: tone mapping settings.
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRToneMappingEyeDarkLimit)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRToneMappingEyeLightLimit)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRToneMappingMiddleGray)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDRToneMappingWhitePoint)

    //-- HDR: environment settings.
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDREnvSkyLumMultiplier)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDREnvSunLumMultiplier)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDREnvAmbientLumMultiplier)
    BW_GENERATE_HDR_ACCESSOR_DECLARATION(OnHDREnvFogLumMultiplier)

#undef BW_GENERATE_HDR_ACCESSOR_DECLARATION

    afx_msg void OnKillFocusHDRAdaptSpeed();
    afx_msg void OnKillFocusHDRAvgLumUpdateInterval();
    afx_msg void OnKillFocusHDRBloomBrightThreshold();
    afx_msg void OnKillFocusHDRBloomBrightOffset();
    afx_msg void OnKillFocusHDRBloomAmount();
    afx_msg void OnKillFocusHDRToneMappingEyeDarkLimit();
    afx_msg void OnKillFocusHDRToneMappingEyeLightLimit();
    afx_msg void OnKillFocusHDRToneMappingMiddleGray();
    afx_msg void OnKillFocusHDRToneMappingWhitePoint();
    afx_msg void OnKillFocusHDREnvSkyLumMultiplier();
    afx_msg void OnKillFocusHDREnvSunLumMultiplier();
    afx_msg void OnKillFocusHDREnvAmbientLumMultiplier();
    afx_msg void OnKillFocusHDREnvFogLumMultiplier();
    afx_msg void OnKillFocusSSAOFade();
    afx_msg void OnKillFocusSSAOAmplify();
    afx_msg void OnKillFocusSSAORadius();
    afx_msg void OnKillFocusSSAOFactorSpeedTreeAmbient();
    afx_msg void OnKillFocusSSAOFactorSpeedTreeLighting();
    afx_msg void OnKillFocusSSAOFactorTerrainAmbient();
    afx_msg void OnKillFocusSSAOFactorTerrainLighting();
    afx_msg void OnKillFocusSSAOFactorObjectsAmbient();
    afx_msg void OnKillFocusSSAOFactorObjectsLighting();

    DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP(PageOptionsHDRLighting, CFormView)

    void saveUndoState(BW::string const &description);
    void periodicSaveUndoState();

private:

    //--
#undef  BW_GENERATE_SSAO_CONTROL
#define BW_GENERATE_SSAO_CONTROL(name)      \
    controls::EditNumeric   name##Edit_;    \
    LimitSlider             name##Slider_;

    //-- SSAO settings
    CButton ssaoEnableBtn_;
    CButton ssaoShowBufferBtn_;
    BW_GENERATE_SSAO_CONTROL(ssaoFade)
    BW_GENERATE_SSAO_CONTROL(ssaoAmplify)
    BW_GENERATE_SSAO_CONTROL(ssaoRadius)

    BW_GENERATE_SSAO_CONTROL(ssaoFactorSpeedTreeAmbient)
    BW_GENERATE_SSAO_CONTROL(ssaoFactorSpeedTreeLighting)
    BW_GENERATE_SSAO_CONTROL(ssaoFactorTerrainAmbient)
    BW_GENERATE_SSAO_CONTROL(ssaoFactorTerrainLighting)
    BW_GENERATE_SSAO_CONTROL(ssaoFactorObjectsAmbient)
    BW_GENERATE_SSAO_CONTROL(ssaoFactorObjectsLighting)

#undef  BW_GENERATE_SSAO_CONTROL
    
        //--
#undef  BW_GENERATE_HDR_CONTROL
#define BW_GENERATE_HDR_CONTROL(name)       \
    controls::EditNumeric   name##Edit_;    \
    LimitSlider             name##Slider_;

    //-- HDR: common settings.
    CButton hdrEnableBtn_;
    CButton hdrUseLinearSpaceLightingBtn_;
    BW_GENERATE_HDR_CONTROL(hdrAdaptSpeed)
    BW_GENERATE_HDR_CONTROL(hdrAvgLumUpdateInterval)

    //-- HDR: bloom settings.
    CButton hdrBloomEnableBtn_;
    BW_GENERATE_HDR_CONTROL(hdrBloomBrightThreshold)
    BW_GENERATE_HDR_CONTROL(hdrBloomBrightOffset)
    BW_GENERATE_HDR_CONTROL(hdrBloomAmount)

    //-- HDR: tone mapping settings.
    BW_GENERATE_HDR_CONTROL(hdrToneMappingEyeDarkLimit)
    BW_GENERATE_HDR_CONTROL(hdrToneMappingEyeLightLimit)
    BW_GENERATE_HDR_CONTROL(hdrToneMappingMiddleGray)
    BW_GENERATE_HDR_CONTROL(hdrToneMappingWhitePoint)

    //-- HDR: environment settings.
    BW_GENERATE_HDR_CONTROL(hdrEnvSkyLumMultiplier)
    BW_GENERATE_HDR_CONTROL(hdrEnvSunLumMultiplier)
    BW_GENERATE_HDR_CONTROL(hdrEnvAmbientLumMultiplier)
    BW_GENERATE_HDR_CONTROL(hdrEnvFogLumMultiplier)

#undef BW_GENERATE_HDR_CONTROL

    //-- templated helper for settings hdr constants.
    template<typename Type>
    void onHDREditGeneric(HDRSettings& settings, Type& curValue, controls::EditNumeric& ed, LimitSlider& ls, const char* msg)
    {
        if (filterChange_ == 0)
        {
            ++filterChange_;

            float changedValue = Math::clamp( 
                ls.getMinRange(), ed.GetValue(), ls.getMaxRange() );
            if (curValue != changedValue)
            {
                curValue = changedValue;
                ls.SetPos( static_cast< int >( curValue * 100.0f ) );

                saveUndoState(msg);
                getEnviroMinder().hdrSettings(settings);
                WorldManager::instance().environmentChanged();      
            }

            --filterChange_;        
        }
    }

    //-- templated helper for settings hdr constants.
    template<typename Type>
    void onHDRSliderGeneric(HDRSettings& settings, Type& curValue, controls::EditNumeric& ed, LimitSlider& ls, const char* msg)
    {
        if (filterChange_ == 0)
        {
            ++filterChange_;

            curValue = ls.GetPos() / 100.0f;
            ed.SetValue(curValue);

            saveUndoState(msg);
            getEnviroMinder().hdrSettings(settings);
            WorldManager::instance().environmentChanged();      

            --filterChange_;
        }
    }

    //-- templated helper for settings SSAO constants.
    template<typename Type>
    void onSSAOSliderGeneric(SSAOSettings& settings, Type& curValue, controls::EditNumeric& ed, LimitSlider& ls, const char* msg)
    {
        if (filterChange_ == 0)
        {
            ++filterChange_;

            curValue = ls.GetPos() / 100.0f;
            ed.SetValue(curValue);

            saveUndoState(msg);
            getEnviroMinder().ssaoSettings(settings);
            WorldManager::instance().environmentChanged();      

            --filterChange_;
        }
    }

    //-- templated helper for settings SSAO constants.
    template<typename Type>
    void onSSAOEditGeneric(SSAOSettings& settings, Type& curValue, controls::EditNumeric& ed, LimitSlider& ls, const char* msg)
    {
        if (filterChange_ == 0)
        {
            ++filterChange_;

            float changedValue = Math::clamp( 
                ls.getMinRange(), ed.GetValue(), ls.getMaxRange() );
            if (curValue != changedValue)
            {
                curValue = changedValue;
                ls.SetPos( static_cast< int >( curValue * 100.0f ) );

                saveUndoState(msg);
                getEnviroMinder().ssaoSettings(settings);
                WorldManager::instance().environmentChanged();      
            }

            --filterChange_;        
        }
    }

    size_t filterChange_;
    bool initialised_;
    bool sliding_;

    BW::string nextSaveUndoMsg_;
    uint64 nextSaveUndoTime_;
    UndoRedo::Operation *nextEnvironmentUndoOp_;

    static PageOptionsHDRLighting   *s_instance_;

    struct Impl;
    Impl* pImpl;
    friend Impl;
};

IMPLEMENT_BASIC_CONTENT_FACTORY(PageOptionsHDRLighting)

BW_END_NAMESPACE

#endif // PAGE_OPTIONS_HDR_LIGHTING_HPP
