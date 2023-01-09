#ifndef PSA_PROPERTIES_HPP
#define PSA_PROPERTIES_HPP

#include "gui/gui_utilities.hpp"
#include "controls/auto_tooltip.hpp"
#include "particle/actions/particle_system_action.hpp"

BW_BEGIN_NAMESPACE

class PsaProperties : public CFormView
{
public:
    PsaProperties(UINT nIDTemplate);

    void    SetPSA(ParticleSystemActionPtr action);

    void    CopyDataToControls();
    void    CopyDataToPSA();

    virtual void SetParameters(SetOperation task) = 0;

    // windows member overrides
    virtual void PostNcDestroy();
    virtual void OnInitialUpdate();
    virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
    afx_msg LRESULT OnUpdatePsaProperties(WPARAM mParam, LPARAM lParam);
    DECLARE_AUTO_TOOLTIP_EX(PsaProperties)
    DECLARE_MESSAGE_MAP()

protected:
    ParticleSystemActionPtr	action_;
    bool                    initialised_;
};

BW_END_NAMESPACE

#endif // PSA_PROPERTIES_HPP
