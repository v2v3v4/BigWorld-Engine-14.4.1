#pragma once

#include "afxwin.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/general_properties.hpp"

BW_BEGIN_NAMESPACE

class OrbitorPSA;

// PsaOrbitorProperties form view

class PsaOrbitorProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaOrbitorProperties)

public:
	PsaOrbitorProperties(); 
	virtual ~PsaOrbitorProperties();

	enum { IDD = IDD_PSA_ORBITOR_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	OrbitorPSA *	action();
	void			SetParameters(SetOperation task);

	afx_msg void OnBnClickedPsaOrbitorButton();

	controls::EditNumeric x_;
	controls::EditNumeric z_;
	controls::EditNumeric angularVelocity_;
	CButton affectVelocity_;

private:
	void		position(const Vector3 & position);
	Vector3 	position() const;

	void		addPositionGizmo();
	void		removePositionGizmo();

	SmartPointer< VectorGeneratorMatrixProxy<PsaOrbitorProperties> > positionMatrixProxy_;
	SmartPointer< PositionGizmo >		positionGizmo_;
	controls::EditNumeric	delay_;
};


BW_END_NAMESPACE

