#include "pch.hpp"

#include "tools/common/python_adapter.hpp"
#include "tools/common/utilities.hpp"

#include "main_frm.h"
#include "me_shell.hpp"
#include "me_app.hpp"

#include "common/string_utils.hpp"
#include "common/user_messages.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"

#include "controls/user_messages.hpp"
#include "controls/edit_commit.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/range_slider_ctrl.hpp"
#include "controls/slider.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"

#include "entitydef/constants.hpp"

#include "undo_redo.hpp"

#include "choose_anim.hpp"
#include "trigger_list.hpp"

#include "page_actions.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// SliderContainer
SliderContainer::SliderContainer(
	controls::RangeSliderCtrl* slider, controls::EditNumeric* minEd, controls::EditNumeric* maxEd, int group /* = 0 */ )
	: slider_( slider )
	, minEd_( minEd )
	, maxEd_( maxEd )
	, group_( group )
{}

struct PageActionsImpl: public SafeReferenceCount
{
	static PageActions* s_currPage;

	DataSectionPtr defaultCapsData;
	
	BW::vector< DataSectionPtr > capsList;

	bool inited;
	bool ready;
	bool updating;
	bool selectClicked;
	bool MRUClicked;
	
	int updateCount;

	int lastLockedParents;

	HTREEITEM lastItem;
	HTREEITEM lastMRU;

	BW::string modelName;

	BW::vector<SliderContainerPtr> sliders;

	CTreeCtrl mru;

	CToolBarCtrl toolbar;
	controls::EditCommit name;
	CEdit anim;
	CButton change_anim;

	controls::EditNumeric blendIn;
	controls::Slider blendInSlider;
	controls::EditNumeric blendOut;
	controls::Slider blendOutSlider;

	CButton looped;
	CButton move;
	CButton impact;
	CButton coord;
	controls::EditCommit track;
	CButton ownTrack;

	bool allowEditUpdates;
		
	CStatic match_box;

	CStatic trigger_box;

	CButton		trigger_falling;
	controls::RangeSliderCtrl trigger_speed_sldr;
	controls::EditNumeric trigger_speed_min;
	controls::EditNumeric trigger_speed_max;
	controls::RangeSliderCtrl trigger_aux1_sldr;
	controls::EditNumeric trigger_aux1_min;
	controls::EditNumeric trigger_aux1_max;
	controls::RangeSliderCtrl trigger_yaw_sldr;
	controls::EditNumeric trigger_yaw_min;
	controls::EditNumeric trigger_yaw_max;
	CEdit		trigger_caps_on;
	CButton		trigger_caps_on_sel;
	CEdit		trigger_caps_off;
	CButton		trigger_caps_off_sel;

	CStatic cancel_box;

	CButton		cancel_falling;
	controls::RangeSliderCtrl cancel_speed_sldr;
	controls::EditNumeric cancel_speed_min;
	controls::EditNumeric cancel_speed_max;
	controls::RangeSliderCtrl cancel_aux1_sldr;
	controls::EditNumeric cancel_aux1_min;
	controls::EditNumeric cancel_aux1_max;
	controls::RangeSliderCtrl cancel_yaw_sldr;
	controls::EditNumeric cancel_yaw_min;
	controls::EditNumeric cancel_yaw_max;
	CEdit		cancel_caps_on;
	CButton		cancel_caps_on_sel;
	CEdit		cancel_caps_off;
	CButton		cancel_caps_off_sel;

	CButton one_shot;
	CButton scale_playback_speed;
	CButton feet_follow_direction;
};

PageActions* PageActionsImpl::s_currPage = NULL;

// PageActions

//ID string required for the tearoff tab manager
const BW::wstring PageActions::contentID = L"PageActionsID";

IMPLEMENT_DYNCREATE(PageActions, TreeList)

// A helper function to setup the match fields
void PageActions::initMatcherField( controls::EditNumeric& field, int dec /* = 0 */, float min /* = 0.f */, float max /* = 100.f */, bool allowNeg /* = true */ )
{
	BW_GUARD;

	field.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	field.SetNumDecimals ( dec );
	field.SetMinimum( min, true );
	field.SetMaximum( max, true );
	field.SetAllowNegative( allowNeg );
	field.SetAllowEmpty( false );
}

PageActions::PageActions()
	: TreeList( PageActions::IDD, mutant()->actTree(), "actions" )
{
	BW_GUARD;

	pImpl_ = new PageActionsImpl;
	
	pImpl_->inited = false;
	pImpl_->ready = false;
	pImpl_->updating = false;

	pImpl_->updateCount = -1;
	pImpl_->lastLockedParents = -1;

	pImpl_->lastItem = NULL;
	pImpl_->lastMRU = NULL;

	pImpl_->modelName = "";

	pImpl_->track.autoSelect( true );

	pImpl_->allowEditUpdates = true;
	pImpl_->selectClicked = false;
	pImpl_->MRUClicked = false;

	pImpl_->blendIn.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	pImpl_->blendIn.SetMinimum( 0.f );
	pImpl_->blendIn.SetMaximum( 1.f );
	pImpl_->blendIn.SetAllowNegative( false );

	pImpl_->blendOut.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	pImpl_->blendOut.SetMinimum( 0.f );
	pImpl_->blendOut.SetMaximum( 1.f );
	pImpl_->blendOut.SetAllowNegative( false );
	
	initMatcherField( pImpl_->trigger_speed_min, 1, 0, 100.f );
	initMatcherField( pImpl_->trigger_speed_max, 1, 0, 100.f );
	initMatcherField( pImpl_->trigger_aux1_min, 1, -360.f, 360.f );
	initMatcherField( pImpl_->trigger_aux1_max, 1, -360.f, 360.f );
	initMatcherField( pImpl_->trigger_yaw_min, 1, -360.f, 360.f );
	initMatcherField( pImpl_->trigger_yaw_max, 1, -360.f, 360.f );

	pImpl_->sliders.push_back( new SliderContainer(
		&pImpl_->trigger_speed_sldr,
		&pImpl_->trigger_speed_min,
		&pImpl_->trigger_speed_max,
		0 ) );

	pImpl_->sliders.push_back( new SliderContainer(
		&pImpl_->trigger_aux1_sldr,
		&pImpl_->trigger_aux1_min,
		&pImpl_->trigger_aux1_max,
		0 ) );

	pImpl_->sliders.push_back( new SliderContainer(
		&pImpl_->trigger_yaw_sldr,
		&pImpl_->trigger_yaw_min,
		&pImpl_->trigger_yaw_max,
		0 ) );

	initMatcherField( pImpl_->cancel_speed_min, 1, 0, 100.f );
	initMatcherField( pImpl_->cancel_speed_max, 1, 0, 100.f );
	initMatcherField( pImpl_->cancel_aux1_min, 1, -360.f, 360.f );
	initMatcherField( pImpl_->cancel_aux1_max, 1, -360.f, 360.f );
	initMatcherField( pImpl_->cancel_yaw_min, 1, -360.f, 360.f );
	initMatcherField( pImpl_->cancel_yaw_max, 1, -360.f, 360.f );

	pImpl_->sliders.push_back( new SliderContainer(
		&pImpl_->cancel_speed_sldr,
		&pImpl_->cancel_speed_min,
		&pImpl_->cancel_speed_max,
		0 ) );

	pImpl_->sliders.push_back( new SliderContainer(
		&pImpl_->cancel_aux1_sldr,
		&pImpl_->cancel_aux1_min,
		&pImpl_->cancel_aux1_max,
		0 ) );

	pImpl_->sliders.push_back( new SliderContainer(
		&pImpl_->cancel_yaw_sldr,
		&pImpl_->cancel_yaw_min,
		&pImpl_->cancel_yaw_max,
		0 ) );

	pImpl_->s_currPage = this;

	DataSectionPtr caps = BWResource::openSection(
		EntityDef::Constants::entitiesCapabilitiesFile() );
	if (caps.exists())
	{
		caps->openSections( "state", pImpl_->capsList );
	}
	else
	{
		WARNING_MSG(" Unable to locate the action matcher capabilities file \"%s\".\n",
			EntityDef::Constants::entitiesCapabilitiesFile() );
	}

	BW::vector< DataSectionPtr >::iterator it = pImpl_->capsList.begin();
	BW::vector< DataSectionPtr >::iterator end = pImpl_->capsList.end();

	for(;it != end; ++it)
	{
		int id = (*it)->asInt();
		capsNames_[id] = (*it)->readWideString( "name", L"" );;
	}
}

PageActions::~PageActions()
{
	BW_GUARD;

	clearData();
}

/*static*/ PageActions* PageActions::currPage()
{
	return PageActionsImpl::s_currPage;
}

void PageActions::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	TreeList::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_ACT_MRU, pImpl_->mru);

	DDX_Control(pDX, IDC_ACT_NAME, pImpl_->name);
	DDX_Control(pDX, IDC_ACT_ANIM, pImpl_->anim);
	DDX_Control(pDX, IDC_ACT_CHANGE_ANIM, pImpl_->change_anim);

	DDX_Control(pDX, IDC_ACT_BLEND_IN, pImpl_->blendIn);
	DDX_Control(pDX, IDC_ACT_BLEND_IN_SLIDER, pImpl_->blendInSlider);
	DDX_Control(pDX, IDC_ACT_BLEND_OUT, pImpl_->blendOut);
	DDX_Control(pDX, IDC_ACT_BLEND_OUT_SLIDER, pImpl_->blendOutSlider);

	DDX_Control(pDX, IDC_ACT_LOOP, pImpl_->looped);
	DDX_Control(pDX, IDC_ACT_MOVE, pImpl_->move);
	DDX_Control(pDX, IDC_ACT_IMPACT, pImpl_->impact);
	DDX_Control(pDX, IDC_ACT_COORD, pImpl_->coord);
	DDX_Control(pDX, IDC_ACT_TRACK, pImpl_->track);
	DDX_Control(pDX, IDC_ACT_OWN_TRACK, pImpl_->ownTrack);

	DDX_Control(pDX, IDC_ACT_MATCH_BOX, pImpl_->match_box);

	DDX_Control(pDX, IDC_ACT_TRIGGER_BOX, pImpl_->trigger_box);
	
	DDX_Control(pDX, IDC_ACT_TRIGGER_FALLING, pImpl_->trigger_falling);

	DDX_Control(pDX, IDC_ACT_TRIGGER_SPEED_SLDR, pImpl_->trigger_speed_sldr);
	DDX_Control(pDX, IDC_ACT_TRIGGER_SPEED_MIN, pImpl_->trigger_speed_min);
	DDX_Control(pDX, IDC_ACT_TRIGGER_SPEED_MAX, pImpl_->trigger_speed_max);

	DDX_Control(pDX, IDC_ACT_TRIGGER_AUX1_SLDR, pImpl_->trigger_aux1_sldr);
	DDX_Control(pDX, IDC_ACT_TRIGGER_AUX1_MIN, pImpl_->trigger_aux1_min);
	DDX_Control(pDX, IDC_ACT_TRIGGER_AUX1_MAX, pImpl_->trigger_aux1_max);

	DDX_Control(pDX, IDC_ACT_TRIGGER_YAW_SLDR, pImpl_->trigger_yaw_sldr);
	DDX_Control(pDX, IDC_ACT_TRIGGER_YAW_MIN, pImpl_->trigger_yaw_min);
	DDX_Control(pDX, IDC_ACT_TRIGGER_YAW_MAX, pImpl_->trigger_yaw_max);

	DDX_Control(pDX, IDC_ACT_TRIGGER_CAPS_ON, pImpl_->trigger_caps_on);
	DDX_Control(pDX, IDC_ACT_TRIGGER_CAPS_ON_SEL, pImpl_->trigger_caps_on_sel);

	DDX_Control(pDX, IDC_ACT_TRIGGER_CAPS_OFF, pImpl_->trigger_caps_off);
	DDX_Control(pDX, IDC_ACT_TRIGGER_CAPS_OFF_SEL, pImpl_->trigger_caps_off_sel);

	DDX_Control(pDX, IDC_ACT_CANCEL_BOX, pImpl_->cancel_box);
	
	DDX_Control(pDX, IDC_ACT_CANCEL_FALLING, pImpl_->cancel_falling);

	DDX_Control(pDX, IDC_ACT_CANCEL_SPEED_SLDR, pImpl_->cancel_speed_sldr);
	DDX_Control(pDX, IDC_ACT_CANCEL_SPEED_MIN, pImpl_->cancel_speed_min);
	DDX_Control(pDX, IDC_ACT_CANCEL_SPEED_MAX, pImpl_->cancel_speed_max);

	DDX_Control(pDX, IDC_ACT_CANCEL_AUX1_SLDR, pImpl_->cancel_aux1_sldr);
	DDX_Control(pDX, IDC_ACT_CANCEL_AUX1_MIN, pImpl_->cancel_aux1_min);
	DDX_Control(pDX, IDC_ACT_CANCEL_AUX1_MAX, pImpl_->cancel_aux1_max);

	DDX_Control(pDX, IDC_ACT_CANCEL_YAW_SLDR, pImpl_->cancel_yaw_sldr);
	DDX_Control(pDX, IDC_ACT_CANCEL_YAW_MIN, pImpl_->cancel_yaw_min);
	DDX_Control(pDX, IDC_ACT_CANCEL_YAW_MAX, pImpl_->cancel_yaw_max);

	DDX_Control(pDX, IDC_ACT_CANCEL_CAPS_ON, pImpl_->cancel_caps_on);
	DDX_Control(pDX, IDC_ACT_CANCEL_CAPS_ON_SEL, pImpl_->cancel_caps_on_sel);

	DDX_Control(pDX, IDC_ACT_CANCEL_CAPS_OFF, pImpl_->cancel_caps_off);
	DDX_Control(pDX, IDC_ACT_CANCEL_CAPS_OFF_SEL, pImpl_->cancel_caps_off_sel);

	DDX_Control(pDX, IDC_ACT_ONE_SHOT, pImpl_->one_shot);
	DDX_Control(pDX, IDC_ACT_SCALE_PLAYBACK_SPEED, pImpl_->scale_playback_speed);
	DDX_Control(pDX, IDC_ACT_FEET_FOLLOW_DIRECTION, pImpl_->feet_follow_direction);

	pImpl_->toolbar.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,0,0), this, 0 );

	GUI::Manager::instance().add( new GUI::Toolbar( "ActToolbar", pImpl_->toolbar ) );

	CWnd toolbarPos;
	DDX_Control(pDX, IDC_ACT_TOOLBAR, toolbarPos);
	
	CRect toolbarRect;
    toolbarPos.GetWindowRect (&toolbarRect);
    ScreenToClient (&toolbarRect);

	pImpl_->toolbar.MoveWindow(toolbarRect);

	//Make the action matcher properties text bold.
	CFont font;
	LOGFONT logFont;

	pImpl_->match_box.GetFont()->GetLogFont( &logFont );
	logFont.lfWeight = FW_BOLD;
	font.CreateFontIndirectW( &logFont );
	pImpl_->match_box.SetFont(&font);

	pImpl_->inited = true;
}

BOOL PageActions::OnInitDialog()
{
	BW_GUARD;

	pImpl_->blendIn.SetMinimum(0); 
	pImpl_->blendIn.SetMaximum(100);

	pImpl_->blendOut.SetMinimum(0); 
	pImpl_->blendOut.SetMaximum(100);
	
	pImpl_->trigger_speed_sldr.setRange( 0, 100.f, 1 );
	pImpl_->trigger_aux1_sldr.setRange( -360.f, 360.f, 1 );
	pImpl_->trigger_yaw_sldr.setRange( -360.f, 360.f, 1 );

	pImpl_->cancel_speed_sldr.setRange( 0, 100.f, 1 );
	pImpl_->cancel_aux1_sldr.setRange( -360.f, 360.f, 1 );
	pImpl_->cancel_yaw_sldr.setRange( -360.f, 360.f, 1 );
		
	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/*
 * This method gets the named caps for a caps string, the caps string 
 * is the string stored in the model file action section, which is a
 * string of numbers matching the caps that are set for the property
 */
BW::wstring PageActions::getNamedCaps( const BW::string& capsStr )
{
	BW_GUARD;

	// Tokenise the cap string
	BW::vector< BW::string > temp;
	bw_tokenise( capsStr, ":, ", temp );

	BW::wstring caps = L"";

	// Iterate over tokens in cap string
	for (unsigned i=0; i < temp.size(); i++)
	{	
		// Convert token to int
		int val = 0;
		sscanf( temp[i].c_str(), "%d", &val );
		if (i != 0)
		{
			caps += L", ";
		}
		
		// If a nice name is defined for the token, add it to our named string
		// oterwise name it as Undefined #
		if (capsNames_.find(val) != capsNames_.end())
		{
			caps += capsNames_[ val ];
		}
		else
		{
			char buf[32];
			bw_snprintf( buf, 32, "Undefined %s", temp[i].c_str() );
			BW::wstring numberedCap;
			bw_utf8tow( buf, numberedCap );
			caps += numberedCap;
		}
	}

	return caps;
}

BEGIN_MESSAGE_MAP(PageActions, TreeList)

	ON_WM_CREATE()

	ON_WM_SIZE()

	ON_WM_CTLCOLOR()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_WM_HSCROLL()

	ON_NOTIFY(NM_CLICK, IDC_ACT_MRU, OnNMClickMRU)
	
	ON_EN_CHANGE(IDC_ACT_NAME, OnEnChangeActName)
	ON_EN_KILLFOCUS(IDC_ACT_NAME, updateName)

	ON_BN_CLICKED(IDC_ACT_CHANGE_ANIM, OnChangeActAnim)

	ON_BN_CLICKED(IDC_ACT_LOOP, updateFlags)
	ON_BN_CLICKED(IDC_ACT_MOVE, updateFlags)
	ON_BN_CLICKED(IDC_ACT_IMPACT, updateFlags)
	ON_BN_CLICKED(IDC_ACT_COORD, updateFlags)
	
	ON_BN_CLICKED(IDC_ACT_OWN_TRACK, OnBnClickedActOwnTrack)

	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_SPEED_MIN, updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_SPEED_MAX, updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_AUX1_MIN,  updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_AUX1_MAX,  updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_YAW_MIN,   updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_YAW_MAX,   updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_CAPS_ON,   updateTriggerFlags)
	ON_EN_KILLFOCUS(IDC_ACT_TRIGGER_CAPS_OFF,  updateTriggerFlags)

	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_SPEED_MIN, updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_SPEED_MAX, updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_AUX1_MIN,  updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_AUX1_MAX,  updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_YAW_MIN,   updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_YAW_MAX,   updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_CAPS_ON,   updateCancelFlags)
	ON_EN_KILLFOCUS(IDC_ACT_CANCEL_CAPS_OFF,  updateCancelFlags)

	ON_BN_CLICKED(IDC_ACT_ONE_SHOT, updateMatchFlags)
	ON_BN_CLICKED(IDC_ACT_SCALE_PLAYBACK_SPEED, updateMatchFlags)
	ON_BN_CLICKED(IDC_ACT_FEET_FOLLOW_DIRECTION, updateMatchFlags)
	
	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_BN_CLICKED(IDC_ACT_TRIGGER_CAPS_ON_SEL, OnBnClickedActTriggerCapsOnSel)
	ON_BN_CLICKED(IDC_ACT_TRIGGER_CAPS_OFF_SEL, OnBnClickedActTriggerCapsOffSel)
	ON_BN_CLICKED(IDC_ACT_CANCEL_CAPS_ON_SEL, OnBnClickedActCancelCapsOnSel)
	ON_BN_CLICKED(IDC_ACT_CANCEL_CAPS_OFF_SEL, OnBnClickedActCancelCapsOffSel)

	ON_MESSAGE( WM_RANGESLIDER_CHANGED, OnRangeSliderChanged )
	ON_MESSAGE( WM_RANGESLIDER_TRACK, OnRangeSliderTrack )

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

	ON_BN_CLICKED(IDC_ACT_TRIGGER_FALLING, OnBnClickedActTriggerFalling)
	ON_BN_CLICKED(IDC_ACT_CANCEL_FALLING, OnBnClickedActCancelFalling)
END_MESSAGE_MAP()

void PageActions::clearData()
{
	BW_GUARD;

	for ( uint32 i = 0; i < data_.size(); i++ )
	{
		BW::string* data = data_[i];
		delete data;
	}
	data_.clear();
}

void PageActions::selClick( const StringPair& )
{
	pImpl_->selectClicked = true;
}

void PageActions::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageActions::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )                                                   
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageActions::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	LPTSTR* msg = (LPTSTR*)wParam;
	mainFrame_->setMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageActions::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	mainFrame_->setMessageText( L"" );
	return 0;
}

SliderContainerPtr PageActions::find( void* ptr )
{
	BW_GUARD;

	for ( BW::vector<SliderContainerPtr>::iterator i = pImpl_->sliders.begin();
		i != pImpl_->sliders.end(); ++i )
	{
		if ( (*i)->slider_ == (controls::RangeSliderCtrl*)ptr ||
			(*i)->minEd_ == (controls::EditNumeric*)ptr ||
			(*i)->maxEd_ == (controls::EditNumeric*)ptr )
			return (*i);
	}

	return NULL;
}

void PageActions::writeEditNum( controls::EditNumeric* edit, float num, int group /* = 0 */ )
{
	BW_GUARD;

	SliderContainerPtr slider = find( edit );
	if ( !slider )
		return;

	edit->SetValue( num );
}

void PageActions::sliderFromEdits( SliderContainerPtr slider )
{
	BW_GUARD;

	if ( !pImpl_->allowEditUpdates )
		return;

	CString str;
	
	float min = slider->minEd_->GetValue();
	float max = slider->maxEd_->GetValue();
	
	float oldmin = 0.f;
	float oldmax = 0.f;
	slider->slider_->getThumbValues( oldmin, oldmax );

	if ( (oldmin != min || oldmax != max) && max < min )
	{
		std::swap(min, max);
		pImpl_->allowEditUpdates = false;
		writeEditNum( slider->minEd_, min, slider->group_ );
		writeEditNum( slider->maxEd_, max, slider->group_ );
		pImpl_->allowEditUpdates = true;
	}

	slider->slider_->setThumbValues( min, max );
}

LRESULT PageActions::OnRangeSliderChanged(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	LRESULT result = OnRangeSliderTrack( wParam, lParam );

	bool oneUndoOnly = true;
	updateTriggerFlags( oneUndoOnly, false );
	updateCancelFlags( oneUndoOnly, false );

	return result;
}

LRESULT PageActions::OnRangeSliderTrack(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	SliderContainerPtr slider = find( (void*)lParam );

	if ( !slider )
		return 0; // should never do this

	pImpl_->allowEditUpdates = false;

	float min = 0;
	float max = 0;
	slider->slider_->getThumbValues( min, max );

	writeEditNum( slider->minEd_, min, slider->group_ );
	writeEditNum( slider->maxEd_, max, slider->group_ );

	pImpl_->allowEditUpdates = true;

	// This will reload the model when the user releases the mouse button
	// when dragging the slider.  This method will be called a final time
	// on mouse release.
	if (!slider->slider_->sliderHasFocus())
	{
		mutant()->reloadModel();
	}

	return 0;
}

// PageActions message handlers

int PageActions::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

void PageActions::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!pImpl_->inited) return;
	
	Utilities::stretchToRight( this, pImpl_->mru, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->name, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->anim, cx, 40 );
	Utilities::moveToRight( this, pImpl_->change_anim, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->blendInSlider, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->blendOutSlider, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->match_box, cx, 0 );
	Utilities::stretchToRight( this, pImpl_->trigger_box, cx, 6 );
	
	Utilities::stretchToRight( this, pImpl_->trigger_speed_sldr, cx, 58 );
	Utilities::moveToRight( this, pImpl_->trigger_speed_max, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->trigger_aux1_sldr, cx, 58 );
	Utilities::moveToRight( this, pImpl_->trigger_aux1_max, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->trigger_yaw_sldr, cx, 58 );
	Utilities::moveToRight( this, pImpl_->trigger_yaw_max, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->trigger_caps_on, cx, 40 );
	Utilities::moveToRight( this, pImpl_->trigger_caps_on_sel, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->trigger_caps_off, cx, 40 );
	Utilities::moveToRight( this, pImpl_->trigger_caps_off_sel, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->cancel_box, cx, 6 );
	
	Utilities::stretchToRight( this, pImpl_->cancel_speed_sldr, cx, 58 );
	Utilities::moveToRight( this, pImpl_->cancel_speed_max, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->cancel_aux1_sldr, cx, 58 );
	Utilities::moveToRight( this, pImpl_->cancel_aux1_max, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->cancel_yaw_sldr, cx, 58 );
	Utilities::moveToRight( this, pImpl_->cancel_yaw_max, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->cancel_caps_on, cx, 40 );
	Utilities::moveToRight( this, pImpl_->cancel_caps_on_sel, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->cancel_caps_off, cx, 40 );
	Utilities::moveToRight( this, pImpl_->cancel_caps_off_sel, cx, 12 );

	TreeList::OnSize( nType, cx, cy );
}

void PageActions::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	HWND target = pScrollBar->GetSafeHwnd();
	
	int pos, min, max;
	
	if (target == pImpl_->blendInSlider.GetSafeHwnd())
	{
		pos = pImpl_->blendInSlider.GetPos(); 
		min = pImpl_->blendInSlider.GetRangeMin(); 
		max = pImpl_->blendInSlider.GetRangeMax();

		float blendVal = (float)(1.f * (pos-min) / (max-min)); // Scale 0.0 -> 1.0

		pImpl_->blendIn.SetValue( blendVal );
	}

	if (target == pImpl_->blendOutSlider.GetSafeHwnd())
	{
		pos = pImpl_->blendOutSlider.GetPos(); 
		min = pImpl_->blendOutSlider.GetRangeMin(); 
		max = pImpl_->blendOutSlider.GetRangeMax();

		float blendVal = (float)(1.f * (pos-min) / (max-min)); // Scale 0.0 -> 1.0

		pImpl_->blendOut.SetValue( blendVal );
	}

	TreeList::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageActions::updateCheck( CButton& button, const StringPair& actID, const BW::string& flagName )
{
	BW_GUARD;

	bool flag = mutant()->actFlag( actID, flagName );
	button.SetCheck( flag ? BST_CHECKED : BST_UNCHECKED );
	button.ModifyStyle( WS_DISABLED, 0 );
}

void PageActions::disableCheck( CButton& button )
{
	BW_GUARD;

	button.SetCheck( BST_UNCHECKED );
	button.ModifyStyle( 0, WS_DISABLED );
}

void PageActions::updateMatchField( controls::EditNumeric& field, const StringPair& actID, const BW::string& typeName, const BW::string& flagName, bool lockedParents /* = false */, bool canBeNeg /* = true */ )
{
	BW_GUARD;

	bool valSet;
	float valFloat = mutant()->actMatchFloat( actID, typeName, flagName, valSet );
	if (!canBeNeg)
	{
		valFloat = fabsf( valFloat );
	}
	
	if (valSet)
	{
		field.SetValue( valFloat );
	}
	else
	{
		field.Clear();
	}
	field.SetReadOnly( lockedParents );
	field.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
}

void PageActions::disableField( CEdit& field )
{
	BW_GUARD;

	field.SetWindowText( L"" );
	field.SetReadOnly( true );
	field.ModifyStyle( 0, WS_DISABLED );
}

#define DISABLE_SLIDER_GROUP( name )						\
	pImpl_->##name##_sldr.ModifyStyle( 0, WS_DISABLED );	\
	pImpl_->##name##_sldr.setThumbValues( 0.f, 0.f );		\
	pImpl_->##name##_min.Clear();							\
	pImpl_->##name##_min.SetReadOnly( true );				\
	pImpl_->##name##_min.ModifyStyle( 0, WS_DISABLED );		\
	pImpl_->##name##_max.Clear();							\
	pImpl_->##name##_max.SetReadOnly( true );				\
	pImpl_->##name##_max.ModifyStyle( 0, WS_DISABLED );

void PageActions::removeMRU( const StringPair& action )
{
	BW_GUARD;

	BW::string modelPath = action.second;
	BW::string::size_type first = modelPath.rfind("/") + 1;
	BW::string::size_type last = modelPath.rfind(".");
	BW::string modelName = modelPath.substr( first, last-first );
	BW::string actionDisplay = action.first + " - " + modelName;
	
	HTREEITEM hItem = pImpl_->mru.GetChildItem( TVI_ROOT );
	while ( hItem != NULL )
	{
		BW::string itemDisplay = bw_wtoutf8( pImpl_->mru.GetItemText( hItem ).GetString() );
		if ( actionDisplay == itemDisplay )
		{
			BW::string* str = (BW::string*)pImpl_->mru.GetItemData(hItem);

			BW::vector<BW::string*>::iterator it = data_.begin();
			BW::vector<BW::string*>::iterator end = data_.end();

			while ( it != end )
			{
				if ( *it == str )
				{
					delete str;
					data_.erase( it );
					break;
				}

				++it;
			}

			pImpl_->mru.DeleteItem( hItem );
			break;
		}
		hItem = pImpl_->mru.GetNextItem( hItem, TVGN_NEXT );
	}

	pImpl_->mru.RedrawWindow();
}

void PageActions::addMRU( const StringPair &action )
{
	BW_GUARD;

	BW::string modelPath = action.second;
	BW::string::size_type first = modelPath.rfind("/") + 1;
	BW::string::size_type last = modelPath.rfind(".");
	BW::string modelName = modelPath.substr( first, last-first );
	BW::string actionDisplay = action.first + " - " + modelName;
	
	int items = 0;
	HTREEITEM hItem = pImpl_->mru.GetChildItem( TVI_ROOT );
	while ( hItem != NULL )
	{
		BW::string itemDisplay = bw_wtoutf8( pImpl_->mru.GetItemText( hItem ).GetString() );
		if ( actionDisplay == itemDisplay )
		{
			BW::string* itemModelPath = (BW::string*)(pImpl_->mru.GetItemData(hItem));
			pImpl_->mru.DeleteItem( hItem );
			hItem = pImpl_->mru.InsertItem( bw_utf8tow( actionDisplay ).c_str() );
			pImpl_->mru.SetItemData( hItem, (DWORD_PTR)itemModelPath );
			pImpl_->mru.SelectItem( hItem );
			return;
		}
		items++;
		hItem = pImpl_->mru.GetNextItem( hItem, TVGN_NEXT );
	}
	// Three items is max, if you get more then delete the oldest one
	if ( items == 3 )
	{
		hItem = pImpl_->mru.GetChildItem( TVI_ROOT );
		BW::string* itemModelPath = (BW::string*)(pImpl_->mru.GetItemData(hItem));
		BW::vector<BW::string*>::iterator it = data_.begin();
		BW::vector<BW::string*>::iterator end = data_.end();
		while ( it != end )
		{
			if ( *it == itemModelPath )
			{
				delete itemModelPath;
				data_.erase( it );
				break;
			}
			++it;
		}
		pImpl_->mru.DeleteItem( hItem );
	}
	if ( items >= 0 )
	{
		hItem = pImpl_->mru.InsertItem( bw_utf8tow( actionDisplay ).c_str() );
		BW::string* newModelPath = new BW::string(modelPath);
		pImpl_->mru.SetItemData( hItem, (DWORD_PTR)newModelPath );
		pImpl_->mru.SelectItem( hItem );
		data_.push_back( newModelPath );
	}

	pImpl_->mru.RedrawWindow();
}

afx_msg void PageActions::OnNMClickMRU(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	//Save the item index
	POINT point;
	::GetCursorPos( &point );
	pImpl_->mru.ScreenToClient(&point);
	HTREEITEM hItem = pImpl_->mru.HitTest( point, 0 );

	if ( hItem == NULL )
		return;

	if ( hItem != pImpl_->lastMRU )
	{
		pImpl_->MRUClicked = true;
		return;// it will be handled in OnUpdateControls
	}

	BW::string actionDisplay = bw_wtoutf8( pImpl_->mru.GetItemText( hItem ).GetString() );
	BW::string actionName = actionDisplay.substr( 0, actionDisplay.rfind( " - " ) );
	BW::string model = *(BW::string*)(pImpl_->mru.GetItemData( hItem ));

	StringPair action( actionName, model );
	mutant()->setAct( action );
}

afx_msg LRESULT PageActions::OnUpdateControls(WPARAM, LPARAM)
{
	BW_GUARD;

	pImpl_->updating = pImpl_->updateCount != mutant()->updateCount("Act");
	pImpl_->updateCount = mutant()->updateCount("Act");
	
	//if (CModelEditorApp::instance()->pythonAdapter())
	//{
		// TODO: We will do stuff here...
	//}

	if (!pImpl_->ready)
	{
		OnInitDialog();
		
		pImpl_->ready = true;
	}
	
	if (mutant())
	{
		//Has the model changed?
		BW::string modelName = mutant()->modelName();
		if (pImpl_->updating || (modelName != pImpl_->modelName))
		{
			OnUpdateTreeList();

			if (modelName == "")	
			{
				pImpl_->toolbar.ModifyStyle( 0, WS_DISABLED );
			}
			else
			{
				pImpl_->toolbar.ModifyStyle( WS_DISABLED, 0 );
			}
			GUI::Manager::instance().update();
			pImpl_->toolbar.RedrawWindow();

			// Ensure that the lock state will be updated
			int lockLod = Options::getOptionInt( "settings/lockLodParents", 0 );
			pImpl_->lastLockedParents = !lockLod;

			if (modelName != pImpl_->modelName)
			{
				pImpl_->modelName = modelName;
	
				HTREEITEM hItem = pImpl_->mru.GetChildItem( TVI_ROOT );
				while ( hItem != NULL )
				{
					BW::string* itemModelPath = (BW::string*)(pImpl_->mru.GetItemData(hItem));
					delete itemModelPath;
					hItem = pImpl_->mru.GetNextItem( hItem, TVGN_NEXT );
				}
				data_.clear();

				pImpl_->mru.DeleteAllItems();
			}
		}
	}

	// Ensure that the parent lock is enabled
	int lockLod = Options::getOptionInt( "settings/lockLodParents", 0 );
	if ( lockLod != pImpl_->lastLockedParents )
	{
		locked( !!lockLod );
		pImpl_->lastLockedParents = lockLod;
	}

	// This flagged determines if the root item in the action tree is selected.
	// If it is, no action should be played.  Note that an empty string is passed
	// as the action name since an action may have the same name as the model.
	static bool rootItemSelected = true;

	// Check whether the action (or model) selection has changed
	if (pImpl_->updating || (pImpl_->selectClicked && !sameClicked()) || pImpl_->MRUClicked)
	{
		bool hasParent = false;
		bool searchTree = true;
		StringPair selection;

		if (pImpl_->MRUClicked)
		{
			HTREEITEM hItem = pImpl_->mru.GetSelectedItem();
			if ( hItem )
			{
				hasParent = true;
				searchTree = false;
				BW::string actionDisplay = bw_wtoutf8( pImpl_->mru.GetItemText( hItem ).GetString() );
				selection.first = actionDisplay.substr( 0, actionDisplay.rfind( " - " ) );
				selection.second = *(BW::string*)(pImpl_->mru.GetItemData( hItem ));
			}
		}
		else
		{
			hasParent = tree().GetParentItem(selItem()) != NULL;
			selection = selID();
		}

		pImpl_->selectClicked = pImpl_->MRUClicked = false;

		if (hasParent && (mutant()->hasActs( selection.second )))
		{
			// A child item (an action) is selected
			rootItemSelected = false;
			
			bool lockedParents = locked();
			
			
			const BW::string& actName = mutant()->actName( selection );
			pImpl_->name.SetWindowText( bw_utf8tow( actName ).c_str() );
			pImpl_->name.SetReadOnly( lockedParents );
			pImpl_->name.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			const BW::string& actAnim = mutant()->actAnim( selection );
			pImpl_->anim.SetWindowText( bw_utf8tow( actAnim ).c_str() );
			pImpl_->anim.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->change_anim.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			float blendInVal = mutant()->actBlendTime( selection, "blendInTime" );
			pImpl_->blendIn.SetValue( blendInVal );
			pImpl_->blendIn.SetReadOnly( lockedParents );
			pImpl_->blendIn.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->blendInSlider.SetPos((int)( 100 * blendInVal ));
			pImpl_->blendInSlider.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			float blendOutVal = mutant()->actBlendTime( selection, "blendOutTime" );
			pImpl_->blendOut.SetValue( blendOutVal );
			pImpl_->blendOut.SetReadOnly( lockedParents );
			pImpl_->blendOut.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->blendOutSlider.SetPos((int)( 100 * blendOutVal ));
			pImpl_->blendOutSlider.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			updateCheck( pImpl_->looped, selection, "filler" );
			updateCheck( pImpl_->move,   selection, "isMovement" );
			updateCheck( pImpl_->impact, selection, "isImpacting" );
			updateCheck( pImpl_->coord,  selection, "isCoordinated" );

			pImpl_->looped.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->move.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->impact.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->coord.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			wchar_t buf[8];
			bool ownTrack = false;
			int actTrack = mutant()->actTrack( selection );
			if (actTrack != -1)
			{
				bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d", actTrack );
				pImpl_->track.SetWindowText( buf );
				pImpl_->ownTrack.SetCheck( BST_UNCHECKED );
			}
			else
			{
				ownTrack = true;
				pImpl_->track.SetWindowText( L"" );
				pImpl_->ownTrack.SetCheck( BST_CHECKED );
			}
			pImpl_->track.SetReadOnly( ownTrack || lockedParents );
			pImpl_->track.ModifyStyle( ownTrack || lockedParents ? 0 : WS_DISABLED, ownTrack || lockedParents ? WS_DISABLED : 0 );

			pImpl_->ownTrack.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			pImpl_->trigger_falling.ModifyStyle( WS_DISABLED, 0 );
			
			pImpl_->trigger_speed_sldr.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->trigger_aux1_sldr.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->trigger_yaw_sldr.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			
			bool valSet;
			bool falling = mutant()->actMatchFloat( selection, "trigger", "minEntitySpeed", valSet ) < 0.f;
			pImpl_->trigger_falling.SetCheck( falling ? BST_CHECKED : BST_UNCHECKED );
			pImpl_->trigger_falling.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			
			updateMatchField( pImpl_->trigger_speed_min, selection, "trigger", "minEntitySpeed",	lockedParents, false );
			updateMatchField( pImpl_->trigger_speed_max, selection, "trigger", "maxEntitySpeed",	lockedParents, false );
			updateMatchField( pImpl_->trigger_aux1_min, selection,  "trigger", "minEntityAux1",	lockedParents );
			updateMatchField( pImpl_->trigger_aux1_max, selection,  "trigger", "maxEntityAux1",	lockedParents );
			updateMatchField( pImpl_->trigger_yaw_min, selection,   "trigger", "minModelYaw",		lockedParents );
			updateMatchField( pImpl_->trigger_yaw_max, selection,   "trigger", "maxModelYaw",		lockedParents );

			pImpl_->trigger_caps_on.SetWindowText( getNamedCaps(
				mutant()->actMatchCaps( selection, "trigger", "capsOn" )).c_str());
			pImpl_->trigger_caps_off.SetWindowText( getNamedCaps(
				mutant()->actMatchCaps( selection, "trigger", "capsOff" )).c_str());

			pImpl_->trigger_caps_on_sel.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->trigger_caps_off_sel.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			pImpl_->cancel_falling.ModifyStyle( WS_DISABLED, 0 );
			
			pImpl_->cancel_speed_sldr.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->cancel_aux1_sldr.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->cancel_yaw_sldr.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			
			falling = mutant()->actMatchFloat( selection, "cancel", "minEntitySpeed", valSet ) < 0.f;
			pImpl_->cancel_falling.SetCheck( falling ? BST_CHECKED : BST_UNCHECKED );
			pImpl_->cancel_falling.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			
			updateMatchField( pImpl_->cancel_speed_min, selection, "cancel", "minEntitySpeed",	lockedParents, false );
			updateMatchField( pImpl_->cancel_speed_max, selection, "cancel", "maxEntitySpeed",	lockedParents, false );
			updateMatchField( pImpl_->cancel_aux1_min, selection,  "cancel", "minEntityAux1",		lockedParents );
			updateMatchField( pImpl_->cancel_aux1_max, selection,  "cancel", "maxEntityAux1",		lockedParents );
			updateMatchField( pImpl_->cancel_yaw_min, selection,   "cancel", "minModelYaw",		lockedParents );
			updateMatchField( pImpl_->cancel_yaw_max, selection,   "cancel", "maxModelYaw",		lockedParents );

			pImpl_->cancel_caps_on.SetWindowText( getNamedCaps(
				mutant()->actMatchCaps( selection, "cancel", "capsOn" )).c_str());
			pImpl_->cancel_caps_off.SetWindowText( getNamedCaps(
				mutant()->actMatchCaps( selection, "cancel", "capsOff" )).c_str());

			pImpl_->cancel_caps_on_sel.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			pImpl_->cancel_caps_off_sel.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			//Update all the sliders for the matcher fields
			for (unsigned i=0; i<pImpl_->sliders.size(); i++)
			{
				sliderFromEdits( pImpl_->sliders[i] );
			}

			updateCheck( pImpl_->one_shot,              selection, "match/oneShot" );
			updateCheck( pImpl_->scale_playback_speed,  selection, "match/scalePlaybackSpeed" );
			updateCheck( pImpl_->feet_follow_direction, selection, "match/feetFollowDirection" );

			pImpl_->one_shot.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );
			updateScalePlaybackSpeed();
			pImpl_->feet_follow_direction.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0 );

			if ( searchTree )
			{
				addMRU( selection );
			}
			else
			{
				selectItem( selection );
			}
		}
		else // We are a model...
		{
			// The root item (the model) is selected
			rootItemSelected = true;

			HTREEITEM model = tree().GetParentItem( selItem() );
			if (!model) model = selItem();

			disableField( pImpl_->name);
			pImpl_->name.SetWindowText( Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/MODEL", (LPCTSTR)tree().GetItemText( model )) );

			disableField( pImpl_->anim );
			pImpl_->change_anim.ModifyStyle( 0, WS_DISABLED );

			disableField( pImpl_->blendIn );
			pImpl_->blendInSlider.ModifyStyle( 0, WS_DISABLED );

			disableField( pImpl_->blendOut );
			pImpl_->blendOutSlider.ModifyStyle( 0, WS_DISABLED );
			
			disableCheck( pImpl_->looped );
			disableCheck( pImpl_->move );
			disableCheck( pImpl_->impact );
			disableCheck( pImpl_->coord );

			disableField( pImpl_->track );
			pImpl_->ownTrack.ModifyStyle( 0, WS_DISABLED );

			pImpl_->trigger_falling.ModifyStyle( 0, WS_DISABLED );
			
			DISABLE_SLIDER_GROUP( trigger_speed )
			DISABLE_SLIDER_GROUP( trigger_aux1 )
			DISABLE_SLIDER_GROUP( trigger_yaw )

			pImpl_->trigger_caps_on.SetWindowText(L"");
			pImpl_->trigger_caps_on_sel.ModifyStyle( 0, WS_DISABLED );
			pImpl_->trigger_caps_off.SetWindowText(L"");
			pImpl_->trigger_caps_off_sel.ModifyStyle( 0, WS_DISABLED );

			pImpl_->cancel_falling.ModifyStyle( 0, WS_DISABLED );
			
			DISABLE_SLIDER_GROUP( cancel_speed )
			DISABLE_SLIDER_GROUP( cancel_aux1 )
			DISABLE_SLIDER_GROUP( cancel_yaw )

			pImpl_->cancel_caps_on.SetWindowText(L"");
			pImpl_->cancel_caps_on_sel.ModifyStyle( 0, WS_DISABLED );
			pImpl_->cancel_caps_off.SetWindowText(L"");
			pImpl_->cancel_caps_off_sel.ModifyStyle( 0, WS_DISABLED );

			disableCheck( pImpl_->one_shot );
			disableCheck( pImpl_->scale_playback_speed );
			disableCheck( pImpl_->feet_follow_direction );
		}

		if (!pImpl_->updating)
		{
			if (rootItemSelected)
			{
				StringPair emptyAction( "", selection.second );
				mutant()->setAct( emptyAction );
			}
			else
			{
				mutant()->setAct( selection );
			}
		}

		GUI::Manager::instance().update();
		this->RedrawWindow();

		pImpl_->lastMRU = pImpl_->mru.GetSelectedItem();
		pImpl_->lastItem = selItem();
	}
	else if ( (pImpl_->lastItem == selItem()) && sameClicked() )
	{
		mutant()->setAct( selID() );
		sameClicked( false );
		pImpl_->selectClicked = false;
	}
	//Now catch any updates from pressing enter in a field or moving a slider

	if (pImpl_->name.doUpdate())
	{
		updateName();
	}

	if ((pImpl_->blendIn.doUpdate()) || (pImpl_->blendInSlider.doUpdate()))
	{
		float val = pImpl_->blendIn.GetValue();
		pImpl_->blendInSlider.SetPos((int)( 100 * val ));
		mutant()->actBlendTime( getAct(), "blendInTime", val );
	}

	if ((pImpl_->blendOut.doUpdate()) || (pImpl_->blendOutSlider.doUpdate()))
	{
		float val = pImpl_->blendOut.GetValue();
		pImpl_->blendOutSlider.SetPos((int)( 100 * val ));
		mutant()->actBlendTime( getAct(), "blendOutTime", val );
	}

	if (pImpl_->track.doUpdate())
	{
		updateTrack();
	}

	if ((pImpl_->trigger_speed_min.doUpdate()) ||
		(pImpl_->trigger_speed_max.doUpdate()) ||
		(pImpl_->trigger_aux1_min.doUpdate()) ||
		(pImpl_->trigger_aux1_max.doUpdate()) ||
		(pImpl_->trigger_yaw_min.doUpdate()) ||
		(pImpl_->trigger_yaw_max.doUpdate()))
	{
		updateTriggerFlags();
	}

	if ((pImpl_->cancel_speed_min.doUpdate()) ||
		(pImpl_->cancel_speed_max.doUpdate()) ||
		(pImpl_->cancel_aux1_min.doUpdate()) ||
		(pImpl_->cancel_aux1_max.doUpdate()) ||
		(pImpl_->cancel_yaw_min.doUpdate()) ||
		(pImpl_->cancel_yaw_max.doUpdate()))
	{
		updateCancelFlags();
	}
	
	return 0;
}


void PageActions::updateScalePlaybackSpeed()
{
	BW_GUARD;

	bool scalePlaybackSpeedEnabled = ( (pImpl_->move.GetCheck() == BST_CHECKED) && !locked() );
	pImpl_->scale_playback_speed.EnableWindow( (int)scalePlaybackSpeedEnabled );
	pImpl_->scale_playback_speed.RedrawWindow();
}


bool PageActions::isLockedAct()
{
	BW_GUARD;

	return locked();
}

/*~ function ModelEditor.isActLocked
 *	@components{ modeleditor }
 *
 *	This function returns whether the selected action is locked.
 *	If the 'lock LOD parents' option is enabled in the Preferences dialog, then actions
 *	that belong to the LOD parents of the currently visible LOD level will be locked
 *	from modifications.
 *
 *	@return Returns True (1) if the action is locked, False (0) otherwise.
 */
static PyObject * py_isActLocked( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		return PyInt_FromLong( PageActions::currPage()->isLockedAct() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( isActLocked, ModelEditor )

void PageActions::actNew()
{
	BW_GUARD;

	CChooseAnim chooseAnimDlg( IDD_CREATE_ACT, true, selID().second );
	if (chooseAnimDlg.DoModal() == IDOK)
	{
		StringPair actID ( chooseAnimDlg.actName() , selID().second );
		StringPair newAct = mutant()->createAct( actID, chooseAnimDlg.animName(), selID() );
		mutant()->setAct( newAct );
		selID( newAct );

		OnUpdateTreeList();
		OnUpdateControls( 0, 0 );
	}
}

/*~ function ModelEditor.newAct
 *	@components{ modeleditor }
 *
 *	This function enables ModelEditor's Create Action dialog.
 */
static PyObject * py_newAct( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		PageActions::currPage()->actNew();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( newAct, ModelEditor )

void PageActions::actPromote()
{
	BW_GUARD;

	HTREEITEM nextItem = tree().GetPrevSiblingItem( selItem() );

	if (nextItem == NULL) return;

	BW::string actionName = bw_wtoutf8( tree().GetItemText( nextItem ).GetString() );

	BW::string modelName = getAct().second;

	StringPair selNext( actionName, modelName );

	mutant()->swapActions( "Promoting", getAct(), selNext );

	OnUpdateTreeList();
}

/*~ function ModelEditor.promoteAct
 *	@components{ modeleditor }
 *
 *	This function promotes the currently selected action.
 *	Promoting an action moves it higher up the actions list.
 *	When the action matcher chooses an action to trigger, actions
 *	higher on the actions list will take precedence.
 */
static PyObject * py_promoteAct( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		PageActions::currPage()->actPromote();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( promoteAct, ModelEditor )

void PageActions::actDemote()
{
	BW_GUARD;

	HTREEITEM prevItem = tree().GetNextSiblingItem( selItem() );

	if (prevItem == NULL) return;

	BW::string actionName = bw_wtoutf8( tree().GetItemText( prevItem ).GetString() );

	BW::string modelName = getAct().second;

	StringPair selNext( actionName, modelName );

	mutant()->swapActions( "Demoting", getAct(), selNext );

	OnUpdateTreeList();
}

/*~ function ModelEditor.demoteAct
 *	@components{ modeleditor }
 *
 *	This function demotes the currently selected action.
 *	Demoting an action moves it lower down the actions list.
 *	When the action matcher chooses an action to trigger, actions
 *	higher on the actions list will take precedence.
 */
static PyObject * py_demoteAct( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		PageActions::currPage()->actDemote();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( demoteAct, ModelEditor )

void PageActions::actRemove()
{
	BW_GUARD;

	HTREEITEM nextItem = tree().GetPrevSiblingItem( selItem() );
	
	if (nextItem != NULL) 
	{
		BW::string actionName = bw_wtoutf8( tree().GetItemText( nextItem ).GetString() );

		BW::string modelName = getAct().second;

		selID( StringPair( actionName, modelName ) );
	}

	mutant()->removeAct( getAct() );
	removeMRU( getAct() );

	mutant()->setAct( selID() );

	OnUpdateTreeList();
	OnUpdateControls( 0, 0 );
}

/*~ function ModelEditor.removeAct
 *	@components{ modeleditor }
 *
 *	This function removes the currently selected action from the model.
 */
static PyObject * py_removeAct( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		PageActions::currPage()->actRemove();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( removeAct, ModelEditor )

void PageActions::actPlay()
{
	BW_GUARD;

	mutant()->setAct( getAct() );
}

/*~ function ModelEditor.playAct
 *	@components{ modeleditor }
 *
 *	This function forces the currently selected action to play.
 */
static PyObject * py_playAct( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		PageActions::currPage()->actPlay();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( playAct, ModelEditor )

void PageActions::actStop()
{
	BW_GUARD;

	mutant()->stopAct();
}

/*~ function ModelEditor.stopAct
 *	@components{ modeleditor }
 *
 *	This function stops all currently playing actions.
 */
static PyObject * py_stopAct( PyObject * args )
{
	BW_GUARD;

	if (PageActions::currPage())
		PageActions::currPage()->actStop();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( stopAct, ModelEditor )

void PageActions::OnEnChangeActName()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	//Do nothing if there is no parent item (i.e. we are a model, not an action)
	//This is not necessary since the field should be readonly (but lets be safe)
	if (tree().GetParentItem(selItem()) == NULL)
		return;

	int first, last;
	pImpl_->name.GetSel(first,last);
	CString actName_cstr;
	pImpl_->name.GetWindowText( actName_cstr );
	BW::string actName( bw_wtoutf8( actName_cstr.GetString() ) );
	actName = Utilities::pythonSafeName( actName );
	pImpl_->name.SetWindowText( bw_utf8tow( actName ).c_str() );
	tree().SetItemText( selItem(), bw_utf8tow( actName ).c_str() );
	pImpl_->name.SetSel(first,last);

	BW::string modelPath = getAct().second;
	BW::string modelName = modelPath.substr( modelPath.rfind("/") + 1 );
	BW::string actionDisplay;

	modelName.resize( modelName.rfind(".") );
	actionDisplay = actName + " - " + modelName;
	pImpl_->mru.SetItemText( pImpl_->lastMRU, bw_utf8tow( actionDisplay ).c_str() );
}

void PageActions::updateName()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	//Do nothing if there is no parent item (i.e. we are a model, not an action)
	if (tree().GetParentItem(selItem()) == NULL)
		return;

	if (!pImpl_->name.doUpdate())
		return;

	CString new_name_cstr;
	pImpl_->name.GetWindowText(new_name_cstr);
	BW::string new_name = bw_wtoutf8( new_name_cstr.GetString() );

	if (new_name == "")
	{
		pImpl_->name.SetWindowText( bw_utf8tow( getAct().first ).c_str() );
		tree().SetItemText( selItem(), bw_utf8tow( getAct().first ).c_str() );
		
		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_ACTIONS/NO_RENAME_ACTION"),
			Localise(L"MODELEDITOR/PAGES/PAGE_ACTIONS/INVALID_ACTION_NAME"), MB_OK | MB_ICONERROR );
	}
	else if (mutant()->actName( getAct(), new_name ))
	{
		selID( StringPair( new_name, selID().second ) );
	}
	else
	{
		pImpl_->name.SetWindowText( bw_utf8tow( getAct().first ).c_str() );
		tree().SetItemText( selItem(), bw_utf8tow( getAct().first ).c_str() );

		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_ACTIONS/ACTION_NAME_USED"),
			Localise(L"MODELEDITOR/PAGES/PAGE_ACTIONS/ACTION_NAME_EXISTS"), MB_OK | MB_ICONERROR );
	}
}

void PageActions::OnChangeActAnim()
{
	BW_GUARD;

	CChooseAnim chooseAnimDlg( IDD_CHOOSE_ANIM, false, selID().second );
	if (chooseAnimDlg.DoModal() == IDOK)
	{
		mutant()->actAnim( getAct(), chooseAnimDlg.animName() );
		pImpl_->anim.SetWindowText( bw_utf8tow( chooseAnimDlg.animName() ).c_str() );
	}
}

void PageActions::updateFlags()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	mutant()->actFlag( getAct(), "filler",        pImpl_->looped.GetCheck() == BST_CHECKED );
	mutant()->actFlag( getAct(), "isMovement",    pImpl_->move.GetCheck() == BST_CHECKED );
	mutant()->actFlag( getAct(), "isCoordinated", pImpl_->coord.GetCheck() == BST_CHECKED );
	mutant()->actFlag( getAct(), "isImpacting",   pImpl_->impact.GetCheck() == BST_CHECKED );

	updateScalePlaybackSpeed();

	GUI::Manager::instance().update();

	mutant()->reloadModel();
}

void PageActions::updateTrack()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	//First lets get the value entered
	CString trackStr;
	pImpl_->track.GetWindowText( trackStr );

	int trackNum = -1;
	if (trackStr != "") // If something was entered
	{
		swscanf( trackStr.GetString(), L"%d", &trackNum );
	}
	if ( trackNum == -1 )
	{
		pImpl_->ownTrack.SetCheck( BST_CHECKED );
		disableField( pImpl_->track );
	}
	mutant()->actTrack( getAct(), trackNum );
}

BW::string PageActions::getFieldText( CEdit& field )
{
	BW_GUARD;

	CString fieldStr;
	field.GetWindowText( fieldStr );
	return bw_wtoutf8(fieldStr.GetString());
}

void PageActions::updateTriggerFlags()
{
	BW_GUARD;

	bool updateUndo = true;
	updateTriggerFlags( updateUndo, true );
}

void PageActions::updateTriggerFlags( bool & shouldUndo, bool reloadModel /*= false*/ )
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;

	bool falling = pImpl_->trigger_falling.GetCheck() == BST_CHECKED;

	bool hasChanged = false;

	// Clear edit field pairs if either is empty
	if (pImpl_->trigger_speed_min.isEmpty() ||
		pImpl_->trigger_speed_max.isEmpty() ||
		( pImpl_->trigger_speed_min.GetValue() == 0.1f &&
		pImpl_->trigger_speed_max.GetValue() == 0.1f ))
	{
		pImpl_->trigger_speed_min.Clear();
		pImpl_->trigger_speed_max.Clear();
	}
	hasChanged |= mutant()->actMatchVal( getAct(), "trigger", "minEntitySpeed", pImpl_->trigger_speed_min.isEmpty(),
		(falling ? -1.f : 1.f) * pImpl_->trigger_speed_min.GetValue(), shouldUndo  );
	hasChanged |= mutant()->actMatchVal( getAct(), "trigger", "maxEntitySpeed", pImpl_->trigger_speed_max.isEmpty(),
		(falling ? -1.f : 1.f) * pImpl_->trigger_speed_max.GetValue(), shouldUndo  );

	// Clear edit field pairs if either is empty
	if (pImpl_->trigger_aux1_min.isEmpty() ||
		pImpl_->trigger_aux1_max.isEmpty() ||
		( pImpl_->trigger_aux1_min.GetValue() == 0.f &&
		pImpl_->trigger_aux1_max.GetValue() == 0.f ))
	{
		pImpl_->trigger_aux1_min.Clear();
		pImpl_->trigger_aux1_max.Clear();
	}
	hasChanged |= mutant()->actMatchVal( getAct(), "trigger", "minEntityAux1",  pImpl_->trigger_aux1_min.isEmpty(),	pImpl_->trigger_aux1_min.GetValue(), shouldUndo  );
	hasChanged |= mutant()->actMatchVal( getAct(), "trigger", "maxEntityAux1",  pImpl_->trigger_aux1_max.isEmpty(),	pImpl_->trigger_aux1_max.GetValue(), shouldUndo  );

	// Clear edit field pairs if either is empty
	if (pImpl_->trigger_yaw_min.isEmpty() ||
		pImpl_->trigger_yaw_max.isEmpty() ||
		( pImpl_->trigger_yaw_min.GetValue() == 0.f &&
		pImpl_->trigger_yaw_max.GetValue() == 0.f ))
	{
		pImpl_->trigger_yaw_min.Clear();
		pImpl_->trigger_yaw_max.Clear();
	}
	hasChanged |= mutant()->actMatchVal( getAct(), "trigger", "minModelYaw",    pImpl_->trigger_yaw_min.isEmpty(),	pImpl_->trigger_yaw_min.GetValue(), shouldUndo  );
	hasChanged |= mutant()->actMatchVal( getAct(), "trigger", "maxModelYaw",    pImpl_->trigger_yaw_max.isEmpty(),	pImpl_->trigger_yaw_max.GetValue(), shouldUndo  );

	//Update all the sliders for the matcher fields
	for (unsigned i=0; i<pImpl_->sliders.size(); i++)
	{
		sliderFromEdits( pImpl_->sliders[i] );
	}

	// We only want to reload the model when the reload model flag is set
	// and changes have been made
	if (reloadModel && hasChanged)
	{
		mutant()->reloadModel();
	}

	GUI::Manager::instance().update();
}

void PageActions::updateCancelFlags()
{
	BW_GUARD;

	bool updateUndo = true;
	updateCancelFlags( updateUndo, true );
}

void PageActions::updateCancelFlags( bool &shouldUndo, bool reloadModel /*= false*/ )
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;

	bool falling = pImpl_->cancel_falling.GetCheck() == BST_CHECKED;

	bool hasChanged = false;

	// Clear edit field pairs if either is empty
	if (pImpl_->cancel_speed_min.isEmpty() ||
		pImpl_->cancel_speed_max.isEmpty() ||
		( pImpl_->cancel_speed_min.GetValue() == 0.1f &&
		pImpl_->cancel_speed_max.GetValue() == 0.1f ))
	{
		pImpl_->cancel_speed_min.Clear();
		pImpl_->cancel_speed_max.Clear();
	}
	hasChanged |= mutant()->actMatchVal( getAct(), "cancel", "minEntitySpeed", pImpl_->cancel_speed_min.isEmpty(),
		(falling ? -1.f : 1.f) * pImpl_->cancel_speed_min.GetValue(), shouldUndo );
	hasChanged |= mutant()->actMatchVal( getAct(), "cancel", "maxEntitySpeed", pImpl_->cancel_speed_max.isEmpty(),
		(falling ? -1.f : 1.f) * pImpl_->cancel_speed_max.GetValue(), shouldUndo  );

	// Clear edit field pairs if either is empty
	if (pImpl_->cancel_aux1_min.isEmpty() ||
		pImpl_->cancel_aux1_max.isEmpty() ||
		( pImpl_->cancel_aux1_min.GetValue() == 0.f &&
		pImpl_->cancel_aux1_max.GetValue() == 0.f ))
	{
		pImpl_->cancel_aux1_min.Clear();
		pImpl_->cancel_aux1_max.Clear();
	}
	hasChanged |= mutant()->actMatchVal( getAct(), "cancel", "minEntityAux1",  pImpl_->cancel_aux1_min.isEmpty(),
		pImpl_->cancel_aux1_min.GetValue(), shouldUndo  );
	hasChanged |= mutant()->actMatchVal( getAct(), "cancel", "maxEntityAux1",  pImpl_->cancel_aux1_max.isEmpty(),
		pImpl_->cancel_aux1_max.GetValue(), shouldUndo  );

	// Clear edit field pairs if either is empty
	if (pImpl_->cancel_yaw_min.isEmpty() ||
		pImpl_->cancel_yaw_max.isEmpty() ||
		( pImpl_->cancel_yaw_min.GetValue() == 0.f &&
		pImpl_->cancel_yaw_max.GetValue() == 0.f ))
	{
		pImpl_->cancel_yaw_min.Clear();
		pImpl_->cancel_yaw_max.Clear();
	}
	hasChanged |= mutant()->actMatchVal( getAct(), "cancel", "minModelYaw",    pImpl_->cancel_yaw_min.isEmpty(),
		pImpl_->cancel_yaw_min.GetValue(), shouldUndo  );
	hasChanged |= mutant()->actMatchVal( getAct(), "cancel", "maxModelYaw",    pImpl_->cancel_yaw_max.isEmpty(),
		pImpl_->cancel_yaw_max.GetValue(), shouldUndo  );

	//Update all the sliders for the matcher fields
	for (unsigned i=0; i<pImpl_->sliders.size(); i++)
	{
		sliderFromEdits( pImpl_->sliders[i] );
	}

	// We only want to reload the model when the reload model flag is set
	// and changes have been made
	if (reloadModel && hasChanged)
	{
		mutant()->reloadModel();
	}

	GUI::Manager::instance().update();
}

void PageActions::updateMatchFlags()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	mutant()->actMatchFlag( getAct(), "oneShot",             pImpl_->one_shot.GetCheck() == BST_CHECKED );
	mutant()->actMatchFlag( getAct(), "scalePlaybackSpeed",  pImpl_->scale_playback_speed.GetCheck() == BST_CHECKED );
	mutant()->actMatchFlag( getAct(), "feetFollowDirection", pImpl_->feet_follow_direction.GetCheck() == BST_CHECKED );

	GUI::Manager::instance().update();

	mutant()->reloadModel();
}

void PageActions::OnBnClickedActOwnTrack()
{
	BW_GUARD;

	bool ownTrack = pImpl_->ownTrack.GetCheck() == BST_CHECKED;
	if ( ownTrack )
	{
		disableField( pImpl_->track );
		mutant()->actTrack( getAct(), -1 );
	}
	else
	{
		pImpl_->track.ModifyStyle( WS_DISABLED, 0 );
		pImpl_->track.SetReadOnly( false );
		pImpl_->track.SetWindowText( L"0" );
		mutant()->actTrack( getAct(), 0 );
	}
}

void PageActions::OnBnClickedActTriggerCapsOnSel()
{
	BW_GUARD;

	BW::string capsStr = mutant()->actMatchCaps( getAct(), "trigger", "capsOn" );
	CTriggerList triggerListDlg( "Action Triggers (On)", pImpl_->capsList, capsStr );
	if (triggerListDlg.DoModal() == IDOK)
	{
		BW::string caps = triggerListDlg.caps();
		
		mutant()->actMatchCaps( getAct(), "trigger", "capsOn", caps );

		pImpl_->trigger_caps_on.SetWindowText( getNamedCaps( caps ).c_str() );
	}
}

void PageActions::OnBnClickedActTriggerCapsOffSel()
{
	BW_GUARD;

	BW::string capsStr = mutant()->actMatchCaps( getAct(), "trigger", "capsOff" );
	CTriggerList triggerListDlg( "Action Triggers (Off)", pImpl_->capsList, capsStr );
	if (triggerListDlg.DoModal() == IDOK)
	{
		BW::string caps = triggerListDlg.caps();
		
		mutant()->actMatchCaps( getAct(), "trigger", "capsOff", caps );

		pImpl_->trigger_caps_off.SetWindowText( getNamedCaps( caps ).c_str() );
	}
}

void PageActions::OnBnClickedActCancelCapsOnSel()
{
	BW_GUARD;

	BW::string capsStr = mutant()->actMatchCaps( getAct(), "cancel", "capsOn" );
	CTriggerList triggerListDlg( "Action Cancel (On)", pImpl_->capsList, capsStr );
	if (triggerListDlg.DoModal() == IDOK)
	{
		BW::string caps = triggerListDlg.caps();
		
		mutant()->actMatchCaps( getAct(), "cancel", "capsOn", caps );

		pImpl_->cancel_caps_on.SetWindowText( getNamedCaps( caps ).c_str() );
	}
}

void PageActions::OnBnClickedActCancelCapsOffSel()
{
	BW_GUARD;

	BW::string capsStr = mutant()->actMatchCaps( getAct(), "cancel", "capsOff" );
	CTriggerList triggerListDlg( "Action Cancel (Off)", pImpl_->capsList, capsStr );
	if (triggerListDlg.DoModal() == IDOK)
	{
		BW::string caps = triggerListDlg.caps();
		
		mutant()->actMatchCaps( getAct(), "cancel", "capsOff", caps );

		pImpl_->cancel_caps_off.SetWindowText( getNamedCaps( caps ).c_str() );
	}
}

void PageActions::OnBnClickedActTriggerFalling()
{
	BW_GUARD;

	bool falling = pImpl_->trigger_falling.GetCheck() == BST_CHECKED;

	initMatcherField( pImpl_->trigger_speed_min, 1, 
		falling ? 0.1f : 0.f,
		100.f, false );

	initMatcherField( pImpl_->trigger_speed_max, 1, 
		falling ? 0.1f : 0.f,
		100.f, false );

	initMatcherField( pImpl_->trigger_aux1_min, 0, 
		-360.f, 360.f, true );

	initMatcherField( pImpl_->trigger_aux1_max, 0, 
		-360.f, 360.f, true );

	initMatcherField( pImpl_->trigger_yaw_min, 0, 
		-360.f, 360.f, true );

	initMatcherField( pImpl_->trigger_yaw_max, 0, 
		-360.f, 360.f, true );

	pImpl_->trigger_speed_sldr.setRange(
		falling ? 0.1f : 0.f,
		100.f, 1 );

	pImpl_->trigger_aux1_sldr.setRange(
		-360.f, 360.f, 0 );

	pImpl_->trigger_yaw_sldr.setRange(
		-360.f, 360.f, 0 );

	if (falling) 
	{
		if ((pImpl_->trigger_speed_min.isEmpty()) || (pImpl_->trigger_speed_min.GetValue() == 0.f))
		{
			pImpl_->trigger_speed_min.SetValue( 0.1f );
		}
		if ((pImpl_->trigger_speed_max.isEmpty()) || (pImpl_->trigger_speed_min.GetValue() == 0.f))
		{
			pImpl_->trigger_speed_max.SetValue( 0.1f );
		}
		if ((pImpl_->trigger_aux1_min.isEmpty()) || (pImpl_->trigger_aux1_min.GetValue() == 0.f))
		{
			pImpl_->trigger_aux1_min.SetValue( 0.0f );
		}
		if ((pImpl_->trigger_aux1_max.isEmpty()) || (pImpl_->trigger_aux1_max.GetValue() == 0.f))
		{
			pImpl_->trigger_aux1_max.SetValue( 0.0f );
		}
		if ((pImpl_->trigger_yaw_min.isEmpty()) || (pImpl_->trigger_yaw_min.GetValue() == 0.f))
		{
			pImpl_->trigger_yaw_min.SetValue( 0.0f );
		}
		if ((pImpl_->trigger_yaw_max.isEmpty()) || (pImpl_->trigger_yaw_max.GetValue() == 0.f))
		{
			pImpl_->trigger_yaw_max.SetValue( 0.0f );
		}
	}

	updateTriggerFlags();
}

void PageActions::OnBnClickedActCancelFalling()
{
	BW_GUARD;

	bool falling = pImpl_->cancel_falling.GetCheck() == BST_CHECKED;

	initMatcherField( pImpl_->cancel_speed_min, 1, 
		falling ? 0.1f : 0.f,
		100.f, false );

	initMatcherField( pImpl_->cancel_speed_max, 1, 
		falling ? 0.1f : 0.f,
		100.f, false );

	initMatcherField( pImpl_->cancel_aux1_min, 0, 
		-360.f, 360.f, true );

	initMatcherField( pImpl_->cancel_aux1_max, 0, 
		-360.f, 360.f, true );

	initMatcherField( pImpl_->cancel_yaw_min, 0, 
		-360.f, 360.f, true );

	initMatcherField( pImpl_->cancel_yaw_max, 0, 
		-360.f, 360.f, true );

	pImpl_->cancel_speed_sldr.setRange(
		falling ? 0.1f : 0.f,
		100.f, 1 );

	pImpl_->cancel_aux1_sldr.setRange(
		-360.f, 360.f, 0 );

	pImpl_->cancel_yaw_sldr.setRange(
		-360.f, 360.f, 0 );

	if (falling) 
	{
		if ((pImpl_->cancel_speed_min.isEmpty()) || (pImpl_->cancel_speed_min.GetValue() == 0.f))
		{
			pImpl_->cancel_speed_min.SetValue( 0.1f );
		}
		if ((pImpl_->cancel_speed_max.isEmpty()) || (pImpl_->cancel_speed_min.GetValue() == 0.f))
		{
			pImpl_->cancel_speed_max.SetValue( 0.1f );
		}
		if ((pImpl_->cancel_aux1_min.isEmpty()) || (pImpl_->cancel_aux1_min.GetValue() == 0.f))
		{
			pImpl_->cancel_aux1_min.SetValue( 0.0f );
		}
		if ((pImpl_->cancel_aux1_max.isEmpty()) || (pImpl_->cancel_aux1_max.GetValue() == 0.f))
		{
			pImpl_->cancel_aux1_max.SetValue( 0.0f );
		}
		if ((pImpl_->cancel_yaw_min.isEmpty()) || (pImpl_->cancel_yaw_min.GetValue() == 0.f))
		{
			pImpl_->cancel_yaw_min.SetValue( 0.0f );
		}
		if ((pImpl_->cancel_yaw_max.isEmpty()) || (pImpl_->cancel_yaw_max.GetValue() == 0.f))
		{
			pImpl_->cancel_yaw_max.SetValue( 0.0f );
		}
	}

	updateCancelFlags();
}

/*afx_msg*/ HBRUSH PageActions::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor ) 
{
	BW_GUARD;

	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor ); 

	for ( BW::vector<SliderContainerPtr>::iterator i = pImpl_->sliders.begin();
		i != pImpl_->sliders.end(); ++i )
	{
		(*i)->minEd_->SetBoundsColour( pDC, pWnd,
			(*i)->minEd_->GetMinimum(), (*i)->maxEd_->GetMaximum());

		(*i)->maxEd_->SetBoundsColour( pDC, pWnd, 
				(*i)->minEd_->GetMinimum(), (*i)->maxEd_->GetMaximum() );
	}

	return brush; 
} 

BW_END_NAMESPACE

