// LoadingDialog.cpp : implementation file
//
#include "pch.hpp"

#include "shader_loading_dialog.hpp"

BW_BEGIN_NAMESPACE

// Default constructor.
CShaderLoadingDialog::CShaderLoadingDialog()
	: CDialog(CShaderLoadingDialog::IDD)
{
	BW_GUARD;

	Create( IDD_SHADER_LOADING );
}

// Destructor.
CShaderLoadingDialog::~CShaderLoadingDialog()
{
	BW_GUARD;

	DestroyWindow();
}

// Setup data exchange with progress bar.
void CShaderLoadingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SHADER_PROGRESS_BAR, bar_);
}

// On initialisation centre the progress bar.
BOOL CShaderLoadingDialog::OnInitDialog() 
{
	BW_GUARD;

	CDialog::OnInitDialog();

	this->CenterWindow();

	return TRUE;
}

// Set the range of the progress bar.
void CShaderLoadingDialog::setRange( int num )
{
	BW_GUARD;

	bar_.SetRange( 0, num );
	bar_.SetStep( 1 );
}

// Step the progress bar on a unit.
void CShaderLoadingDialog::step()
{
	BW_GUARD;

	bar_.StepIt();
}


BEGIN_MESSAGE_MAP(CShaderLoadingDialog, CDialog)
END_MESSAGE_MAP()
BW_END_NAMESPACE

