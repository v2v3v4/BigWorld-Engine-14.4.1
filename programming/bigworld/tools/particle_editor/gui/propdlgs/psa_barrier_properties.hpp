#pragma once

#include "afxwin.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/ccombobox_auto_complete.hpp"
#include "gizmo/combination_gizmos.hpp"
#include "particle/actions/barrier_psa.hpp"

BW_BEGIN_NAMESPACE

class BarrierPSA;


// PsaBarrierProperties form view

class PsaBarrierProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaBarrierProperties)

public:
	PsaBarrierProperties(); 
	virtual ~PsaBarrierProperties();

	enum { IDD = IDD_PSA_BARRIER_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()


public:
	BarrierPSA *	action() { return static_cast<BarrierPSA *>(&*action_); }
	void			SetParameters(SetOperation task);

	controls::CComboBoxAutoComplete barrierShape_;
	controls::CComboBoxAutoComplete barrierReaction_;

//	VectorGeneratorCustodian<BarrierPSA> barrierGeneratorCustodian_;
	afx_msg void OnCbnSelchangePsaBarrierCombo();

private:
	void setGizmo(BarrierPSA::Shape shapeType);
	void populate();

	GizmoPtr		gizmo_;
	bool			populated_;
	controls::EditNumeric	delay_;
};


BW_END_NAMESPACE

