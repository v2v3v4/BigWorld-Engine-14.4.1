#pragma once

#include "afxwin.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/general_properties.hpp"

BW_BEGIN_NAMESPACE

class StreamPSA;

// PsaStreamProperties form view

class PsaStreamProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaStreamProperties)

public:
	PsaStreamProperties(); 
	virtual ~PsaStreamProperties();

	enum { IDD = IDD_PSA_STREAM_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	controls::EditNumeric halfLife_;
	controls::EditNumeric z_;
	controls::EditNumeric y_;
	controls::EditNumeric x_;

public:
	StreamPSA *	action();
	void		SetParameters(SetOperation task);

private:
	void		position(const Vector3 & position);
	Vector3 	position() const;

	void		addPositionGizmo();
	void		removePositionGizmo();

	SmartPointer< VectorGeneratorMatrixProxy<PsaStreamProperties> > positionMatrixProxy_;
	GizmoPtr		positionGizmo_;
	controls::EditNumeric	delay_;
};


BW_END_NAMESPACE

