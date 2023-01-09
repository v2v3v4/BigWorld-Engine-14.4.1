#include "pch.hpp"
#include "worldeditor/gui/dialogs/placement_ctrls_dlg.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "cstdmf/debug.hpp"
#include "controls/user_messages.hpp"


DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

static const int PLACECTRL_ROTATE_DIGITS = 0;
static const int PLACECTRL_SCALE_DIGITS = 2;
static const int PLACECTRL_MIN_WIDTH = 220;


// SliderContainer
PlacementCtrlsDlg::SliderContainer::SliderContainer(
    controls::RangeSliderCtrl* slider, controls::EditNumeric* minEd, 
    controls::EditNumeric* maxEd, controls::ThemedImageButton* reset,
	int group )
	: slider_( slider )
	, minEd_( minEd )
	, maxEd_( maxEd )
	, reset_( reset )
	, group_( group )
{
};


// PlacementCtrlsDlg dialog

// GUITABS content ID ( declared by the IMPLEMENT_DIALOG_CONTENT macro )
const BW::wstring PlacementCtrlsDlg::contentID = L"PlacementCtrlsDlg";

PlacementCtrlsDlg::PlacementCtrlsDlg(CWnd* pParent)
	: CDialog(PlacementCtrlsDlg::IDD, pParent)
	, GUI::ActionMaker<PlacementCtrlsDlg>(
		"newPlacementPreset", &PlacementCtrlsDlg::newPreset )
	, GUI::ActionMaker<PlacementCtrlsDlg, 1>(
		"delPlacementPreset", &PlacementCtrlsDlg::delPreset )
	, GUI::UpdaterMaker<PlacementCtrlsDlg>(
		"presetEditableUpdater", &PlacementCtrlsDlg::presetEditableUpdater )
	, GUI::ActionMaker<PlacementCtrlsDlg, 2>(
		"renamePlacementPreset", &PlacementCtrlsDlg::renamePreset )
	, updateEdits_( true )
	, linkIcon_( NULL )
	, resetIcon_( NULL )
{
}

PlacementCtrlsDlg::~PlacementCtrlsDlg()
{
	BW_GUARD;

	::DestroyIcon(linkIcon_); 
	linkIcon_ = NULL;

	::DestroyIcon(resetIcon_); 
	resetIcon_ = NULL;
}

void PlacementCtrlsDlg::loadPreset()
{
	BW_GUARD;

	DataSectionPtr preset =
		PlacementPresets::instance()->getCurrentPresetData();
	if ( !preset )
	{
		linkRandRot_.SetCheck( BST_UNCHECKED );
		linkRandSca_.SetCheck( BST_UNCHECKED );
		uniformSca_.SetCheck( BST_UNCHECKED );
		updateLinks();
		for( int j = 0; j < (int)sliders_.size(); ++j )
			setDefaults( sliders_[j] );
		return;
	}

	if ( preset->readBool( "linkRotation" ) )
		linkRandRot_.SetCheck( BST_CHECKED );
	else
		linkRandRot_.SetCheck( BST_UNCHECKED );

	if ( preset->readBool( "linkScale" ) )
		linkRandSca_.SetCheck( BST_CHECKED );
	else
		linkRandSca_.SetCheck( BST_UNCHECKED );

	updateLinks();

	if ( preset->readBool( "uniformScale" ) )
		uniformSca_.SetCheck( BST_CHECKED );
	else
		uniformSca_.SetCheck( BST_UNCHECKED );

	CString name;
	wchar_t axis[3] = { L'X', L'Y', L'Z' };
	for( int j = 0; j < (int)sliders_.size(); ++j )
	{
		name.Format( L"min%s%c",
			sliders_[j]->group_ == GROUP_ROTATION ? L"Rot" : L"Sca",
			axis[ j % 3 ] );
		float minVal = preset->readFloat( bw_wtoutf8( name.GetString() ) );

		name.Format( L"max%s%c",
			sliders_[j]->group_ == GROUP_ROTATION ? L"Rot" : L"Sca",
			axis[ j % 3 ] );
		float maxVal = preset->readFloat( bw_wtoutf8( name.GetString() ) );
		
		updateEdits_ = false;
		sliders_[j]->minEd_->SetValue( minVal );
		sliders_[j]->maxEd_->SetValue( maxVal );

		sliders_[j]->slider_->setThumbValues( minVal , maxVal );
		updateEdits_ = true;
	}
}

void PlacementCtrlsDlg::savePreset()
{
	BW_GUARD;

	DataSectionPtr preset =
		PlacementPresets::instance()->getCurrentPresetData();
	if ( !preset )
		return;

	preset->writeBool( "linkRotation", linkRandRot_.GetCheck() == BST_CHECKED );
	preset->writeBool( "linkScale", linkRandSca_.GetCheck() == BST_CHECKED );
	preset->writeBool( "uniformScale", uniformSca_.GetCheck() == BST_CHECKED );
	CString name;
	wchar_t axis[3] = { L'X', L'Y', L'Z' };
	for( int j = 0; j < (int)sliders_.size(); ++j )
	{
		name.Format( L"min%s%c",
			sliders_[j]->group_ == GROUP_ROTATION ? L"Rot" : L"Sca",
			axis[ j % 3 ] );
		preset->writeFloat( bw_wtoutf8( name.GetString() ), sliders_[j]->minEd_->GetValue() );

		name.Format( L"max%s%c",
			sliders_[j]->group_ == GROUP_ROTATION ? L"Rot" : L"Sca",
			axis[ j % 3 ] );
		preset->writeFloat( bw_wtoutf8( name.GetString() ), sliders_[j]->maxEd_->GetValue() );
	}
	PlacementPresets::instance()->save();
}

bool PlacementCtrlsDlg::newPreset( GUI::ItemPtr item )
{
	BW_GUARD;

	newPlacementDlg_.newName_ = "";
	BW::vector<BW::wstring> existingNames;
	PlacementPresets::instance()->presetNames(existingNames);
	newPlacementDlg_.existingNames(existingNames);
	if ( newPlacementDlg_.DoModal() != IDOK ||
		newPlacementDlg_.newName_.IsEmpty() )
		return true;

	DataSectionPtr presetsSection = PlacementPresets::instance()->presetsSection();
	if ( !presetsSection )
		return false;

	DataSectionPtr preset = presetsSection->newSection( "preset" );
	if ( !preset )
		return false;

	BW::string name = PlacementPresets::instance()->getNewPresetName();
	preset->writeString( "name", name );
	preset->writeString( "displayName", bw_wtoutf8( newPlacementDlg_.newName_.GetString() ) );

	PlacementPresets::instance()->readPresets();
	PlacementPresets::instance()->currentPreset( name );
	for( int j = 0; j < (int)sliders_.size(); ++j )
		setDefaults( sliders_[j] );

	linkRandRot_.SetCheck( BST_UNCHECKED );
	linkRandSca_.SetCheck( BST_CHECKED );
	uniformSca_.SetCheck( BST_CHECKED );
	updateLinks();

	savePreset();
	updateVisibility();
	GUI::Manager::instance().update();

	return true;
}

bool PlacementCtrlsDlg::delPreset( GUI::ItemPtr item )
{
	BW_GUARD;

	if ( MessageBox( Localise(L"WORLDEDITOR/GUI/PLACEMENT_CTRLS_DLG/DELETE_PRESET_TEXT"),
		Localise(L"WORLDEDITOR/GUI/PLACEMENT_CTRLS_DLG/DELETE_PRESET_TITLE"),
		MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 ) != IDYES )
		return false;

	PlacementPresets::instance()->deleteCurrentPreset();

	PlacementPresets::instance()->readPresets();
	PlacementPresets::instance()->currentPreset(
		PlacementPresets::instance()->defaultPreset() );
	loadPreset();
	updateVisibility();
	GUI::Manager::instance().update();

	return true;
}

unsigned int PlacementCtrlsDlg::presetEditableUpdater( GUI::ItemPtr item )
{
	BW_GUARD;

	if ( PlacementPresets::instance()->defaultPresetCurrent() ||
		PlacementPresets::instance()->currentPresetStock() )
		return 0;
	else
		return 1;
}

bool PlacementCtrlsDlg::renamePreset( GUI::ItemPtr item )
{
	BW_GUARD;

	DataSectionPtr preset = PlacementPresets::instance()->getCurrentPresetData();
	if ( !preset )
		return false;

	newPlacementDlg_.newName_ = preset->readString( "displayName" ).c_str();
	BW::vector<BW::wstring> existingNames;
	PlacementPresets::instance()->presetNames(existingNames);
	newPlacementDlg_.existingNames(existingNames);
	if ( newPlacementDlg_.DoModal() != IDOK ||
		newPlacementDlg_.newName_.IsEmpty() )
		return true;

	preset->writeString( "displayName", bw_wtoutf8( newPlacementDlg_.newName_.GetString() ) );
	PlacementPresets::instance()->save();

	BW::string name = PlacementPresets::instance()->currentPreset();
	PlacementPresets::instance()->readPresets();
	PlacementPresets::instance()->currentPreset( name );
	GUI::Manager::instance().update();

	return true;
}

void PlacementCtrlsDlg::currentPresetChanged( const BW::string& presetName )
{
	BW_GUARD;

	loadPreset();
	updateVisibility();
}

PlacementCtrlsDlg::SliderContainerPtr PlacementCtrlsDlg::find( void* ptr )
{
	BW_GUARD;

	for ( BW::vector<SliderContainerPtr>::iterator i = sliders_.begin();
		i != sliders_.end(); ++i )
	{
		if ( (*i)->slider_ == (controls::RangeSliderCtrl*)ptr ||
            (*i)->minEd_ == (controls::EditNumeric*)ptr ||
			(*i)->maxEd_ == (controls::EditNumeric*)ptr )
			return (*i);
	}

	return 0;
}

BOOL PlacementCtrlsDlg::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	INIT_AUTO_TOOLTIP();

	// fill slider vector with slider/edit/button pointers
	sliders_.push_back( new SliderContainer(
		&sliderRandRotX_, &minRandRotX_, &maxRandRotX_, &resetRotX_, GROUP_ROTATION ) );
	sliders_.push_back( new SliderContainer(
		&sliderRandRotY_, &minRandRotY_, &maxRandRotY_, &resetRotY_, GROUP_ROTATION ) );
	sliders_.push_back( new SliderContainer(
		&sliderRandRotZ_, &minRandRotZ_, &maxRandRotZ_, &resetRotZ_, GROUP_ROTATION ) );

	sliders_.push_back( new SliderContainer(
		&sliderRandScaX_, &minRandScaX_, &maxRandScaX_, &resetScaX_, GROUP_SCALE ) );
	sliders_.push_back( new SliderContainer(
		&sliderRandScaY_, &minRandScaY_, &maxRandScaY_, &resetScaY_, GROUP_SCALE ) );
	sliders_.push_back( new SliderContainer(
		&sliderRandScaZ_, &minRandScaZ_, &maxRandScaZ_, &resetScaZ_, GROUP_SCALE ) );

	PlacementPresets::instance()->dialog( this );

	CRect rect;
	GetClientRect( &rect );

	CRect ctrlRect;

	maxRandRotX_.GetWindowRect( &ctrlRect );
	ScreenToClient( &ctrlRect );
	rightEditDist_ = rect.right - ctrlRect.left;

	resetRotX_.GetWindowRect( &ctrlRect );
	ScreenToClient( &ctrlRect );
	rightResetButDist_ = rect.right - ctrlRect.left;

	DataSectionPtr conf = PlacementPresets::instance()->rootSection();
	ASSERT( !!conf );

	float rotMin = conf->readFloat( "rotateRangeMin", -360 );
	float rotMax = conf->readFloat( "rotateRangeMax", 360 );

	float exponent = conf->readFloat( "scaleSliderExponent", 3.2f );
	// some tricks here, to overcome precision problems that occur when
	// converting from double to float
	float scaMin = float( conf->readDouble( "scaleRangeMin", 0.01 )
		+ 0.000000001 );
	float scaMax = float( conf->readDouble( "scaleRangeMax", 10.0 )
		+ 0.000000001 );

	// initialise slider-related controls, using the slider vector
	linkIcon_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PLACEMENT_LINK ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
	linkRandRot_.SetIcon( linkIcon_ );
	linkRandSca_.SetIcon( linkIcon_ );

	resetIcon_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PLACEMENT_RESET ),
			IMAGE_ICON, 12, 12, LR_DEFAULTCOLOR );

	for ( BW::vector<SliderContainerPtr>::iterator i = sliders_.begin();
		i != sliders_.end(); ++i )
	{
		if ( (*i)->group_ == GROUP_ROTATION )
		{
			(*i)->slider_->setRange( rotMin, rotMax, 0 );
			(*i)->slider_->SetLineSize( 1 );
            (*i)->minEd_->SetNumericType( controls::EditNumeric::ENT_INTEGER );
			(*i)->minEd_->SetMinimum( rotMin );
			(*i)->minEd_->SetMaximum( rotMax );
			(*i)->minEd_->SetAllowNegative( true );
			(*i)->maxEd_->SetNumericType( controls::EditNumeric::ENT_INTEGER );
			(*i)->maxEd_->SetMinimum( rotMin );
			(*i)->maxEd_->SetMaximum( rotMax );
			(*i)->maxEd_->SetAllowNegative( true );
		}
		else
		{
			(*i)->slider_->setExponent( exponent );
			(*i)->slider_->setRange( scaMin, scaMax, PLACECTRL_SCALE_DIGITS );
			(*i)->slider_->SetLineSize( 3 );
			(*i)->minEd_->SetNumericType( controls::EditNumeric::ENT_FLOAT );
			(*i)->minEd_->SetMinimum( scaMin );
			(*i)->minEd_->SetMaximum( scaMax );
			(*i)->minEd_->SetAllowNegative( false );
			(*i)->maxEd_->SetNumericType( controls::EditNumeric::ENT_FLOAT );
			(*i)->maxEd_->SetMinimum( scaMin );
			(*i)->maxEd_->SetMaximum( scaMax );
			(*i)->minEd_->SetAllowNegative( false );
		}
		(*i)->reset_->SetIcon( resetIcon_ );
	}

	// toolbar
	DataSectionPtr section = PlacementPresets::instance()->rootSection();
	DataSectionPtr tbSection = 0;
	if ( !!section )
		tbSection = section->openSection( "Toolbar" );
	else
		ERROR_MSG( "Cannot find random placement controls file '%s'",
			PlacementPresets::instance()->xmlFilePath() );

	if ( tbSection )
	{
		for( int i = 0; i < tbSection->countChildren(); ++i )
			GUI::Manager::instance().add(
				new GUI::Item( tbSection->openChild( i ) ) );

		toolbar_.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
			TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS |
			CBRS_TOOLTIPS, CRect(0,0,1,1), this, 0 );

		CToolTipCtrl* tc = toolbar_.GetToolTips();
		if ( tc )
			tc->SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

		GUI::Toolbar* guiTB = new GUI::Toolbar( "PlacementToolbar", toolbar_ );
		GUI::Manager::instance().add( guiTB );

		CRect rect;
		presetsLabel_.GetWindowRect( &rect );
		ScreenToClient( rect );

		SIZE tbSize = guiTB->minimumSize();
		toolbar_.SetWindowPos( 0, rect.right + 5, rect.top - 5,
			tbSize.cx, tbSize.cy, SWP_NOZORDER );
	}
	else
		ERROR_MSG( "Cannot find toolbar in the random placement file '%s'",
			PlacementPresets::instance()->xmlFilePath() );

	// setup and go
	PlacementPresets::instance()->readPresets();
	loadPreset();
	updateVisibility();

	return TRUE;
}

void PlacementCtrlsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RANDROT_GRP, randRotGroup_);
	DDX_Control(pDX, IDC_RANDSCA_GRP, randScaGroup_);
	DDX_Control(pDX, IDC_RANDPRESETSLABEL, presetsLabel_);
	DDX_Control(pDX, IDC_RANDROT_LINK, linkRandRot_);
	DDX_Control(pDX, IDC_RANDROT_LINKUP, linkRotUpIcon_);
	DDX_Control(pDX, IDC_RANDROT_LINKDOWN, linkRotDownIcon_);
	DDX_Control(pDX, IDC_RANDSCA_LINK, linkRandSca_);
	DDX_Control(pDX, IDC_RANDSCA_LINKUP, linkScaUpIcon_);
	DDX_Control(pDX, IDC_RANDSCA_LINKDOWN, linkScaDownIcon_);
	DDX_Control(pDX, IDC_RANDROTX_RANGE, sliderRandRotX_);
	DDX_Control(pDX, IDC_RANDROTY_RANGE, sliderRandRotY_);
	DDX_Control(pDX, IDC_RANDROTZ_RANGE, sliderRandRotZ_);
	DDX_Control(pDX, IDC_RANDSCAX_RANGE, sliderRandScaX_);
	DDX_Control(pDX, IDC_RANDSCAY_RANGE, sliderRandScaY_);
	DDX_Control(pDX, IDC_RANDSCAZ_RANGE, sliderRandScaZ_);
	DDX_Control(pDX, IDC_RANDROTX_MIN, minRandRotX_);
	DDX_Control(pDX, IDC_RANDROTX_MAX, maxRandRotX_);
	DDX_Control(pDX, IDC_RANDROTY_MIN, minRandRotY_);
	DDX_Control(pDX, IDC_RANDROTY_MAX, maxRandRotY_);
	DDX_Control(pDX, IDC_RANDROTZ_MIN, minRandRotZ_);
	DDX_Control(pDX, IDC_RANDROTZ_MAX, maxRandRotZ_);
	DDX_Control(pDX, IDC_RANDSCAX_MIN, minRandScaX_);
	DDX_Control(pDX, IDC_RANDSCAX_MAX, maxRandScaX_);
	DDX_Control(pDX, IDC_RANDSCAY_MIN, minRandScaY_);
	DDX_Control(pDX, IDC_RANDSCAY_MAX, maxRandScaY_);
	DDX_Control(pDX, IDC_RANDSCAZ_MIN, minRandScaZ_);
	DDX_Control(pDX, IDC_RANDSCAZ_MAX, maxRandScaZ_);
	DDX_Control(pDX, IDC_RANDROTX_RESET, resetRotX_);
	DDX_Control(pDX, IDC_RANDROTY_RESET, resetRotY_);
	DDX_Control(pDX, IDC_RANDROTZ_RESET, resetRotZ_);
	DDX_Control(pDX, IDC_RANDSCAX_RESET, resetScaX_);
	DDX_Control(pDX, IDC_RANDSCAY_RESET, resetScaY_);
	DDX_Control(pDX, IDC_RANDSCAZ_RESET, resetScaZ_);
	DDX_Control(pDX, IDC_RANDSCA_UNI, uniformSca_);
}

void PlacementCtrlsDlg::setDefaults( SliderContainerPtr slider )
{
	BW_GUARD;

	DataSectionPtr conf = PlacementPresets::instance()->rootSection();
	ASSERT( !!conf );

	float val;
	if ( slider->group_ == GROUP_ROTATION )
		val = conf->readFloat( "rotateCentre", 0.0f );
	else
		val = conf->readFloat( "scaleCentre", 1.0f );

	slider->slider_->setThumbValues( val, val );
	slider->slider_->changed();
}

void PlacementCtrlsDlg::writeEditNum( controls::EditNumeric* edit, float num, int group )
{
	BW_GUARD;

	SliderContainerPtr slider = find( edit );
	if ( !slider )
		return;

	edit->SetValue( num );
}

void PlacementCtrlsDlg::sliderFromEdits( SliderContainerPtr slider )
{
	BW_GUARD;

	if ( !updateEdits_ )
		return;

	static CWnd* s_lastFocus = GetFocus();

	CString str;
	
	float min = slider->minEd_->GetValue();
	float max = slider->maxEd_->GetValue();
	
	float oldmin = 0.f;
	float oldmax = 0.f;
	slider->slider_->getThumbValues( oldmin, oldmax );

	if ( (oldmin != min || oldmax != max) && max < min )
	{
		std::swap(min, max);
		updateEdits_ = false;
		if ((GetFocus() != slider->minEd_ && s_lastFocus == slider->minEd_ ) ||
				(GetFocus() != slider->maxEd_ && s_lastFocus == slider->maxEd_ ))
		{
			writeEditNum( slider->minEd_, min, slider->group_ );
			writeEditNum( slider->maxEd_, max, slider->group_ );
		}

		updateEdits_ = true;
	}
	s_lastFocus = GetFocus();
	slider->slider_->setThumbValues( min, max );
	syncBars( slider );
}

void PlacementCtrlsDlg::syncBars( SliderContainerPtr slider )
{
	BW_GUARD;

	if ( ( slider->group_ == GROUP_ROTATION &&
		linkRandRot_.GetCheck() == BST_CHECKED ) ||
		( slider->group_ == GROUP_SCALE &&
		linkRandSca_.GetCheck() == BST_CHECKED ) )
	{
		float min = slider->minEd_->GetValue();
		float max = slider->maxEd_->GetValue();
		for ( BW::vector<SliderContainerPtr>::iterator i = sliders_.begin();
			i != sliders_.end(); ++i )
		{
			if ( (*i)->group_ == slider->group_ &&
				(*i) != slider ) 
			{
				if ( max < min )
				{
					std::swap(min, max);
				}
				(*i)->slider_->setThumbValues( min, max );
				updateEdits_ = false;
				writeEditNum( (*i)->minEd_, min, (*i)->group_ );
				writeEditNum( (*i)->maxEd_, max, (*i)->group_ );
				updateEdits_ = true;
			}
		}
	}
	savePreset();
}

void PlacementCtrlsDlg::updateVisibility()
{
	BW_GUARD;

	BOOL enable = FALSE;
	if ( !PlacementPresets::instance()->defaultPresetCurrent() )
		enable = TRUE;

	for ( BW::vector<SliderContainerPtr>::iterator i = sliders_.begin();
		i != sliders_.end(); ++i )
	{
		(*i)->slider_->EnableWindow( enable );
		(*i)->minEd_->EnableWindow( enable );
		(*i)->maxEd_->EnableWindow( enable );
		(*i)->reset_->EnableWindow( enable );
	}
	linkRandRot_.EnableWindow( enable );
	linkRandSca_.EnableWindow( enable && uniformSca_.GetCheck() == BST_UNCHECKED );
	uniformSca_.EnableWindow( enable );
}

// Messages

BEGIN_MESSAGE_MAP(PlacementCtrlsDlg, CDialog)
	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_MESSAGE( WM_RANGESLIDER_CHANGED, OnRangeSliderChanged )
	ON_MESSAGE( WM_RANGESLIDER_TRACK, OnRangeSliderTrack )
	ON_EN_CHANGE(IDC_RANDROTX_MIN, OnEnChangeRandRotX)
	ON_EN_CHANGE(IDC_RANDROTX_MAX, OnEnChangeRandRotX)
	ON_EN_CHANGE(IDC_RANDROTY_MIN, OnEnChangeRandRotY)
	ON_EN_CHANGE(IDC_RANDROTY_MAX, OnEnChangeRandRotY)
	ON_EN_CHANGE(IDC_RANDROTZ_MIN, OnEnChangeRandRotZ)
	ON_EN_CHANGE(IDC_RANDROTZ_MAX, OnEnChangeRandRotZ)
	ON_EN_CHANGE(IDC_RANDSCAX_MIN, OnEnChangeRandScaX)
	ON_EN_CHANGE(IDC_RANDSCAX_MAX, OnEnChangeRandScaX)
	ON_EN_CHANGE(IDC_RANDSCAY_MIN, OnEnChangeRandScaY)
	ON_EN_CHANGE(IDC_RANDSCAY_MAX, OnEnChangeRandScaY)
	ON_EN_CHANGE(IDC_RANDSCAZ_MIN, OnEnChangeRandScaZ)
	ON_EN_CHANGE(IDC_RANDSCAZ_MAX, OnEnChangeRandScaZ)
	ON_BN_CLICKED(IDC_RANDROTX_RESET, OnBnClickedRandRotXReset)
	ON_BN_CLICKED(IDC_RANDROTY_RESET, OnBnClickedRandRotYReset)
	ON_BN_CLICKED(IDC_RANDROTZ_RESET, OnBnClickedRandRotZReset)
	ON_BN_CLICKED(IDC_RANDSCAX_RESET, OnBnClickedRandScaXReset)
	ON_BN_CLICKED(IDC_RANDSCAY_RESET, OnBnClickedRandScaYReset)
	ON_BN_CLICKED(IDC_RANDSCAZ_RESET, OnBnClickedRandScaZReset)
	ON_BN_CLICKED(IDC_RANDROT_LINK, OnBnClickedRandrotLink)
	ON_BN_CLICKED(IDC_RANDSCA_LINK, OnBnClickedRandscaLink)
	ON_BN_CLICKED(IDC_RANDSCA_UNI, OnBnClickedRandscaUni)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// PlacementCtrlsDlg message handlers

void PlacementCtrlsDlg::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	GUI::Manager::instance().act( nID );
}

LRESULT PlacementCtrlsDlg::OnRangeSliderChanged(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	return OnRangeSliderTrack( wParam, lParam );
}

LRESULT PlacementCtrlsDlg::OnRangeSliderTrack(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	SliderContainerPtr slider = find( (void*)lParam );

	if ( !slider )
		return 0; // should never do this

	updateEdits_ = false;

	float min = 0;
	float max = 0;
	slider->slider_->getThumbValues( min, max );

	writeEditNum( slider->minEd_, min, slider->group_ );
	writeEditNum( slider->maxEd_, max, slider->group_ );

	updateEdits_ = true;

	syncBars( slider );
	return 0;
}

void PlacementCtrlsDlg::updateLinks()
{
	BW_GUARD;

	int show = linkRandRot_.GetCheck() == BST_CHECKED?SW_SHOW:SW_HIDE;
	linkRotUpIcon_.ShowWindow( show );
	linkRotDownIcon_.ShowWindow( show );
	show = linkRandSca_.GetCheck() == BST_CHECKED?SW_SHOW:SW_HIDE;
	linkScaUpIcon_.ShowWindow( show );
	linkScaDownIcon_.ShowWindow( show );
}

void PlacementCtrlsDlg::OnBnClickedRandrotLink()
{
	BW_GUARD;

	updateLinks();
	syncBars( sliders_[0] );
}

void PlacementCtrlsDlg::OnBnClickedRandscaLink()
{
	BW_GUARD;

	updateLinks();
	syncBars( sliders_[3] );
}

void PlacementCtrlsDlg::OnBnClickedRandscaUni()
{
	BW_GUARD;

	if ( uniformSca_.GetCheck() == BST_CHECKED )
	{
		linkRandSca_.SetCheck( BST_CHECKED );
		OnBnClickedRandscaLink();
	}
	linkRandSca_.EnableWindow( uniformSca_.IsWindowEnabled() &&
		uniformSca_.GetCheck() == BST_UNCHECKED );
	savePreset();
}

void PlacementCtrlsDlg::OnEnChangeRandRotX()
{
	BW_GUARD;

	sliderFromEdits( sliders_[0] );
}

void PlacementCtrlsDlg::OnEnChangeRandRotY()
{
	BW_GUARD;

	sliderFromEdits( sliders_[1] );
}

void PlacementCtrlsDlg::OnEnChangeRandRotZ()
{
	BW_GUARD;

	sliderFromEdits( sliders_[2] );
}

void PlacementCtrlsDlg::OnEnChangeRandScaX()
{
	BW_GUARD;

	sliderFromEdits( sliders_[3] );
}

void PlacementCtrlsDlg::OnEnChangeRandScaY()
{
	BW_GUARD;

	sliderFromEdits( sliders_[4] );
}

void PlacementCtrlsDlg::OnEnChangeRandScaZ()
{
	BW_GUARD;

	sliderFromEdits( sliders_[5] );
}

void PlacementCtrlsDlg::OnBnClickedRandRotXReset()
{
	BW_GUARD;

	setDefaults( sliders_[0] );
	savePreset();
}

void PlacementCtrlsDlg::OnBnClickedRandRotYReset()
{
	BW_GUARD;

	setDefaults( sliders_[1] );
	savePreset();
}

void PlacementCtrlsDlg::OnBnClickedRandRotZReset()
{
	BW_GUARD;

	setDefaults( sliders_[2] );
	savePreset();
}

void PlacementCtrlsDlg::OnBnClickedRandScaXReset()
{
	BW_GUARD;

	setDefaults( sliders_[3] );
	savePreset();
}

void PlacementCtrlsDlg::OnBnClickedRandScaYReset()
{
	BW_GUARD;

	setDefaults( sliders_[4] );
	savePreset();
}

void PlacementCtrlsDlg::OnBnClickedRandScaZReset()
{
	BW_GUARD;

	setDefaults( sliders_[5] );
	savePreset();
}

void PlacementCtrlsDlg::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CDialog::OnSize( nType, cx, cy );

	if ( sliders_.empty() )
		return;

	CRect rect;
	GetClientRect( &rect );

	if ( rect.right < PLACECTRL_MIN_WIDTH )
		rect.right = PLACECTRL_MIN_WIDTH;

	CRect ctrlRect;
	randRotGroup_.GetWindowRect( &ctrlRect );
	ScreenToClient( &ctrlRect );
	randRotGroup_.SetWindowPos( 0, 0, 0,
		rect.Width(), ctrlRect.Height(), SWP_NOMOVE | SWP_NOZORDER );
	randScaGroup_.GetWindowRect( &ctrlRect );
	ScreenToClient( &ctrlRect );
	randScaGroup_.SetWindowPos( 0, 0, 0,
		rect.Width(), ctrlRect.Height(), SWP_NOMOVE | SWP_NOZORDER );

	for ( BW::vector<SliderContainerPtr>::iterator i = sliders_.begin();
		i != sliders_.end(); ++i )
	{
		(*i)->maxEd_->GetWindowRect( &ctrlRect );
		ScreenToClient( &ctrlRect );
		int oldEdPos = ctrlRect.left;
		int maxEdPos = rect.right - rightEditDist_;
		(*i)->maxEd_->SetWindowPos( 0,
			maxEdPos, ctrlRect.top,
			0, 0, SWP_NOSIZE | SWP_NOZORDER );

		(*i)->reset_->GetWindowRect( &ctrlRect );
		ScreenToClient( &ctrlRect );
		(*i)->reset_->SetWindowPos( 0,
			rect.right - rightResetButDist_, ctrlRect.top,
			0, 0, SWP_NOSIZE | SWP_NOZORDER );

		(*i)->slider_->GetWindowRect( &ctrlRect );
		ScreenToClient( &ctrlRect );
		(*i)->slider_->SetWindowPos( 0, 0, 0,
			(maxEdPos - oldEdPos) + ctrlRect.Width(),
			ctrlRect.Height(), SWP_NOMOVE | SWP_NOZORDER );
	}		
}

afx_msg HBRUSH PlacementCtrlsDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	for ( BW::vector<SliderContainerPtr>::iterator i = sliders_.begin();
		i != sliders_.end(); ++i )
	{
		(*i)->minEd_->SetBoundsColour( pDC, pWnd,
			(*i)->minEd_->GetMinimum(), (*i)->maxEd_->GetMaximum());

		(*i)->maxEd_->SetBoundsColour( pDC, pWnd, 
				(*i)->minEd_->GetMinimum(), (*i)->maxEd_->GetMaximum() );
	}

	return brush;
}
BW_END_NAMESPACE

