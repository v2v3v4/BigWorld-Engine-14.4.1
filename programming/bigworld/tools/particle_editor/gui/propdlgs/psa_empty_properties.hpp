#ifndef PSA_EMPTY_PROPERTIES_HPP
#define PSA_EMPTY_PROPERTIES_HPP

#include "gui/propdlgs/psa_properties.hpp"

BW_BEGIN_NAMESPACE

class PsaEmptyProperties : public PsaProperties
{
    DECLARE_DYNCREATE(PsaEmptyProperties)

public:
    enum { IDD = IDD_PSA_EMPTY };

    PsaEmptyProperties(); 

    /*virtual*/ ~PsaEmptyProperties();

    /*virtual*/ void *action();

    /*virtual*/ void SetParameters(SetOperation);

protected:
    DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

#endif // PSA_EMPTY_PROPERTIES_HPP
