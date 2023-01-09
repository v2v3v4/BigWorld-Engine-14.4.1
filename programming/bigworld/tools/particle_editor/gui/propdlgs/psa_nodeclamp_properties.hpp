#ifndef PSA_NODECLAMP_PROPERTIES_HPP
#define PSA_NODECLAMP_PROPERTIES_HPP

#include "resource.h"
#include "controls/edit_numeric.hpp"
#include "psa_properties.hpp"

BW_BEGIN_NAMESPACE

class NodeClampPSA;

class PsaNodeClampProperties : public PsaProperties
{
public:
    DECLARE_DYNCREATE(PsaNodeClampProperties)

    enum { IDD = IDD_PSA_NODECLAMP_PROPERTIES };

    //
    // Constructor.
    //
    PsaNodeClampProperties();

	NodeClampPSA *	action();

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

#endif // PSA_NODECLAMP_PROPERTIES_HPP
