#pragma once

#include "afxwin.h"

#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_button.hpp"

BW_BEGIN_NAMESPACE

class FlarePSA;

// PsaFlareProperties form view

class PsaFlareProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaFlareProperties)

public:
	PsaFlareProperties(); 
	virtual ~PsaFlareProperties();

	enum { IDD = IDD_PSA_FLARE_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnUpdatePsRenderProperties(WPARAM mParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	FlarePSA *	action();
	void		SetParameters(SetOperation task);

	afx_msg void OnBnClickedPsaFlareButton();
	afx_msg void OnCbnSelchangeFlarename();
	afx_msg void OnBnClickedPsaFlareFlarenameDirectory();

	void finishPopulatingFlareNames();

    controls::ImageButton   flareNameDirectoryBtn_;
    CEdit                   flareNameDirectoryEdit_;
	CComboBox	            flareNameSelection_;
	controls::EditNumeric	flareStep_;
	CButton		            colourize_;
	CButton		            useParticleSize_;
	controls::EditNumeric	delay_;
};


BW_END_NAMESPACE

