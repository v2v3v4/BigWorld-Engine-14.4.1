#ifndef PSA_SPLAT_PROPERTIES_HPP
#define PSA_SPLAT_PROPERTIES_HPP

#include "resource.h"
#include "controls/edit_numeric.hpp"
#include "psa_properties.hpp"

BW_BEGIN_NAMESPACE

class SplatPSA;

class PsaSplatProperties : public PsaProperties
{
public:
    DECLARE_DYNCREATE(PsaSplatProperties)

    enum { IDD = IDD_PSA_SPLAT_PROPERTIES };

    //
    // Constructor.
    //
    PsaSplatProperties();

	SplatPSA *	action();

    //
    // Set the parameters.
    //
    // @param task      The parameters to set.
    //
    /*virtual*/ void SetParameters(SetOperation /*task*/);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()


private:
	controls::EditNumeric	delay_;
};

BW_END_NAMESPACE

#endif // PSA_SPLAT_PROPERTIES_HPP
