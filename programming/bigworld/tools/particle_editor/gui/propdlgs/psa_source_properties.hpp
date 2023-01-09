#pragma once
#include "afxwin.h"

#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/ccombobox_auto_complete.hpp"
#include "gui/vector_generator_custodian.hpp"

BW_BEGIN_NAMESPACE

class SourcePSA;

// PsaSourceProperties form view

class PsaSourceProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaSourceProperties)

public:
	PsaSourceProperties(); 
	virtual ~PsaSourceProperties();


	virtual void OnInitialUpdate();

public:
	enum { IDD = IDD_PSA_SOURCE_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	SourcePSA *	    action();
	void	        SetParameters(SetOperation task);
	afx_msg void    OnBnClickedPsaSourceButton();
	void	        setControlEnableStates();
	afx_msg void    OnBnClickedPsaSourcePositionGeneratorShow();
	afx_msg void    OnBnClickedPsaSourceVelocityGeneratorShow();
    afx_msg void    OnEnChangePsaSourceSpriteSpinrateRand();

    VectorGeneratorCustodian<SourcePSA> positionGeneratorCustodian_;
    VectorGeneratorCustodian<SourcePSA> velocityGeneratorCustodian_;
	CButton                             motionTriggered_;
	CButton                             timeTriggered_;
	CButton                             grounded_;
	controls::EditNumeric                        dropDistance_;
	controls::EditNumeric                        rate_;
	controls::EditNumeric                        sensitivity_;
	controls::EditNumeric                        activePeriod_;
	controls::EditNumeric                        sleepPeriod_;
	controls::EditNumeric                        minimumSize_;
	controls::EditNumeric                        maximumSize_;
	controls::EditNumeric                        forcedUnitSize_;
	controls::EditNumeric                        allowedTimeInSeconds_;
	controls::EditNumeric                        sleepPeriodMax_;
	controls::CComboBoxAutoComplete               positionGenerator_;   
	controls::CComboBoxAutoComplete               velocityGenerator_;    
	CButton                             velocityGeneratorShow_;
	CButton                             positionGeneratorShow_;
	CButton                             randomSpin_;
	controls::EditNumeric                        randomSpinMinRPS_;
	controls::EditNumeric                        randomSpinMaxRPS_;
	CButton                             initialOrientRandomise_;
	controls::EditNumeric                        initialOrientRandomiseX_;
	controls::EditNumeric                        initialOrientRandomiseY_;
	controls::EditNumeric                        initialOrientX_;
	controls::EditNumeric                        initialOrientY_;
	CStatic                             sizeBox_;
	controls::EditNumeric                        spriteInitialOrient_;
	controls::EditNumeric                        spriteInitialOrientRand_;
	controls::EditNumeric                        spriteSpin_;
	controls::EditNumeric                        spriteSpinRand_;
	controls::EditNumeric                        positionGeneratorX_;
	controls::EditNumeric                        positionGeneratorY_;
	controls::EditNumeric                        positionGeneratorZ_;
	controls::EditNumeric                        positionGeneratorX2_;
	controls::EditNumeric                        positionGeneratorY2_;
	controls::EditNumeric                        positionGeneratorZ2_;
	controls::EditNumeric                        velocityGeneratorX_;
	controls::EditNumeric                        velocityGeneratorY_;
	controls::EditNumeric                        velocityGeneratorZ_;
	controls::EditNumeric                        velocityGeneratorX2_;
	controls::EditNumeric                        velocityGeneratorY2_;
	controls::EditNumeric                        velocityGeneratorZ2_;
    controls::EditNumeric                        inheritVelocity_;
    controls::EditNumeric                        maxSpeed_;

private:
	static bool s_showVel_;
	static bool s_showPos_;
	controls::EditNumeric	delay_;
};
BW_END_NAMESPACE

