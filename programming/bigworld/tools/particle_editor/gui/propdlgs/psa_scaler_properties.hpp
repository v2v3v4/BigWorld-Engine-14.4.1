#pragma once

#include "afxwin.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"

BW_BEGIN_NAMESPACE

class ScalerPSA;

// PsaScalerProperties form view

class PsaScalerProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaScalerProperties)

public:
	PsaScalerProperties();           // protected constructor used by dynamic creation
	virtual ~PsaScalerProperties();

	enum { IDD = IDD_PSA_SCALER_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()


public:
	ScalerPSA *	action();
	void		SetParameters(SetOperation task);

	controls::EditNumeric size_;
	controls::EditNumeric rate_;
	controls::EditNumeric	delay_;
};

BW_END_NAMESPACE

