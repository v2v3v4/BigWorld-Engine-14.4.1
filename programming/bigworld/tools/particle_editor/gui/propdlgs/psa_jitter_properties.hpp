#pragma once

#include "afxwin.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/ccombobox_auto_complete.hpp"
#include "gui/vector_generator_custodian.hpp"

BW_BEGIN_NAMESPACE

class JitterPSA;


// PsaJitterProperties form view

class PsaJitterProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaJitterProperties)

public:
	PsaJitterProperties(); 
	virtual ~PsaJitterProperties();

	enum { IDD = IDD_PSA_JITTER_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	JitterPSA *		action();
	void			SetParameters(SetOperation task);

	CButton affectPosition_;
	CButton affectVelocity_;
	afx_msg void OnBnClickedPsaJitterAffectPosition();
	afx_msg void OnBnClickedPsaJitterAffectVelocity();
	afx_msg void OnBnClickedPsaJitterButton();

private:	
	VectorGeneratorCustodian<JitterPSA>     positionGeneratorCustodian_;	
	VectorGeneratorCustodian<JitterPSA>     velocityGeneratorCustodian_;
    controls::CComboBoxAutoComplete                   positionGenerator_;
    controls::CComboBoxAutoComplete                   velocityGenerator_;
	controls::EditNumeric delay_;
    bool                                    filterUpdatePSA_;
};


BW_END_NAMESPACE

