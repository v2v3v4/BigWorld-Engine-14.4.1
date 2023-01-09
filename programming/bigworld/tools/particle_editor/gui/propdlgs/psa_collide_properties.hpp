#ifndef PSA_COLLIDE_PROPERTIES_HPP
#define PSA_COLLIDE_PROPERTIES_HPP

#include "afxwin.h"
#include "fwd.hpp"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_button.hpp"

BW_BEGIN_NAMESPACE

class PsaCollideProperties : public PsaProperties
{
    DECLARE_DYNCREATE(PsaCollideProperties)

public:
    enum { IDD = IDD_PSA_COLLIDE_PROPERTIES };

    PsaCollideProperties(); 

    /*virtual*/ ~PsaCollideProperties();  

    /*virtual*/ void OnInitialUpdate();

    /*virtual*/ void DoDataExchange(CDataExchange* pDX); 

     CollidePSA *action();

    void SetParameters(SetOperation task);

private:
	controls::EditNumeric	elasticity_;
	controls::EditNumeric	delay_;
};

BW_END_NAMESPACE

#endif // PSA_COLLIDE_PROPERTIES_HPP
