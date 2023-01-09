#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/propdlgs/psa_flare_properties.hpp"
#include "controls/dir_dialog.hpp"
#include "gui/gui_utilities.hpp"
#include "appmgr/options.hpp"
#include "particle/actions/flare_psa.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

const CString c_flarePropertiesdefaultDirectoryText("No Directory");

IMPLEMENT_DYNCREATE(PsaFlareProperties, PsaProperties)

PsaFlareProperties::PsaFlareProperties()
: 
PsaProperties(PsaFlareProperties::IDD)
{
	BW_GUARD;

	flareStep_.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaFlareProperties::~PsaFlareProperties()
{
}

static bool validFlareFilename( const BW::string & filename )
{
	BW_GUARD;

	if (BWResource::getExtension( filename ) == "xml")
	{
		return LensEffect::isLensEffect( filename );
	}

	return false;
}

void PsaFlareProperties::OnInitialUpdate()
{
	BW_GUARD;

    PsaProperties::OnInitialUpdate();	// data is copied to controls

	SetParameters(SET_CONTROL);
	initialised_ = true;

	// By doing this the lens effect is loaded and thus checked for validity
	CopyDataToPSA();

    flareNameDirectoryBtn_.setBitmapID(IDB_OPEN, IDB_OPEND);

	ParticleSystemPtr pSystem = 
        MainFrame::instance()->GetCurrentParticleSystem();
 
	if (pSystem)
	{
		bool isMesh = pSystem->pRenderer()->isMeshStyle();
		useParticleSize_.EnableWindow( !isMesh );
	}
}

afx_msg LRESULT PsaFlareProperties::OnUpdatePsRenderProperties(WPARAM mParam, LPARAM lParam)
{
	BW_GUARD;

	if (initialised_)
		SetParameters(SET_PSA);

	return 0;
}

void PsaFlareProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_INT_PARAMETER(task, flareStep);
	SET_FLOAT_PARAMETER(task, delay);
	SET_CHECK_PARAMETER(task, colourize);
	SET_CHECK_PARAMETER(task, useParticleSize);

	if (task == SET_CONTROL)
	{
		// read in
		FlarePSA * flarePSA = action();

		CString longFilename = bw_utf8tow( flarePSA->flareName() ).c_str();
		CString filename(L"");
		CString directory(L"");

		if (longFilename.IsEmpty())
		{
			// nothing set.. give an appropriate directory by looking where the sun flare is
			longFilename = bw_utf8tow( Options::getOptionString( "resourceGlue/environment/sunFlareXML" ) ).c_str();

			// see if already have saved one
			longFilename = bw_utf8tow( Options::getOptionString( "defaults/flareXML", bw_wtoutf8( longFilename.GetString() ) ) ).c_str();
		}

		GetFilenameAndDirectory(longFilename, filename, directory);

		// remember for next time
		Options::setOptionString( "defaults/flareXML", bw_wtoutf8( longFilename.GetString() ) );

		// populate with all the textures in that directory
		BW::string relativeDirectory = BWResource::dissolveFilename( bw_wtoutf8( directory.GetString() ));
		PopulateComboBoxWithFilenames
        (
            flareNameSelection_, 
            relativeDirectory, 
            validFlareFilename
        );
		finishPopulatingFlareNames();
		flareNameDirectoryEdit_.SetWindowText(bw_utf8tow( relativeDirectory ).c_str());
		flareNameSelection_.SelectString(-1, filename);
	}
	else
	{
		// write out
		FlarePSA * flarePSA = action();
		
		int selected = flareNameSelection_.GetCurSel();
		if (selected != -1)
		{
			CString texName, dirName;
			flareNameSelection_.GetLBText(selected, texName);
			flareNameDirectoryEdit_.GetWindowText(dirName);
			BW::string nflareName = bw_wtoutf8( (dirName + L"/" + texName).GetString() );
			flarePSA->flareName( nflareName );

			// remember for next time
			Options::setOptionString( "defaults/flareXML", nflareName );
		}
	}
}

void PsaFlareProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_FLARE_FLARENAME, flareNameSelection_);
	DDX_Control(pDX, IDC_PSA_FLARE_FLARESTEP, flareStep_);
	DDX_Control(pDX, IDC_PSA_FLARE_COLOURIZE, colourize_);
	DDX_Control(pDX, IDC_PSA_FLARE_USEPARTICLESIZE, useParticleSize_);
	DDX_Control(pDX, IDC_PSA_FLARE_FLARENAME_DIRECTORY_BTN, flareNameDirectoryBtn_);
    DDX_Control(pDX, IDC_PSA_FLARE_FLARENAME_DIRECTORY_EDIT, flareNameDirectoryEdit_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}

BEGIN_MESSAGE_MAP(PsaFlareProperties, PsaProperties)
	ON_BN_CLICKED(IDC_PSA_FLARE_COLOURIZE, OnBnClickedPsaFlareButton)
	ON_BN_CLICKED(IDC_PSA_FLARE_USEPARTICLESIZE, OnBnClickedPsaFlareButton)
	ON_CBN_SELCHANGE(IDC_PSA_FLARE_FLARENAME, OnBnClickedPsaFlareButton)
	ON_BN_CLICKED(IDC_PSA_FLARE_FLARENAME_DIRECTORY_BTN, OnBnClickedPsaFlareFlarenameDirectory)
END_MESSAGE_MAP()


// PsaFlareProperties diagnostics

#ifdef _DEBUG
void PsaFlareProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaFlareProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


// PsaFlareProperties message handlers

void PsaFlareProperties::OnBnClickedPsaFlareButton()
{
	BW_GUARD;

	if (flareNameSelection_.GetCurSel() != -1)
	{
		CString curSelection;
		flareNameSelection_.GetWindowText( curSelection );
		if (curSelection == Localise( L"PARTICLEEDITOR/GUI/PSA_FLARE_PROPERTIES/NO_FLARES_FOUND" ))
		{
			flareNameSelection_.SetCurSel( -1 );
		}
		else
		{
			CopyDataToPSA();
		}
	}
}

void PsaFlareProperties::OnCbnSelchangeFlarename()
{
	BW_GUARD;

	SetParameters(SET_PSA);
}

void PsaFlareProperties::OnBnClickedPsaFlareFlarenameDirectory()
{
	BW_GUARD;

	DirDialog dlg; 

	dlg.windowTitle_ = Localise(L"PARTICLEEDITOR/OPEN");
	dlg.promptText_ = Localise(L"PARTICLEEDITOR/CHOOSE_DIR");
	dlg.fakeRootDirectory_ = dlg.basePath();

	CString startDir;
	flareNameDirectoryEdit_.GetWindowText(startDir);
    if (startDir != c_flarePropertiesdefaultDirectoryText)
		dlg.startDirectory_ = BWResource::resolveFilenameW( startDir.GetString() ).c_str();

	if (dlg.doBrowse( AfxGetApp()->m_pMainWnd )) 
	{
		dlg.userSelectedDirectory_ += "/";
		BW::wstring relativeDirectory = BWResource::dissolveFilenameW(dlg.userSelectedDirectory_.GetString());
		flareNameDirectoryEdit_.SetWindowText(relativeDirectory.c_str());

		PopulateComboBoxWithFilenames
        (
            flareNameSelection_, 
            bw_wtoutf8( relativeDirectory ), 
            validFlareFilename
        );
		finishPopulatingFlareNames();
		flareNameSelection_.SetCurSel(-1);
	}
}


void PsaFlareProperties::finishPopulatingFlareNames()
{
	BW_GUARD;

	if (flareNameSelection_.GetCount() == 0)
	{
		flareNameSelection_.AddString( Localise( L"PARTICLEEDITOR/GUI/PSA_FLARE_PROPERTIES/NO_FLARES_FOUND" ) );
	}
}


FlarePSA *	PsaFlareProperties::action()
{
	return static_cast< FlarePSA * >(action_.get());
}

BW_END_NAMESPACE

