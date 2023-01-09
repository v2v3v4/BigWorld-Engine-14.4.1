// graphics_settings_table.cpp : implementation file
//
#include "pch.hpp"
#include "graphics_settings_table.hpp"
#include "user_messages.hpp"

#include "common/editor_views.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "moo/graphics_settings.hpp"
#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

typedef SmartPointer<Moo::GraphicsSetting> GraphicsSettingPtr;


///////////////////////////////////////////////////////////////////////////////
// Section: SettingsProxy helper local class
///////////////////////////////////////////////////////////////////////////////
/**
 *	Proxy class to get/set the value of a graphics setting in the property
 *	list.
 */
class SettingsProxy : public IntProxy
{
public:
	SettingsProxy( GraphicsSettingPtr setting, GraphicsSettingsTable* page ):
		setting_( setting ),
		page_( page )
	{
	}

	~SettingsProxy()
	{
	}

	int32 get() const
	{
		BW_GUARD;

		// return the curOption returned by isPending if the setting requires a
		// save, or by activeOption otherwise.
		int curOption = 0;
		if ( !Moo::GraphicsSetting::isPending( setting_, curOption ) )
		{
			curOption = setting_->activeOption();
		}
		return curOption;
	}

	void set( int32 value, bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		setting_->selectOption( value );
		// commit changes immediatly.
		if ( Moo::GraphicsSetting::hasPending() )
			Moo::GraphicsSetting::commitPending();

		// if the setting needs restart, notify the page.
		if ( setting_->requiresRestart() )
			page_->needsRestart( setting_->label() );
	}

protected:
	GraphicsSettingPtr setting_;
	GraphicsSettingsTable* page_;
};


///////////////////////////////////////////////////////////////////////////////
// Section: GraphicsSettingsTable
///////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor
 */
GraphicsSettingsTable::GraphicsSettingsTable( UINT dlgId )
	: PropertyTable( dlgId )
	, inited_( false )
	, needsRestart_( false )
{
}


/**
 *	Destructor
 */
GraphicsSettingsTable::~GraphicsSettingsTable()
{
}


/**
 *	Initialises the property list from the graphics settings. This method can
 *	be called multiple times by setting the inited_ flag to false.
 *
 *  @returns		true if successful, false otherwise.
 */
bool GraphicsSettingsTable::init()
{
	BW_GUARD;

	if (inited_)
		return false;

	inited_ = true;

	PropTableSetter setter( this );

	int oldSel = propertyList()->GetCurSel();
	propertyList()->deselectCurrentItem();

	if ( editor_ )
		editor_->expel();
	// Must tell the smartpointer that the reference is already incremented,
	// because the PyObjectPlus base class increments the refcnt (!)
	editor_ = GeneralEditorPtr( new GeneralEditor(), true );

	const Moo::GraphicsSetting::GraphicsSettingVector& settings =
		Moo::GraphicsSetting::settings();

	for( Moo::GraphicsSetting::GraphicsSettingVector::const_iterator
		i = settings.begin(); i != settings.end(); ++i )
	{
		const GraphicsSettingPtr setting = *i;
		const Moo::GraphicsSetting::Options& options = setting->options();
		DataSectionPtr ds = new XMLSection( "setting" );
		int index = 0;
		for ( Moo::GraphicsSetting::Options::const_iterator
			j = options.begin(); j != options.end(); ++j )
		{
			BW::string value = (*j).m_label;
			ds->writeInt( value, index++ );
		}
		BW::string label = setting->desc();
		if ( changedSettings_.find( setting->label() ) != changedSettings_.end() )
			label = label + " *";
		editor_->addProperty(
			new ChoiceProperty(
				Name( label ), new SettingsProxy( setting, this ), ds ) );
	}

	editor_->elect();

	propertyList()->SetRedraw(FALSE);

	propertyList()->clear();

	for (BW::list<BaseView*>::iterator vi = viewList().begin();
		vi != viewList().end();
		vi++)
	{
		addItemsForView(*vi);
	}

	propertyList()->SetCurSel( oldSel );

	propertyList()->SetRedraw(TRUE);

	return true;
}


/**
 *	Helper method let know the page that settings that take effect after the 
 *	application restarts. Should be called from derived classes even if they
 *  override it.
 *
 *  @param		graphics setting string (label or option)
 */
void GraphicsSettingsTable::needsRestart( const BW::string& setting )
{
	BW_GUARD;

	changedSettings_.insert( setting );
	needsRestart_ = true;

	// Trigger a rebuild of the list, to get the new '*' in the setting's label.
	inited_ = false;
}


/**
 *	DoDataExchange
 */
void GraphicsSettingsTable::DoDataExchange(CDataExchange* pDX)
{
	PropertyTable::DoDataExchange(pDX);
}


/**
 *	MFC Message Map
 */
BEGIN_MESSAGE_MAP(GraphicsSettingsTable, CFormView)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_MESSAGE(WM_SELECT_PROPERTYITEM, OnSelectPropertyItem)
	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_MESSAGE(WM_DBLCLK_PROPERTYITEM, OnDblClkPropertyItem)
END_MESSAGE_MAP()


/**
 *	Called every frame by the tool. Inits the property list (if not already
 *	inited) and calls update on the base class.
 */
afx_msg LRESULT GraphicsSettingsTable::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	init();

	update();

	return 0;
}


/**
 *	A list item has been selected
 */
afx_msg LRESULT GraphicsSettingsTable::OnSelectPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onSelect();
	}

	return 0;
}


/**
 *	A list item's value has been changed
 */
afx_msg LRESULT GraphicsSettingsTable::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onChange(wParam != 0);
	}

	return 0;
}


/**
 *	A list item has been right-clicked
 */
afx_msg LRESULT GraphicsSettingsTable::OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		PropertyItem* relevantView = (PropertyItem*)lParam;
		relevantView->onBrowse();
	}
	
	return 0;
}


BW_END_NAMESPACE

