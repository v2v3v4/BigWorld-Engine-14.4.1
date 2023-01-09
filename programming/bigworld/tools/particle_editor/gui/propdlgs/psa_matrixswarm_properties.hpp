#ifndef PSA_MATRIXSWARM_PROPERTIES_HPP
#define PSA_MATRIXSWARM_PROPERTIES_HPP

#include "resource.h"
#include "psa_properties.hpp"
#include "controls/edit_numeric.hpp"

BW_BEGIN_NAMESPACE

class MatrixSwarmPSA;


class PsaMatrixSwarmProperties : public PsaProperties
{
public:
    DECLARE_DYNCREATE(PsaMatrixSwarmProperties)

    enum { IDD = IDD_PSA_MATRIXSWARM_PROPERTIES };

    //
    // Constructor.
    //
    PsaMatrixSwarmProperties();

	MatrixSwarmPSA * action();

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

#endif // PSA_MATRIXSWARM_PROPERTIES_HPP
