#pragma once

#include "afxwin.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"

BW_BEGIN_NAMESPACE

class MagnetPSA;


// PsaMagnetProperties form view

class PsaMagnetProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaMagnetProperties)

public:
	PsaMagnetProperties(); 
	virtual ~PsaMagnetProperties();

	enum { IDD = IDD_PSA_MAGNET_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()


public:
	MagnetPSA *		action();
	void			SetParameters(SetOperation task);

	controls::EditNumeric strength_;
	controls::EditNumeric minDist_;
	controls::EditNumeric	delay_;
};


BW_END_NAMESPACE

