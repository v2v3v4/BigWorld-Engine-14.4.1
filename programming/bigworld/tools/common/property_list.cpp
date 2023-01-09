#include "pch.hpp"

#include "property_list.hpp"
#include "user_messages.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "editor_views.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 2 )

BW_BEGIN_NAMESPACE

//-----------------------------------------------------------------------------
//	Anonymous namespace - utility functions
//-----------------------------------------------------------------------------

namespace
{

static int s_viewControlsHookUsers = 0;
static HHOOK s_viewControlsHook = NULL;
typedef BW::map< HWND, PropertyList * > ViewControlsMap;
static ViewControlsMap s_viewControlsHookWindows;


/**
 *	Windows created for view input should register themselves here, so, when
 *	they get the KillFocus message, we can deselect the item from the list.
 */
void registerViewHook( HWND hwnd, PropertyList * list )
{
	s_viewControlsHookWindows.insert( std::make_pair( hwnd, list ) );
}


/**
 *	Hook to detect when a view gets the KillFocus event, in which case we
 *	need to remove it.
 */
LRESULT WINAPI viewControlsWndProcHook( int nCode, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;
	
	CWPSTRUCT * pInfo = (CWPSTRUCT *)lParam;

	if (nCode == HC_ACTION && pInfo->message == WM_KILLFOCUS)
	{
		// Check to see if the window that lost the focus is one of our views
		ViewControlsMap::iterator itKilled =
						s_viewControlsHookWindows.find( pInfo->hwnd );
		if (itKilled != s_viewControlsHookWindows.end())
		{
			ViewControlsMap::iterator itFocused =
						s_viewControlsHookWindows.find( (HWND)pInfo->wParam );
			if (itFocused == s_viewControlsHookWindows.end() ||
				(*itFocused).second != (*itKilled).second)
			{
				// The new focused window is not one of our views, or it is but
				// it's from a different property list, so deselect ourselves
				// from our property list.
				(*itKilled).second->needsDeselectCurrentItem();
				s_viewControlsHookWindows.erase( pInfo->hwnd );
			}
		}
	}

	return CallNextHookEx( s_viewControlsHook, nCode, wParam, lParam );
}

} // anonymous namespace


//-----------------------------------------------------------------------------
//	Section: PropertyItem
//-----------------------------------------------------------------------------

PropertyItem::PropertyItem(const Name& name) 
	: name_( name )
	, stringValue_( "" )
	, selectable_( true )
	, changeBuddy_( NULL )
	, parent_( NULL )
	, groupDepth_( 0 )
	, canExposeToScript_( false )
	, arrayIndex_( -1 )
	, multipleValue_( false )
{
}


Name PropertyItem::name() const
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
	{
		return name_;
	}
	else
	{
		return name_.str() + " (" + exposedToScriptName_.c_str() + ')';
	}
}


/**
 *	This method returns a fully qualified name, including the group.
 *
 *	@return  Fully qualified name, including the group.
 */
Name PropertyItem::fullName() const
{
	BW_GUARD;

	const Name& group = this->getGroup();

	if (group.empty())
	{
		return this->name();
	}

	return group.str() + '/' + this->name().c_str();
}


CString PropertyItem::displayValue() const
{
	BW_GUARD;

	return multipleValue_ ? L"" : stringValue_;
}


void PropertyItem::setGroup( const Name& group )
{
	BW_GUARD;

	MF_ASSERT( !group.empty() || group[0] != '/');

	// trim extra '/' from tail
	group_ = group;
	while (group_.back() == '/')
	{
		BW::string s = group_.str();

		s.resize( s.size() - 1 );
		group_ = s;
	}

	MF_ASSERT( group_.size() <= INT_MAX );
	if (group_.empty())
		groupDepth_ = 0;
	else
	{
		groupDepth_ =
			( int ) std::count( group_.begin( ), group_.end( ), '/' ) + 1;
	}
}


/**
 *	This method returns whether the item is a group.
 *
 *	@return  Whether the item is a group.
 */
/*virtual*/ bool PropertyItem::isGroupType() const
{
	return	this->getType() == Type_Group ||
			this->getType() == Type_Colour ||
			this->getType() == Type_Vector ||
			this->getType() == Type_ColourScalar;
}


/*virtual*/ BW::wstring PropertyItem::UIDescExtra() const
{
	BW_GUARD;

	if (!canExposeToScript_) return L"";
	
	if (exposedToScriptName_.empty())
	{
		return Localise(L"COMMON/PROPERTY_LIST/EXPOSE_PYTHON");
	}
	else
	{
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_EXPOSED", bw_utf8tow( exposedToScriptName_.c_str() ) );
	}
}


//-----------------------------------------------------------------------------
//	Section: ChangeDetectEdit
//-----------------------------------------------------------------------------

BEGIN_MESSAGE_MAP( ChangeDetectEdit, CEdit )
	ON_WM_CHAR()
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
//	Section: GroupPropertyItem
//-----------------------------------------------------------------------------

GroupPropertyItem::GroupPropertyItem(const Name& name, int depth)
	: PropertyItem( name )
	, expanded_( true )
{
	groupDepth_ = depth;
}

GroupPropertyItem::~GroupPropertyItem()
{
}

void GroupPropertyItem::create( PropertyList* parent )
{
	parent_ = parent;
}

void GroupPropertyItem::select( CRect rect, bool showDropDown /*= true*/ )
{
	if (parent_)
	{
		parent_->selectedItem( this );
	}
}

void GroupPropertyItem::deselect()
{
	if (!parent_ || parent_->selectedItem() != this)
	{
		return;
	}

	parent_->selectedItem( NULL );
}

void GroupPropertyItem::addChild( PropertyItem * child )
{
	BW_GUARD;

	children_.push_back( child );

	child->setGroup( this->fullName() );
}


//-----------------------------------------------------------------------------
//	Section: ColourPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, ChangeDetectEdit*> ColourPropertyItem::edit_;
/*static*/ BW::map<CWnd*, CButton*> ColourPropertyItem::button_;

ColourPropertyItem::ColourPropertyItem(const Name& name, const CString& init, int depth, bool colour)
	: GroupPropertyItem( name, depth )
{
	stringValue_ = init;
	groupDepth_ = depth;
	colour_ = colour;
}

ColourPropertyItem::~ColourPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(edit_[ parent_ ]);
	edit_.erase( parent_ );
	bw_safe_delete(button_[ parent_ ]);
	button_.erase( parent_ );
}

void ColourPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (edit_.find( parent_ ) == edit_.end())
	{
		edit_[ parent_ ] = new ChangeDetectEdit();
		
		CRect rect(10, 10, 10, 10);
		DWORD style = ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER;
		edit_[ parent_ ]->Create(style, rect, parent, IDC_PROPERTYLIST_STRING);

		CFont* pFont = parent->GetParent()->GetFont();
		edit_[ parent_ ]->SetFont(pFont);
	}

	if (colour_ && button_.find( parent_ ) == button_.end())
	{
		button_[ parent_ ] = new CButton();
		
		CRect rect(10, 10, 10, 10);
		DWORD style = BS_PUSHBUTTON | WS_CHILD | WS_BORDER;
		button_[ parent_ ]->Create( L"...", style, rect, parent, IDC_PROPERTYLIST_BROWSE);

		CFont* pFont = parent->GetParent()->GetFont();
		button_[ parent_ ]->SetFont(pFont);
	}
}

void ColourPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return;
	if ((edit_.find(parent_) == edit_.end()) && (button_.find(parent_) == button_.end()))
		return;
	
	// check the readonly attribute
	// note: must use SendMessage, ModifyStyle does not work on CEdit
	//if (readOnly_)
	//	edit_[ parent_ ]->SendMessage(EM_SETREADONLY, TRUE, 0);
	//else
		edit_[ parent_ ]->SendMessage(EM_SETREADONLY, FALSE, 0);

	const int BUTTON_WIDTH = 20;

	// display edit box
	rect.bottom -= 1;		// keep it inside the selection
	if (colour_)
	{
		rect.left += rect.Height(); // Avoid stomping on the colour preview
		rect.right -= BUTTON_WIDTH; // Add room for button
	}
	edit_[ parent_ ]->MoveWindow(rect);

	if (this->multipleValue())
	{
		edit_[ parent_ ]->SetWindowText( L"" );
		edit_[ parent_ ]->touched( false );
	}
	else
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}
	edit_[ parent_ ]->ShowWindow(SW_SHOW);
	edit_[ parent_ ]->SetFocus();
	edit_[ parent_ ]->SetSel(0, -1);
	registerViewHook( edit_[ parent_ ]->GetSafeHwnd(), parent_ );

	if (colour_)
	{
		rect.left = rect.right;
		rect.right += BUTTON_WIDTH;
		
		button_[ parent_ ]->MoveWindow(rect);

		button_[ parent_ ]->ShowWindow(SW_SHOW);
		button_[ parent_ ]->SetWindowPos( &CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
		registerViewHook( button_[ parent_ ]->GetSafeHwnd(), parent_ );
	}

	parent_->selectedItem( this );
}

void ColourPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()) && (button_.find(parent_) == button_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	CString newStr;
	edit_[ parent_ ]->GetWindowText(newStr);
	if ((!this->multipleValue() && stringValue_ != newStr) ||
		(this->multipleValue() && edit_[ parent_ ]->touched()))
	{
		stringValue_ = newStr;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	edit_[ parent_ ]->ShowWindow(SW_HIDE);
	edit_[ parent_ ]->Invalidate();

	if (colour_)
	{
		button_[ parent_ ]->ShowWindow(SW_HIDE);
		button_[ parent_ ]->Invalidate();
	}

	parent_->selectedItem( NULL );
}

void ColourPropertyItem::set(const BW::wstring& value)
{
	BW_GUARD;

	stringValue_ = value.c_str();

	if (parent_ && parent_->selectedItem() == this && edit_.find( parent_ ) != edit_.end())
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
	}
}

BW::wstring ColourPropertyItem::get() const
{
	BW_GUARD;
	return (LPCTSTR)stringValue_;
}

void ColourPropertyItem::onBrowse()
{
	BW_GUARD;

	if (!colour_) return; // Don't show a colour picker for vectors
	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()))
		return;
	
	CString color;
	edit_[ parent_ ]->GetWindowText( color );
	int r,g,b,a;
	swscanf( color, L"%d , %d , %d , %d", &r, &g, &b, &a );
	CColorDialog colorDlg( RGB( r, g, b ), CC_FULLOPEN );
	if( colorDlg.DoModal() == IDOK )
	{
		wchar_t s[256];
		COLORREF col = colorDlg.GetColor();
		r = col & 0xff;
		g = (col / 256) & 0xff;
		b = (col / 65536) & 0xff;
		bw_snwprintf( s, ARRAY_SIZE(s), L"%d , %d , %d , %d", r,g,b,a );
		stringValue_ = s;
		edit_[ parent_ ]->SetWindowText( s );
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}
	parent_->Invalidate();
}

/*virtual*/ BW::wstring ColourPropertyItem::menuOptions()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_ON");
}



//-----------------------------------------------------------------------------
//	Section: ColourScalarPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, ChangeDetectEdit*> ColourScalarPropertyItem::edit_;
/*static*/ BW::map<CWnd*, CButton*> ColourScalarPropertyItem::button_;

ColourScalarPropertyItem::ColourScalarPropertyItem(const Name& name, const CString& init, int depth)
	: GroupPropertyItem( name, depth )
{
	stringValue_ = init;
	groupDepth_ = depth;	
}

ColourScalarPropertyItem::~ColourScalarPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(edit_[ parent_ ]);
	edit_.erase( parent_ );
	bw_safe_delete(button_[ parent_ ]);
	button_.erase( parent_ );
}

void ColourScalarPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (edit_.find( parent_ ) == edit_.end())
	{
		edit_[ parent_ ] = new ChangeDetectEdit();
		
		CRect rect(10, 10, 10, 10);
		DWORD style = ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER;
		edit_[ parent_ ]->Create(style, rect, parent, IDC_PROPERTYLIST_STRING);

		CFont* pFont = parent->GetParent()->GetFont();
		edit_[ parent_ ]->SetFont(pFont);
	}

	if ((button_.find( parent_ ) == button_.end()))
	{
		button_[ parent_ ] = new CButton();
		
		CRect rect(10, 10, 10, 10);
		DWORD style = BS_PUSHBUTTON | WS_CHILD | WS_BORDER;
		button_[ parent_ ]->Create( L"...", style, rect, parent, IDC_PROPERTYLIST_BROWSE);

		CFont* pFont = parent->GetParent()->GetFont();
		button_[ parent_ ]->SetFont(pFont);
	}
}

void ColourScalarPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return;
	if ((edit_.find(parent_) == edit_.end()) && (button_.find(parent_) == button_.end()))
		return;
	
	// check the readonly attribute
	// note: must use SendMessage, ModifyStyle does not work on CEdit
	//if (readOnly_)
	//	edit_[ parent_ ]->SendMessage(EM_SETREADONLY, TRUE, 0);
	//else
		edit_[ parent_ ]->SendMessage(EM_SETREADONLY, FALSE, 0);

	const int BUTTON_WIDTH = 20;

	// display edit box
	rect.bottom -= 1;		// keep it inside the selection
	rect.left += rect.Height(); // Avoid stomping on the colour preview
	rect.right -= BUTTON_WIDTH; // Add room for button	
	edit_[ parent_ ]->MoveWindow(rect);

	if (this->multipleValue())
	{
		edit_[ parent_ ]->SetWindowText( L"" );
		edit_[ parent_ ]->touched( false );
	}
	else
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}
	edit_[ parent_ ]->ShowWindow(SW_SHOW);
	edit_[ parent_ ]->SetFocus();
	edit_[ parent_ ]->SetSel(0, -1);
	registerViewHook( edit_[ parent_ ]->GetSafeHwnd(), parent_ );

	rect.left = rect.right;
	rect.right += BUTTON_WIDTH;
	button_[ parent_ ]->MoveWindow(rect);
	button_[ parent_ ]->ShowWindow(SW_SHOW);
	button_[ parent_ ]->SetWindowPos( &CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	registerViewHook( button_[ parent_ ]->GetSafeHwnd(), parent_ );

	parent_->selectedItem( this );
}

void ColourScalarPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()) && (button_.find(parent_) == button_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	CString newStr;
	edit_[ parent_ ]->GetWindowText(newStr);

	if ((!this->multipleValue() && stringValue_ != newStr) ||
		(this->multipleValue() && edit_[ parent_ ]->touched()))
	{
		stringValue_ = newStr;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	edit_[ parent_ ]->ShowWindow(SW_HIDE);
	edit_[ parent_ ]->Invalidate();

	button_[ parent_ ]->ShowWindow(SW_HIDE);
	button_[ parent_ ]->Invalidate();

	parent_->selectedItem( NULL );
}

void ColourScalarPropertyItem::set(const BW::string& value)
{
	BW_GUARD;

	stringValue_ = value.c_str();

	if (parent_ && parent_->selectedItem() == this && edit_.find(parent_) != edit_.end())
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
	}
}

BW::string ColourScalarPropertyItem::get() const
{
	BW_GUARD;
	return bw_wtoutf8( stringValue_.GetString() );
}

void ColourScalarPropertyItem::onBrowse()
{	
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()))
		return;
	
	CString color;
	edit_[ parent_ ]->GetWindowText( color );
	int r,g,b;
	////COLOURSCALAR figure out how to get this alpha to set it back in the string value
	float a;
	swscanf( color.GetString(), L"%d , %d , %d , %f", &r, &g, &b, &a );
	CColorDialog colorDlg( RGB( r, g, b ), CC_FULLOPEN );
	if( colorDlg.DoModal() == IDOK )
	{
		wchar_t s[256];
		COLORREF col = colorDlg.GetColor();
		r = col & 0xff;
		g = (col / 256) & 0xff;
		b = (col / 65536) & 0xff;
		bw_snwprintf( s, ARRAY_SIZE(s), L"%d , %d , %d , %0.2f", r,g,b,a );
		stringValue_ = s;
		edit_[ parent_ ]->SetWindowText( s );
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}
	parent_->Invalidate();
}

/*virtual*/ BW::wstring ColourScalarPropertyItem::menuOptions()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_ON");
}




//-----------------------------------------------------------------------------
//	Section: LabelPropertyItem
//-----------------------------------------------------------------------------

LabelPropertyItem::LabelPropertyItem(const Name& name, bool highlight)
	: PropertyItem( name )
	, highlight_( highlight )
{
}

LabelPropertyItem::~LabelPropertyItem()
{
}

void LabelPropertyItem::create(PropertyList* parent)
{
	parent_ = parent;
}

void LabelPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	parent_->selectedItem( this );
}

void LabelPropertyItem::deselect()
{
	if (parent_->selectedItem() != this)
		return;

	parent_->selectedItem( NULL );
}

PropertyItem::ItemType LabelPropertyItem::getType() const
{
	if (highlight_)
        return Type_Label_Highlight;
	else
		return Type_Label;
}


//-----------------------------------------------------------------------------
//	Section: StringPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, ChangeDetectEdit*> StringPropertyItem::edit_;
/*static*/ BW::map<CWnd*, CButton*> StringPropertyItem::button_;


StringPropertyItem::StringPropertyItem(const Name& name, const CString& currentValue, bool readOnly)
	: PropertyItem( name )
	, readOnly_( readOnly )
	, defaultDir_(L"")
	, canTextureFeed_( false )
{
	stringValue_ = currentValue;
}

StringPropertyItem::~StringPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(edit_[ parent_ ]);
	edit_.erase( parent_ );
	bw_safe_delete(button_[ parent_ ]);
	button_.erase( parent_ );
}

/*virtual*/ Name StringPropertyItem::name()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
	{
		return name_;
	}
	else
	{
		return name_.str() + " (" + bw_wtoutf8( textureFeed_ ) + ')';
	}
}

void StringPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (edit_.find( parent_ ) == edit_.end())
	{
		edit_[ parent_ ] = new ChangeDetectEdit();

		CRect rect(10, 10, 10, 10);
		DWORD style = ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER;
		edit_[ parent_ ]->Create(style, rect, parent, IDC_PROPERTYLIST_STRING);

		CFont* pFont = parent->GetParent()->GetFont();
		edit_[ parent_ ]->SetFont(pFont);
	}

	if (button_.find( parent_ ) == button_.end())
	{
		button_[ parent_ ] = new CButton();

		CRect rect(10, 10, 10, 10);
		DWORD style = BS_PUSHBUTTON | WS_CHILD | WS_BORDER;
		button_[ parent_ ]->Create( L"...", style, rect, parent, IDC_PROPERTYLIST_BROWSE);

		CFont* pFont = parent->GetParent()->GetFont();
		button_[ parent_ ]->SetFont(pFont);
	}
}

void StringPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()) && (button_.find(parent_) == button_.end()))
		return;
	// check the readonly attribute
	// note: must use SendMessage, ModifyStyle does not work on CEdit
	if (readOnly_)
		edit_[ parent_ ]->SendMessage(EM_SETREADONLY, TRUE, 0);
	else
		edit_[ parent_ ]->SendMessage(EM_SETREADONLY, FALSE, 0);

	const int BUTTON_WIDTH = ( ( fileFilter_.size() == 0 && !isHexColor() && !isVectColor() ) || readOnly_ ) ? 0 : 20;

	// display edit box
	rect.bottom -= 1;		// keep it inside the selection
	rect.right -= BUTTON_WIDTH;
	edit_[ parent_ ]->MoveWindow(rect);

	if (this->multipleValue())
	{
		edit_[ parent_ ]->SetWindowText( L"" );
		edit_[ parent_ ]->touched( false );
	}
	else
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}
	edit_[ parent_ ]->ShowWindow(SW_SHOW);
	edit_[ parent_ ]->SetFocus();
	edit_[ parent_ ]->SetSel(0, -1);
	registerViewHook( edit_[ parent_ ]->GetSafeHwnd(), parent_ );

	rect.left = rect.right;
	rect.right += BUTTON_WIDTH;
	
	button_[ parent_ ]->MoveWindow(rect);

	button_[ parent_ ]->ShowWindow(SW_SHOW);
	button_[ parent_ ]->SetWindowPos( &CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	registerViewHook( button_[ parent_ ]->GetSafeHwnd(), parent_ );

	parent_->selectedItem( this );
}


void StringPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()) && (button_.find(parent_) == button_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	CString newStr;
	edit_[ parent_ ]->GetWindowText(newStr);

	if (fileFilter_.size())
	{
		BW::string nfilename;
		bw_wtoutf8( newStr, nfilename );
		BW::string disolvedFileName = BWResource::dissolveFilename( nfilename );
		if ((newStr != "") && (!BWResource::validPath( disolvedFileName )))
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"COMMON/PROPERTY_LIST/FILE_IN_FOLDERS_ONLY"),
				Localise(L"COMMON/PROPERTY_LIST/UNABLE_RESOLVE"),
				MB_OK | MB_ICONWARNING );
			edit_[ parent_ ]->SetWindowText( stringValue_ );
			newStr = stringValue_;
		}
	}

	if ((!this->multipleValue() && stringValue_ != newStr) ||
		(this->multipleValue() && edit_[ parent_ ]->touched() && !readOnly_))
	{
		stringValue_ = newStr;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	edit_[ parent_ ]->ShowWindow(SW_HIDE);
	edit_[ parent_ ]->Invalidate();

	button_[ parent_ ]->ShowWindow(SW_HIDE);
	button_[ parent_ ]->Invalidate();

	parent_->selectedItem( NULL );
}

void StringPropertyItem::set(const BW::wstring& value)
{
	BW_GUARD;

	stringValue_ = value.c_str();
	
	if (parent_ && parent_->selectedItem() == this && edit_.find(parent_) != edit_.end())
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}
}

BW::wstring StringPropertyItem::get() const
{
	BW_GUARD;
	return (LPCTSTR)stringValue_;
}

void StringPropertyItem::onBrowse()
{
	BW_GUARD;

	if (!parent_ || readOnly_) return; 
	if ((edit_.find(parent_) == edit_.end()))
		return;
	if( isHexColor() )
	{
		struct MapHex
		{
			int h2d( char ch )
			{
				if( ch >= '0' && ch <= '9' )
					return ch - '0';
				if( ch >= 'A' && ch <= 'F' )
					return ch - 'A' + 10;
				return ch - 'a' + 10;
			}
			int operator()( LPCTSTR str )
			{
				int result = 0;
				while( *str )
				{
					result *= 16;
					result += h2d( static_cast< char >( *str ) );
					++str;
				}
				return result;
			}
		}
		MapHex;
		struct SwapRB
		{
			COLORREF operator()( COLORREF ref )
			{
				return ( ( ref / 65536 ) & 0xff ) + ( ( ref / 256 ) & 0xff ) * 256 + ( ref & 0xff ) * 65536;
			}
		}
		SwapRB;
		CString color;
		edit_[ parent_ ]->GetWindowText( color );
		CColorDialog colorDlg( SwapRB( MapHex( (LPCTSTR)color + 1 ) ), CC_FULLOPEN );
		if( colorDlg.DoModal() == IDOK )
		{
			CString s;
			s.Format(
				_T( "#%06x" ),
				( SwapRB( colorDlg.GetColor() ) & 0xffffff )
			);
			stringValue_ = s;
			edit_[ parent_ ]->SetWindowText( s );
			parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
		}
	}
	else if ( isVectColor() )
	{
		CString color;
		edit_[ parent_ ]->GetWindowText( color );
		int r,g,b;
		swscanf( color, L"%d , %d , %d", &r, &g, &b );
		CColorDialog colorDlg( RGB( r, g, b ), CC_FULLOPEN );
		if( colorDlg.DoModal() == IDOK )
		{
			wchar_t s[256];
			COLORREF col = colorDlg.GetColor();
			r = col & 0xff;
			g = (col / 256) & 0xff;
			b = (col / 65536) & 0xff;
			bw_snwprintf( s, ARRAY_SIZE(s), L"%d , %d , %d", r,g,b );
			stringValue_ = s;
			edit_[ parent_ ]->SetWindowText( s );
			parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
		}
	}
	else
	{
		MF_ASSERT( fileFilter_.size() );
		BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
			( BWFileDialog::FD_HIDEREADONLY | BWFileDialog::FD_OVERWRITEPROMPT );
		BWFileDialog fileDialog( true, NULL, NULL, flags
			, bw_utf8tow( fileFilter_.c_str() ).c_str()
			, ( void * ) edit_[ parent_ ]->GetParent() );

		BW::wstring initDir = stringValue_ != L"" ? BW::wstring(stringValue_) : defaultDir_;
		BW::string ninitDir;
		bw_wtoutf8( initDir, ninitDir );
		bw_utf8tow( BWResource::resolveFilename( ninitDir ), initDir );
		initDir = initDir.substr( 0, initDir.find_last_of( L'/' ));
		std::replace( initDir.begin(), initDir.end(), L'/', L'\\' );
		fileDialog.initialDir( initDir.c_str() );

		if (fileDialog.showDialog())
		{
			BW::string npathName;
			bw_wtoutf8( fileDialog.getFileName(), npathName );
			BW::string dissolvedFileName = BWResource::dissolveFilename( npathName );
			if (BWResource::validPath( dissolvedFileName ))
			{
				BW::wstring wdissolvedFileName;
				bw_utf8tow( dissolvedFileName, wdissolvedFileName );
				stringValue_ = wdissolvedFileName.c_str();
				edit_[ parent_ ]->SetWindowText( stringValue_ );
				parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
			}
			else
			{
				::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
					Localise(L"COMMON/PROPERTY_LIST/FILE_IN_FOLDERS_ONLY"),
					Localise(L"COMMON/PROPERTY_LIST/UNABLE_RESOLVE"),
					MB_OK | MB_ICONWARNING );
			}
		}
	}
}

/*virtual*/ BW::wstring StringPropertyItem::menuOptions()
{
	BW_GUARD;

	if (textureFeed_ == L"")
		return Localise(L"COMMON/PROPERTY_LIST/FEED_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/FEED_ON");
}

PropertyItem::ItemType StringPropertyItem::getType() const
{
	if (readOnly_)
		return Type_String_ReadOnly;

	return Type_String;
}


//-----------------------------------------------------------------------------
//	Section: IDPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, ChangeDetectEdit*> IDPropertyItem::edit_;

IDPropertyItem::IDPropertyItem(const Name& name, const CString& currentValue)
	: PropertyItem( name )
{
	stringValue_ = currentValue;
}

IDPropertyItem::~IDPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(edit_[ parent_ ]);
	edit_.erase( parent_ );
}

void IDPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (edit_.find( parent_ ) == edit_.end())
	{
		edit_[ parent_ ] = new ChangeDetectEdit();

		CRect rect(10, 10, 10, 10);
		DWORD style = ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER | EM_SETREADONLY;
		edit_[ parent_ ]->Create(style, rect, parent, IDC_PROPERTYLIST_STRING);

		CFont* pFont = parent->GetParent()->GetFont();
		edit_[ parent_ ]->SetFont(pFont);
	}
}

void IDPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()))
		return;

	edit_[ parent_ ]->SendMessage(EM_SETREADONLY, TRUE, 0);

	// display edit box
	rect.bottom -= 1;		// keep it inside the selection
	edit_[ parent_ ]->MoveWindow(rect);

	if (this->multipleValue())
	{
		edit_[ parent_ ]->SetWindowText( L"" );
		edit_[ parent_ ]->touched( false );
	}
	else
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}
	edit_[ parent_ ]->ShowWindow(SW_SHOW);
	edit_[ parent_ ]->SetFocus();
	edit_[ parent_ ]->SetSel(0, -1);
	registerViewHook( edit_[ parent_ ]->GetSafeHwnd(), parent_ );

	parent_->selectedItem( this );
}

void IDPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((edit_.find(parent_) == edit_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	CString newStr;
	edit_[ parent_ ]->GetWindowText(newStr);
	if ((!this->multipleValue() && stringValue_ != newStr) ||
		(this->multipleValue() && edit_[ parent_ ]->touched()))
	{
		stringValue_ = newStr;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	edit_[ parent_ ]->ShowWindow(SW_HIDE);
	edit_[ parent_ ]->Invalidate();

	parent_->selectedItem( NULL );
}

void IDPropertyItem::set(const BW::wstring& value)
{
	BW_GUARD;

	stringValue_ = value.c_str();

	if (parent_ && parent_->selectedItem() == this && edit_.find(parent_) != edit_.end())
	{
		edit_[ parent_ ]->SetWindowText(stringValue_);
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
	}
}

BW::wstring IDPropertyItem::get() const
{
	BW_GUARD;
	return (LPCTSTR)stringValue_;
}

PropertyItem::ItemType IDPropertyItem::getType() const
{
	return Type_ID;
}


//-----------------------------------------------------------------------------
//	Section: ComboPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, CComboBox*> ComboPropertyItem::comboBox_;

ComboPropertyItem::ComboPropertyItem(const Name& name, CString currentValue,
	const BW::vector<BW::wstring>& possibleValues)
	: PropertyItem( name )
	, possibleValues_( possibleValues )
{
	BW_GUARD;

//	MF_ASSERT( std::find( possibleValues_.begin(), possibleValues_.end(), currValue ) != possibleValues_.end() );
	stringValue_ = currentValue;
}

ComboPropertyItem::ComboPropertyItem(const Name& name, int currentValueIndex,
	const BW::vector<BW::wstring>& possibleValues)
	: PropertyItem( name )
	, possibleValues_( possibleValues )
{
	BW_GUARD;

	MF_ASSERT( currentValueIndex < (int) possibleValues_.size() );
	stringValue_ = possibleValues_[currentValueIndex].c_str();
}

ComboPropertyItem::~ComboPropertyItem()
{
	BW_GUARD;

	if (comboBox_[ parent_ ])
	{
		comboBox_[ parent_ ]->DestroyWindow();
		bw_safe_delete(comboBox_[ parent_ ]);
	}
	comboBox_.erase( parent_ );
}

void ComboPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (comboBox_.find( parent_ ) == comboBox_.end())
	{
		comboBox_[ parent_ ] = new CComboBox();

		CRect rect(10, 10, 10, 10);
		comboBox_[ parent_ ]->Create(CBS_DROPDOWNLIST | CBS_DISABLENOSCROLL |  
					WS_CHILD | WS_BORDER | WS_VSCROLL,
					rect, parent, IDC_PROPERTYLIST_LIST);

		CFont* pFont = parent->GetParent()->GetFont();
		comboBox_[ parent_ ]->SetFont(pFont);
	}
}

void ComboPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((comboBox_.find(parent_) == comboBox_.end()))
		return;
	
	// add the possible values and get the size of the longest
	int biggestCX = 0;
	CDC* pDC = comboBox_[ parent_ ]->GetDC();

	comboBox_[ parent_ ]->ResetContent();
	for (BW::vector<BW::wstring>::iterator it = possibleValues_.begin();
		it != possibleValues_.end();
		it++)
	{
		comboBox_[ parent_ ]->AddString(it->c_str());

		CSize stringSize = pDC->GetTextExtent(it->c_str());
		if (stringSize.cx > biggestCX)
			biggestCX = stringSize.cx;
	}

	comboBox_[ parent_ ]->ReleaseDC(pDC);

	rect.top -= 2;
	rect.bottom += 256;	// extend the bottom to fit a reasonable number of choices

	// extend the left to fit all the text
	int widthDiff = biggestCX - rect.Width();
	rect.left -= widthDiff > 0 ? widthDiff : 0; 
	comboBox_[ parent_ ]->MoveWindow(rect);

	comboBox_[ parent_ ]->SelectString(-1, stringValue_);
	comboBox_[ parent_ ]->ShowWindow(SW_SHOW);
	comboBox_[ parent_ ]->SetFocus();
	if (showDropDown)
		comboBox_[ parent_ ]->ShowDropDown();
	
	registerViewHook( comboBox_[ parent_ ]->GetSafeHwnd(), parent_ );

	parent_->selectedItem( this );
}

void ComboPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((comboBox_.find(parent_) == comboBox_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	// hide the list
	comboBox_[ parent_ ]->ShowWindow(SW_HIDE);
	comboBox_[ parent_ ]->Invalidate();

	parent_->selectedItem( NULL );
}


void ComboPropertyItem::set(const BW::wstring& value)
{
	BW_GUARD;

	BW::vector<BW::wstring>::iterator it = std::find( possibleValues_.begin(),
											possibleValues_.end(),
											value);
	// validate string
	if (it == possibleValues_.end())
		return;

	stringValue_ = it->c_str();

	if (parent_ && parent_->selectedItem() == this && comboBox_.find(parent_) != comboBox_.end())
	{
		comboBox_[ parent_ ]->SelectString(-1, stringValue_);
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
	}
}

void ComboPropertyItem::set(int index)
{
	BW_GUARD;

	// validate index
	if (index >= (int)(possibleValues_.size()))
		return;

	stringValue_ = possibleValues_.at(index).c_str();

	if (parent_ && parent_->selectedItem() == this && comboBox_.find(parent_) != comboBox_.end())
	{
		comboBox_[ parent_ ]->SelectString(-1, stringValue_);
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
	}
}

BW::wstring ComboPropertyItem::get() const
{
	BW_GUARD;
	return (LPCTSTR)stringValue_;
}

void ComboPropertyItem::comboChange()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((comboBox_.find(parent_) == comboBox_.end()))
		return;
	comboBox_[ parent_ ]->GetLBText(comboBox_[ parent_ ]->GetCurSel(), stringValue_);
	parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
}


//-----------------------------------------------------------------------------
//	Section: BoolPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, CComboBox*> BoolPropertyItem::comboBox_;

BoolPropertyItem::BoolPropertyItem(const Name& name, int currentValue)
	: PropertyItem( name )
{
	BW_GUARD;

	value_ = currentValue;
}

BoolPropertyItem::~BoolPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(comboBox_[ parent_ ]);
	comboBox_.erase( parent_ );
}

void BoolPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	static const wchar_t * BOOL_STRINGS[] = { L"False", L"True" };

	if (comboBox_.find( parent_ ) == comboBox_.end())
	{
		comboBox_[ parent_ ] = new CComboBox();

		CRect rect(10, 10, 10, 10);
		comboBox_[ parent_ ]->Create(CBS_DROPDOWNLIST | CBS_NOINTEGRALHEIGHT |  
					WS_CHILD | WS_BORDER,
					rect, parent, IDC_PROPERTYLIST_BOOL);

		CFont* pFont = parent->GetParent()->GetFont();
		comboBox_[ parent_ ]->SetFont(pFont);

		// add the possible values
		comboBox_[ parent_ ]->ResetContent();
		comboBox_[ parent_ ]->InsertString( 0, BOOL_STRINGS[0] );
		comboBox_[ parent_ ]->InsertString( 1, BOOL_STRINGS[1] );
	}

	// initialise the stringValue_
	comboBox_[ parent_ ]->SetCurSel( value_ );
	stringValue_ = BOOL_STRINGS[ value_ ];
}

void BoolPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((comboBox_.find(parent_) == comboBox_.end()))
		return;
	
	rect.top -= 2;
	rect.bottom += 256;	// extend the bottom to fit in all the choices
	comboBox_[ parent_ ]->MoveWindow(rect);

	comboBox_[ parent_ ]->SetCurSel(value_);
	comboBox_[ parent_ ]->GetLBText(value_, stringValue_);

	comboBox_[ parent_ ]->SetFocus();
	comboBox_[ parent_ ]->ShowWindow(SW_SHOW);
	if (showDropDown)
		comboBox_[ parent_ ]->ShowDropDown();

	registerViewHook( comboBox_[ parent_ ]->GetSafeHwnd(), parent_ );

	parent_->selectedItem( this );
}

void BoolPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((comboBox_.find(parent_) == comboBox_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	// hide the list
	comboBox_[ parent_ ]->ShowWindow(SW_HIDE);
	comboBox_[ parent_ ]->Invalidate();

	parent_->selectedItem( NULL );
}


void BoolPropertyItem::set(bool value)
{
	BW_GUARD;

	value_ = (int)value;

	if (parent_ && comboBox_.find(parent_) != comboBox_.end())
	{
		comboBox_[ parent_ ]->GetLBText(value_, stringValue_);

		if (parent_->selectedItem() == this)
		{
			comboBox_[ parent_ ]->SetCurSel(value_);
		}
	}

	// force the list to redraw
	if (parent_)
	{
		parent_->Invalidate();
	}
}

bool BoolPropertyItem::get() const
{
	BW_GUARD;

	if (parent_ && parent_->selectedItem() == this && comboBox_.find(parent_) != comboBox_.end())
		return comboBox_[ parent_ ]->GetCurSel() != 0;
	else
		return value_ != 0;
}

void BoolPropertyItem::comboChange()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((comboBox_.find(parent_) == comboBox_.end()))
		return;
	
	// update data if required
	CString newValue;
	comboBox_[ parent_ ]->GetLBText(comboBox_[ parent_ ]->GetCurSel(), newValue);
	if (newValue != stringValue_)
	{
		value_ = comboBox_[ parent_ ]->GetCurSel();
		stringValue_ = newValue;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}
}

/*virtual*/ BW::wstring BoolPropertyItem::menuOptions()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_ON");
}


//-----------------------------------------------------------------------------
//	Section: FloatPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, controls::EditNumeric*> FloatPropertyItem::editNumeric_;
/*static*/ BW::map<CWnd*, controls::Slider*> FloatPropertyItem::slider_;
/*static*/ BW::map<CWnd*, CButton*> FloatPropertyItem::button_;

FloatPropertyItem::FloatPropertyItem(const Name& name, float currentValue)
	: PropertyItem( name )
	, value_( currentValue )
	, min_( -FLT_MAX )
	, max_( FLT_MAX )
	, digits_( 3 )
	, ranged_( false )
	, changing_( false )
	, hasDef_( false )
{
}

FloatPropertyItem::~FloatPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(editNumeric_[ parent_ ]);
	editNumeric_.erase( parent_ );
	bw_safe_delete(slider_[ parent_ ]);
	slider_.erase( parent_ );
	bw_safe_delete(button_[ parent_ ]);
	button_.erase( parent_ );
}


void FloatPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;
	
	if (editNumeric_.find( parent_ ) == editNumeric_.end())
	{
		editNumeric_[ parent_ ] = new controls::EditNumeric();

		CRect rect(10, 10, 10, 10);
		editNumeric_[ parent_ ]->Create(ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER,
						rect, parent, IDC_PROPERTYLIST_FLOAT);

		CFont* pFont = parent->GetParent()->GetFont();
		editNumeric_[ parent_ ]->SetFont(pFont);
	}

	if( slider_.find( parent_ ) == slider_.end())
	{
		slider_[ parent_ ] = new controls::Slider();

		CRect rect(10, 10, 10, 10);
		slider_[ parent_ ]->Create(
			TBS_HORZ | TBS_NOTICKS | WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect,
			parent, IDC_PROPERTYLIST_SLIDER );
	}

	if(button_.find( parent_ ) == button_.end())
	{
		button_[ parent_ ] = new CButton();

		CRect rect(10, 10, 10, 10);
		DWORD style = BS_PUSHBUTTON | WS_CHILD | WS_BORDER;
		button_[ parent_ ]->Create( L"*", style, rect, parent, IDC_PROPERTYLIST_DEFAULT);

		CFont* pFont = parent->GetParent()->GetFont();
		button_[ parent_ ]->SetFont(pFont);
	}

	editNumeric_[ parent_ ]->SetNumDecimals( digits_ );
	editNumeric_[ parent_ ]->SetValue(value_);
	stringValue_ = editNumeric_[ parent_ ]->GetStringForm();
}

void FloatPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()) && (button_.find(parent_) == button_.end()))
		return;
	// display edit box
	rect.bottom -= 1;		// keep it inside the selection

	static const int MIN_SLIDER_WIDTH = 60;
	static const int EDIT_WIDTH = 40;
	const int BUTTON_WIDTH = hasDef_ ? 10 : 0;
	const int SLIDER_WIDTH = ( ranged_ == 0 || rect.Width() - BUTTON_WIDTH < EDIT_WIDTH + MIN_SLIDER_WIDTH ) ? 0 : rect.Width() - BUTTON_WIDTH - EDIT_WIDTH;

	rect.right -= BUTTON_WIDTH + SLIDER_WIDTH;
	editNumeric_[ parent_ ]->MoveWindow(rect);

	editNumeric_[ parent_ ]->SetMinimum(min_);
	editNumeric_[ parent_ ]->SetMaximum(max_);

	editNumeric_[ parent_ ]->SetNumDecimals( digits_ );
	if (this->multipleValue())
	{
		editNumeric_[ parent_ ]->SetValue( std::max( 0.0f, min_ ) );
		editNumeric_[ parent_ ]->touched( false );
	}
	else
	{
		editNumeric_[ parent_ ]->SetValue(value_);
	}
	stringValue_ = editNumeric_[ parent_ ]->GetStringForm();

	editNumeric_[ parent_ ]->ShowWindow(SW_SHOW);
	editNumeric_[ parent_ ]->SetFocus();
	editNumeric_[ parent_ ]->SetSel(0, -1);
	registerViewHook( editNumeric_[ parent_ ]->GetSafeHwnd(), parent_ );

	rect.left = rect.right;
	rect.right += SLIDER_WIDTH;
	if( SLIDER_WIDTH != 0 )
	{
		slider_[ parent_ ]->MoveWindow(rect);
		slider_[ parent_ ]->SetRange( (int)( min_ * pow( 10.f, digits_ ) ), (int)( max_ * pow( 10.f, digits_ ) ) );
		slider_[ parent_ ]->SetPos( (int)( value_ * pow( 10.f, digits_ ) ) );
		slider_[ parent_ ]->ShowWindow(SW_SHOW);
		slider_[ parent_ ]->ClearSel( true );
		registerViewHook( slider_[ parent_ ]->GetSafeHwnd(), parent_ );
	}

	rect.left = rect.right;
	rect.right += BUTTON_WIDTH;
	if( BUTTON_WIDTH != 0 )
	{
		button_[ parent_ ]->MoveWindow(rect);
		button_[ parent_ ]->ShowWindow(SW_SHOW);
		registerViewHook( button_[ parent_ ]->GetSafeHwnd(), parent_ );
	}

	parent_->selectedItem( this );
}

void FloatPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()) && (button_.find(parent_) == button_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	// this is called at the end of editing
	// store the value for next time
	CString newValue;
	editNumeric_[ parent_ ]->SetNumericText(TRUE);
	editNumeric_[ parent_ ]->GetWindowText(newValue);
	if ((!this->multipleValue() && newValue != stringValue_) ||
		(this->multipleValue() && editNumeric_[ parent_ ]->touched()))
	{
		value_ = editNumeric_[ parent_ ]->GetValue();
		stringValue_ = newValue;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	// close the edit window
	editNumeric_[ parent_ ]->SendMessage(EN_KILLFOCUS);
	editNumeric_[ parent_ ]->ShowWindow(SW_HIDE);
	editNumeric_[ parent_ ]->Invalidate();

	slider_[ parent_ ]->ShowWindow(SW_HIDE);
	slider_[ parent_ ]->Invalidate();

	button_[ parent_ ]->ShowWindow(SW_HIDE);
	button_[ parent_ ]->Invalidate();

	parent_->selectedItem( NULL );
}


void FloatPropertyItem::setDigits( int digits )
{
	digits_ = digits;
}


void FloatPropertyItem::setRange( float min, float max, int digits )
{
	ranged_ = true;
	min_ = min;
	max_ = max;
	digits_ = digits;
}

void FloatPropertyItem::setDefault( float def )
{
	hasDef_ = true;
	def_ = def;
}


void FloatPropertyItem::set(float value)
{
	BW_GUARD;

	value_ = value;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;

	editNumeric_[ parent_ ]->SetNumDecimals( digits_ );
	editNumeric_[ parent_ ]->GetStringFormOfValue( value, stringValue_ );

	if (parent_->selectedItem() == this)
		editNumeric_[ parent_ ]->SetValue(value);

	// force the list to redraw 
	// TODO: only redraw the particular item
	if (parent_)
		parent_->Invalidate();
}

float FloatPropertyItem::get() const
{
	BW_GUARD;

	if (parent_ && parent_->selectedItem() == this && editNumeric_.find(parent_) != editNumeric_.end())
		return editNumeric_[ parent_ ]->GetValue();
	else
		return value_; 
}

void FloatPropertyItem::sliderChange( int value, bool transient )
{
	BW_GUARD;

	if (!parent_) return; 
	if( !changing_ ) 
	{
		changing_ = true;
		set( (float)( value / pow( 10.f, digits_ )));
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, (WPARAM)transient, (LPARAM)changeBuddy_);
		changing_ = false;
	}
}

void FloatPropertyItem::onDefault()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	if( !changing_ ) 
	{
		changing_ = true;
		editNumeric_[ parent_ ]->SetValue((float)( def_/pow(10.f, digits_) ));
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
		if( ranged_ )
			slider_[ parent_ ]->SetPos( (int)( editNumeric_[ parent_ ]->GetValue() * pow( 10.f, digits_ ) ) );
		changing_ = false;
	}
}

void FloatPropertyItem::editChange()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	if( !changing_ ) 
	{
		changing_ = true;
		if( ranged_ )
			slider_[ parent_ ]->SetPos( (int)( editNumeric_[ parent_ ]->GetValue() * pow(10.f, digits_) ) );
		changing_ = false;
	}
}

/*virtual*/ BW::wstring FloatPropertyItem::menuOptions()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_ON");
}

/*virtual*/ controls::EditNumeric* FloatPropertyItem::ownEdit()
{
	BW_GUARD;

	if (editNumeric_.find( parent_ ) == editNumeric_.end())
		return NULL;
	return editNumeric_[parent_];
}


//-----------------------------------------------------------------------------
//	Section: IntPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, controls::EditNumeric*> IntPropertyItem::editNumeric_;
/*static*/ BW::map<CWnd*, controls::Slider*> IntPropertyItem::slider_;

IntPropertyItem::IntPropertyItem(const Name& name, int currentValue)
	: PropertyItem( name )
	, value_( currentValue )
	, min_( INT_MIN )
	, max_( INT_MAX )
	, ranged_( false )
	, changing_( false )
{
}

IntPropertyItem::~IntPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(editNumeric_[ parent_ ]);
	editNumeric_.erase( parent_ );
	bw_safe_delete(slider_[ parent_ ]);
	slider_.erase( parent_ );
}

void IntPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (editNumeric_.find( parent_ ) == editNumeric_.end())
	{
		editNumeric_[ parent_ ] = new controls::EditNumeric();

		editNumeric_[ parent_ ]->SetNumericType( controls::EditNumeric::ENT_INTEGER );

		CRect rect(10, 10, 10, 10);
		editNumeric_[ parent_ ]->Create(ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER,
						rect, parent, IDC_PROPERTYLIST_INT);

		CFont* pFont = parent->GetParent()->GetFont();
		editNumeric_[ parent_ ]->SetFont(pFont);
	}

	if (slider_.find( parent_ ) == slider_.end())
	{
		slider_[ parent_ ] = new controls::Slider();

		CRect rect(10, 10, 10, 10);
		slider_[ parent_ ]->Create(
			TBS_HORZ | TBS_NOTICKS | WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect,
			parent, IDC_PROPERTYLIST_SLIDER );
	}

	editNumeric_[ parent_ ]->SetIntegerValue(value_);
	stringValue_ = editNumeric_[ parent_ ]->GetStringForm();
}

void IntPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	// display edit box
	rect.bottom -= 1;		// keep it inside the selection

	static const int MIN_SLIDER_WIDTH = 60;
	static const int EDIT_WIDTH = 40;

	const bool rangeTooBig = (((float)max_ - (float)min_) > 65535.0f) ? true : false;
	const int SLIDER_WIDTH = ( ranged_ == 0 || rangeTooBig || rect.Width() < EDIT_WIDTH + MIN_SLIDER_WIDTH ) ? 0 : rect.Width() - EDIT_WIDTH;

	rect.right -= SLIDER_WIDTH;
	editNumeric_[ parent_ ]->MoveWindow(rect);

	editNumeric_[ parent_ ]->SetMinimum((float)min_);
	editNumeric_[ parent_ ]->SetMaximum((float)max_);

	if (this->multipleValue())
	{
		editNumeric_[ parent_ ]->SetIntegerValue( std::max( 0, min_ ) );
		editNumeric_[ parent_ ]->touched( false );
	}
	else
	{
		editNumeric_[ parent_ ]->SetIntegerValue(value_);
	}
	stringValue_ = editNumeric_[ parent_ ]->GetStringForm();

	editNumeric_[ parent_ ]->ShowWindow(SW_SHOW);
	editNumeric_[ parent_ ]->SetFocus();
	editNumeric_[ parent_ ]->SetSel(0, -1);
	registerViewHook( editNumeric_[ parent_ ]->GetSafeHwnd(), parent_ );

	rect.left = rect.right;
	rect.right += SLIDER_WIDTH;
	if( SLIDER_WIDTH != 0 )
	{
		slider_[ parent_ ]->MoveWindow(rect);
		slider_[ parent_ ]->SetRange( min_, max_ );
		slider_[ parent_ ]->SetPos( value_ );
		slider_[ parent_ ]->ShowWindow(SW_SHOW);
		registerViewHook( slider_[ parent_ ]->GetSafeHwnd(), parent_ );
	}

	parent_->selectedItem( this );
}

void IntPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	CString newValue = editNumeric_[ parent_ ]->GetStringForm();
	if ((!this->multipleValue() && newValue != stringValue_) ||
		(this->multipleValue() && editNumeric_[ parent_ ]->touched()))
	{
		value_ = editNumeric_[ parent_ ]->GetIntegerValue();
		stringValue_ = newValue;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	// kill the edit window
	editNumeric_[ parent_ ]->SendMessage(EN_KILLFOCUS);
	editNumeric_[ parent_ ]->ShowWindow(SW_HIDE);
	editNumeric_[ parent_ ]->Invalidate();

	slider_[ parent_ ]->ShowWindow(SW_HIDE);
	slider_[ parent_ ]->Invalidate();
	parent_->selectedItem( NULL );
}

void IntPropertyItem::setRange( int min, int max )
{
	ranged_ = true;
	min_ = min;
	max_ = max;
}

void IntPropertyItem::set(int value)
{
	BW_GUARD;

	value_ = value;

	if (!parent_) return; 
	if (editNumeric_.find(parent_) == editNumeric_.end())
		return;

	editNumeric_[ parent_ ]->GetStringFormOfValue( double( value ), stringValue_ );

	if (parent_->selectedItem() == this)
		editNumeric_[ parent_ ]->SetIntegerValue(value);

	// force the list to redraw 
	parent_->Invalidate();
}

int IntPropertyItem::get() const
{
	BW_GUARD;

	if (parent_ && parent_->selectedItem() == this && editNumeric_.find(parent_) != editNumeric_.end())
		return editNumeric_[ parent_ ]->GetIntegerValue();
	else
		return value_; 
}

void IntPropertyItem::sliderChange( int value, bool transient )
{
	BW_GUARD;

	if (!parent_) return; 
	if( !changing_ )
	{
		changing_ = true;
		set( value );
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, (WPARAM)transient, (LPARAM)changeBuddy_);
		changing_ = false;
	}
}

void IntPropertyItem::editChange()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	if( !changing_ )
	{
		changing_ = true;
		if( ranged_ )
			slider_[ parent_ ]->SetPos( editNumeric_[ parent_ ]->GetIntegerValue() );
		changing_ = false;
	}
}

/*virtual*/ BW::wstring IntPropertyItem::menuOptions()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_ON");
}

/*virtual*/ controls::EditNumeric* IntPropertyItem::ownEdit()
{
	BW_GUARD;

	if (editNumeric_.find( parent_ ) == editNumeric_.end())
		return NULL;
	return editNumeric_[parent_];
}


//-----------------------------------------------------------------------------
//	Section: UIntPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, controls::EditNumeric*> UIntPropertyItem::editNumeric_;
/*static*/ BW::map<CWnd*, controls::Slider*> UIntPropertyItem::slider_;

UIntPropertyItem::UIntPropertyItem(const Name& name, uint32 currentValue)
	: PropertyItem( name )
	, value_( currentValue )
	, min_( std::numeric_limits<uint32>::min() )
	, max_( std::numeric_limits<uint32>::max() )
	, ranged_( false )
	, changing_( false )
{
}

UIntPropertyItem::~UIntPropertyItem()
{
	BW_GUARD;

	bw_safe_delete(editNumeric_[ parent_ ]);
	editNumeric_.erase( parent_ );
	bw_safe_delete(slider_[ parent_ ]);
	slider_.erase( parent_ );
}

void UIntPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	parent_ = parent;

	if (editNumeric_.find( parent_ ) == editNumeric_.end())
	{
		editNumeric_[ parent_ ] = new controls::EditNumeric();

		editNumeric_[ parent_ ]->SetNumericType( controls::EditNumeric::ENT_INTEGER );

		CRect rect(10, 10, 10, 10);
		editNumeric_[ parent_ ]->Create(ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_BORDER,
						rect, parent, IDC_PROPERTYLIST_INT);

		CFont* pFont = parent->GetParent()->GetFont();
		editNumeric_[ parent_ ]->SetFont(pFont);
	}

	if (slider_.find( parent_ ) == slider_.end())
	{
		slider_[ parent_ ] = new controls::Slider();

		CRect rect(10, 10, 10, 10);
		slider_[ parent_ ]->Create(
			TBS_HORZ | TBS_NOTICKS | WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect,
			parent, IDC_PROPERTYLIST_SLIDER );
	}

	editNumeric_[ parent_ ]->SetUIntValue(value_);
	stringValue_ = editNumeric_[ parent_ ]->GetStringForm();
}

void UIntPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	// display edit box
	rect.bottom -= 1;		// keep it inside the selection

	static const int MIN_SLIDER_WIDTH = 60;
	static const int EDIT_WIDTH = 40;

	const bool rangeTooBig = (((float)max_ - (float)min_) > 65535.0f) ? true : false;
	const int SLIDER_WIDTH = ( ranged_ == 0 || rangeTooBig || rect.Width() < EDIT_WIDTH + MIN_SLIDER_WIDTH ) ? 0 : rect.Width() - EDIT_WIDTH;

	rect.right -= SLIDER_WIDTH;
	editNumeric_[ parent_ ]->MoveWindow(rect);

	editNumeric_[ parent_ ]->SetMinimum((float)min_);
	editNumeric_[ parent_ ]->SetMaximum((float)max_);

	if (this->multipleValue())
	{
		editNumeric_[ parent_ ]->SetUIntValue( std::max( uint32( 0 ), min_ ) );
		editNumeric_[ parent_ ]->touched( false );
	}
	else
	{
		editNumeric_[ parent_ ]->SetUIntValue(value_);
	}
	stringValue_ = editNumeric_[ parent_ ]->GetStringForm();

	editNumeric_[ parent_ ]->ShowWindow(SW_SHOW);
	editNumeric_[ parent_ ]->SetFocus();
	editNumeric_[ parent_ ]->SetSel(0, -1);
	registerViewHook( editNumeric_[ parent_ ]->GetSafeHwnd(), parent_ );

	rect.left = rect.right;
	rect.right += SLIDER_WIDTH;
	if( SLIDER_WIDTH != 0 )
	{
		slider_[ parent_ ]->MoveWindow(rect);
		slider_[ parent_ ]->SetRange( int(min_), int(max_) );
		slider_[ parent_ ]->SetPos( value_ );
		slider_[ parent_ ]->ShowWindow(SW_SHOW);
		registerViewHook( slider_[ parent_ ]->GetSafeHwnd(), parent_ );
	}

	parent_->selectedItem( this );
}

void UIntPropertyItem::deselect()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	if (parent_->selectedItem() != this)
		return;

	CString newValue = editNumeric_[ parent_ ]->GetStringForm();
	if ((!this->multipleValue() && newValue != stringValue_) ||
		(this->multipleValue() && editNumeric_[ parent_ ]->touched()))
	{
		value_ = editNumeric_[ parent_ ]->GetUIntValue();
		stringValue_ = newValue;
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, 0, (LPARAM)changeBuddy_);
	}

	// kill the edit window
	editNumeric_[ parent_ ]->SendMessage(EN_KILLFOCUS);
	editNumeric_[ parent_ ]->ShowWindow(SW_HIDE);
	editNumeric_[ parent_ ]->Invalidate();

	slider_[ parent_ ]->ShowWindow(SW_HIDE);
	slider_[ parent_ ]->Invalidate();
	parent_->selectedItem( NULL );
}

void UIntPropertyItem::setRange( uint32 min, uint32 max )
{
	ranged_ = true;
	min_ = min;
	max_ = max;
}

void UIntPropertyItem::set(uint32 value)
{
	BW_GUARD;

	value_ = value;

	if (!parent_) return; 
	if (editNumeric_.find(parent_) == editNumeric_.end())
		return;

	editNumeric_[ parent_ ]->GetStringFormOfValue( double( value ), stringValue_ );

	if (parent_->selectedItem() == this)
		editNumeric_[ parent_ ]->SetUIntValue(value);

	// force the list to redraw 
	parent_->Invalidate();
}

uint32 UIntPropertyItem::get() const
{
	BW_GUARD;

	if (parent_ && parent_->selectedItem() == this && editNumeric_.find(parent_) != editNumeric_.end())
		return editNumeric_[ parent_ ]->GetUIntValue();
	else
		return value_; 
}

void UIntPropertyItem::sliderChange( int32 value, bool transient )
{
	BW_GUARD;

	if (!parent_) return; 
	if( !changing_ )
	{
		changing_ = true;
		set( uint32( value ) );
		parent_->SendMessage(WM_CHANGE_PROPERTYITEM, (WPARAM)transient, (LPARAM)changeBuddy_);
		changing_ = false;
	}
}

void UIntPropertyItem::editChange()
{
	BW_GUARD;

	if (!parent_) return; 
	if ((editNumeric_.find(parent_) == editNumeric_.end()) && (slider_.find(parent_) == slider_.end()))
		return;
	if( !changing_ )
	{
		changing_ = true;
		if( ranged_ )
			slider_[ parent_ ]->SetPos( editNumeric_[ parent_ ]->GetUIntValue() );
		changing_ = false;
	}
}

/*virtual*/ BW::wstring UIntPropertyItem::menuOptions()
{
	BW_GUARD;

	if (exposedToScriptName_.empty())
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_OFF");
	else
		return Localise(L"COMMON/PROPERTY_LIST/PYTHON_ON");
}

/*virtual*/ controls::EditNumeric* UIntPropertyItem::ownEdit()
{
	BW_GUARD;

	if (editNumeric_.find( parent_ ) == editNumeric_.end())
		return NULL;
	return editNumeric_[parent_];
}


//-----------------------------------------------------------------------------
//	Section: ArrayPropertyItem
//-----------------------------------------------------------------------------

/*static*/ BW::map<CWnd*, CButton*> ArrayPropertyItem::addButton_;
/*static*/ BW::map<CWnd*, CButton*> ArrayPropertyItem::delButton_;


/**
 *	Constructor.
 *
 *	@param name		Name of the property item.
 *	@param str		Initial string value (not displayed at the moment)
 *	@param proxy	Proxy to use to manage the array's items.
 */
ArrayPropertyItem::ArrayPropertyItem(const Name& name, const Name& group, const CString& str, ArrayProxyPtr proxy)
	: GroupPropertyItem( group, 1 )
	, proxy_( proxy )
{
	BW_GUARD;

	stringValue_ = str;
}


/**
 *	Destructor
 */
ArrayPropertyItem::~ArrayPropertyItem()
{
	BW_GUARD;

	clear();
}


/**
 *	Creates the array's top list item.
 *
 *	@param parent	Parent window.
 */
void ArrayPropertyItem::create(PropertyList* parent)
{
	BW_GUARD;

	GroupPropertyItem::create( parent );
}


/**
 *	Called when the user selects the array's property item. It draws buttons
 *	that help in managing the array's items.
 *
 *	@param rect				screen rectangle available to the property item.
 *	@param showDropDown		not used.
 */
void ArrayPropertyItem::select( CRect rect, bool showDropDown /* = true */ )
{
	BW_GUARD;

	if (proxy_->readOnly())
		return;

	if ( addButton_.find( parent_ ) == addButton_.end() )
	{
		// build init values
		CRect rect( 0, 0, 1, 1 );
		DWORD style = BS_PUSHBUTTON | WS_CHILD;
		CFont* pFont = parent_->GetParent()->GetFont();

		// Create "Add Item" button
		CButton* addButton = new CButton();

		addButton->Create( _T( "+" ), style, rect, parent_, IDC_PROPERTYLIST_CUSTOM_MIN );
		addButton->SetFont(pFont);

		addButton_[ parent_ ] = addButton;

		// Create "Delete All Items" button
		CButton* delButton = new CButton();

		delButton->Create( _T( "-" ), style, rect, parent_, IDC_PROPERTYLIST_CUSTOM_MIN + 1 );
		delButton->SetFont(pFont);

		delButton_[ parent_ ] = delButton;
	}

	// set up the screen positions for the buttons
	int btnSize = rect.Height();
	rect.top -= 1;
	rect.bottom -= 1;
	rect.right -= btnSize;
	rect.left = rect.right - btnSize;
	
	// Position the "Add Item" button
	CButton* addButton = addButton_[ parent_ ];
	addButton->MoveWindow(rect);
	addButton->ShowWindow(SW_SHOW);
	addButton->SetWindowPos( &CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	addButton->SetFocus();

	// Position the "Delete All Items" button
	rect.left += btnSize;
	rect.right += btnSize;

	CButton* delButton = delButton_[ parent_ ];
	delButton->MoveWindow(rect);
	delButton->ShowWindow(SW_SHOW);
	delButton->SetWindowPos( &CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	delButton->SetFocus();

	registerViewHook( addButton->GetSafeHwnd(), parent_ );
	registerViewHook( delButton->GetSafeHwnd(), parent_ );

	// call the base class' implementation.
	GroupPropertyItem::select( rect, showDropDown );
}


/**
 *	Called when the property item is deselected by the user clicking something else.
 */
void ArrayPropertyItem::deselect()
{
	BW_GUARD;

	if ( parent_->selectedItem() != this )
		return;

	GroupPropertyItem::deselect();

	clear();
}


/**
 *	Event handler for the add/delete buttons.
 *
 *	@param nID	command id of the button pressed.
 */
void ArrayPropertyItem::onCustom( UINT nID )
{
	BW_GUARD;

	PropertyList* parent = parent_;
	int curSel = parent->GetCurSel();
	BaseView* view = (BaseView*)getChangeBuddy();

	if ( nID == IDC_PROPERTYLIST_CUSTOM_MIN )
	{
		view->addItem();
	}
	else if ( nID == IDC_PROPERTYLIST_CUSTOM_MIN + 1 )
	{
		view->delItems();
	}

	parent->selectItem( curSel );
}


/**
 *	Cleans up, destroy the add/delete buttons
 */
void ArrayPropertyItem::clear()
{
	BW_GUARD;

	if ( addButton_.find( parent_ ) != addButton_.end() )
	{
		bw_safe_delete(addButton_[ parent_ ]);
		addButton_.erase( parent_ );
		bw_safe_delete(delButton_[ parent_ ]);
		delButton_.erase( parent_ );
	}
}


//-----------------------------------------------------------------------------
//	Section: PropertyList
//-----------------------------------------------------------------------------

/*static*/ CFrameWnd* PropertyList::mainFrame_ = NULL;
/*static*/ PropertyList::PropertyLists PropertyList::s_propertyLists_;
/*static*/ CButton PropertyList::s_arrayDeleteButton_;


PropertyList::PropertyList( BasePropertyTable* propertyTable ):
	enabled_( true ),
	isSorted_( false ),
	selectedItem_( NULL ),
	needsDeselect_( false ),
	needsSelectPrevItem_(false),
	needsSelectNextItem_(false),
	dividerPos_( 0 ),
	dividerTop_( 0 ),
	dividerBottom_( 0 ),
	dividerLastX_( 0 ),
	dividerMove_( false ),
	cursorArrow_( NULL ),
	cursorSize_( NULL ),
	tooltipsEnabled_( false ),
	delayRedraw_( false ),
	propertyTable_( propertyTable )
{
	BW_GUARD;
	s_propertyLists_.insert( this );

	++s_viewControlsHookUsers;
	if (s_viewControlsHookUsers == 1)
	{
		s_viewControlsHook = SetWindowsHookEx( WH_CALLWNDPROC,
			viewControlsWndProcHook, (HINSTANCE)NULL, GetCurrentThreadId() );
		s_viewControlsHookWindows.clear();
	}
}

PropertyList::~PropertyList()
{
	BW_GUARD;
	--s_viewControlsHookUsers;
	if (s_viewControlsHookUsers == 0)
	{
		UnhookWindowsHookEx( s_viewControlsHook );
		s_viewControlsHook = NULL;
		s_viewControlsHookWindows.clear();
	}

	s_propertyLists_.erase( this );
}

void PropertyList::enable( bool enable )
{
	BW_GUARD;

	ModifyStyle( enable ? WS_DISABLED : 0, enable ? 0 : WS_DISABLED );
	enabled_ = enable;
	OnPaint();
}


void PropertyList::isSorted( bool sorted )
{
	isSorted_ = sorted;
}


void PropertyList::OnPaint()
{
	BW_GUARD;

	//We need to enable tooltips here since it must be inited first
	//and there is no "OnInitDialog" equivalent for a CListBox.
	if (!tooltipsEnabled_)
	{
		EnableToolTips( TRUE );
		tooltipsEnabled_ = true;
	}

	if (!enabled_)
	{
		CClientDC dc(this);
		CRect clientRect;
		GetClientRect( clientRect );
		CBrush brush( ::GetSysColor( COLOR_BTNFACE ));
		dc.FillRect( clientRect, &brush );
		ValidateRect( clientRect );
	}
	CListBox::OnPaint();
}

BEGIN_MESSAGE_MAP(PropertyList, CListBox)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelchange)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_WM_VSCROLL()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CBN_SELENDOK(IDC_PROPERTYLIST_LIST, OnComboChange)
	ON_CBN_SELENDOK(IDC_PROPERTYLIST_BOOL, OnComboChange)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_COMMAND( IDC_PROPERTYLIST_BROWSE, OnBrowse )
	ON_COMMAND( IDC_PROPERTYLIST_DEFAULT, OnDefault )
	ON_COMMAND_RANGE( IDC_PROPERTYLIST_CUSTOM_MIN, IDC_PROPERTYLIST_CUSTOM_MAX, OnCustom )
	ON_COMMAND( IDC_PROPERTYLIST_ARRAY_DEL, OnArrayDelete )
	ON_WM_HSCROLL()
	ON_EN_CHANGE( IDC_PROPERTYLIST_FLOAT, OnEditChange )
	ON_EN_CHANGE( IDC_PROPERTYLIST_INT, OnEditChange )

	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)

END_MESSAGE_MAP()


// PropertyList message handlers

void PropertyList::PreSubclassWindow() 
{
	BW_GUARD;

	dividerPos_ = 0;
	dividerMove_ = false;
	dividerTop_ = 0;
	dividerBottom_ = 0;
	dividerLastX_ = 0;

	selected_ = 0;

	cursorSize_ = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
	cursorArrow_ = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
}


void PropertyList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	BW_GUARD;

	lpMeasureItemStruct->itemHeight = 20;
}

const int childIndent = 16;

void PropertyList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	BW_GUARD;

	if (delayRedraw_ ||
		lpDrawItemStruct->itemID == -1 ||
		lpDrawItemStruct->itemData == NULL)
	{
		return;
	}

	UINT index = lpDrawItemStruct->itemID;
	PropertyItem* item = (PropertyItem*)lpDrawItemStruct->itemData;

	// draw two rectangles, one for the property lable, one for the value
	// (unless is a group)
	int nCrBackground, nCrText;
	if( (lpDrawItemStruct->itemAction | ODA_SELECT) && 
		(lpDrawItemStruct->itemState & ODS_SELECTED) &&
		(item->getType() != PropertyItem::Type_Group) )
	{
		nCrBackground = COLOR_HIGHLIGHT;
		nCrText = COLOR_HIGHLIGHTTEXT;
	}
	else
	{
		nCrBackground = COLOR_WINDOW;
		nCrText = COLOR_WINDOWTEXT;
	}
	COLORREF crBackground = ::GetSysColor(nCrBackground);
	COLORREF crText = ::GetSysColor(nCrText);

	CRect rectItem = lpDrawItemStruct->rcItem;

	CDC dc;
	dc.Attach( lpDrawItemStruct->hDC );
	dc.FillSolidRect(rectItem, crBackground);

	int border = 1;
	rectItem.right -= border;
	rectItem.left += border;
	rectItem.top += border;
	rectItem.bottom -= border;

	CRect rectLabel = lpDrawItemStruct->rcItem;
	CRect rectValue = lpDrawItemStruct->rcItem;
	CRect rectColour = lpDrawItemStruct->rcItem;

	rectLabel.left =  item->getGroupDepth() * childIndent;
	if (item->isGroupType())
	{
		rectLabel.left += childIndent;

		if ((item->getType() == PropertyItem::Type_Group))
		{
			// change the background colour
			nCrBackground = COLOR_ACTIVECAPTION;
			crBackground = ::GetSysColor(nCrBackground);
			dc.FillSolidRect(rectLabel, crBackground);

			crText = ::GetSysColor(COLOR_CAPTIONTEXT);
		}

		// + / -
		CRect rcSign(lpDrawItemStruct->rcItem);
		rcSign.top = (int)(0.5f * (rcSign.bottom - rcSign.top)) + rcSign.top;
		rcSign.bottom = rcSign.top;
		rcSign.right = rectLabel.left - (int)(childIndent * 0.5f);
		rcSign.left = rcSign.right;
		rcSign.InflateRect(5, 5, 7, 7);

		dc.DrawEdge( rcSign, EDGE_RAISED, BF_RECT );
		
		CPoint ptCenter( rcSign.CenterPoint() );
		ptCenter.x -= 1;
		ptCenter.y -= 1;

		CPen pen_( PS_SOLID, 1, crText );
		CPen* oldPen = dc.SelectObject( &pen_ );
		
		// minus		
		dc.MoveTo(ptCenter.x - 3, ptCenter.y);
		dc.LineTo(ptCenter.x + 4, ptCenter.y);

		GroupPropertyItem * gItem = (GroupPropertyItem *)(item);
		if (!gItem->getExpanded())
		{
			// plus
			dc.MoveTo(ptCenter.x, ptCenter.y - 3);
			dc.LineTo(ptCenter.x, ptCenter.y + 4);
		}

		dc.SelectObject( oldPen );

		if (item->getType() == PropertyItem::Type_Colour)
		{
			ColourPropertyItem * cItem = (ColourPropertyItem *)(item);
			
			if (dividerPos_ == 0)
				dividerPos_ = rectValue.Width() / 2;
			rectValue.left = dividerPos_;

			rectColour = rectValue;
			rectColour.left -= 1;
			rectColour.right = rectColour.left + rectColour.Height();
			rectColour.bottom -= 2;

			BW::wstring colour = cItem->get();

			if (colour != L"") 
			{
				int r,g,b,a;
				swscanf( colour.c_str(), L"%d , %d , %d , %d", &r, &g, &b, &a );
				dc.FillSolidRect( rectColour, RGB(r,g,b) );
			}

			rectLabel.right = rectValue.left - 1;
		}
		if (item->getType() == PropertyItem::Type_ColourScalar)
		{
			ColourScalarPropertyItem * cItem = (ColourScalarPropertyItem *)(item);
			
			if (dividerPos_ == 0)
				dividerPos_ = rectValue.Width() / 2;
			rectValue.left = dividerPos_;

			rectColour = rectValue;
			rectColour.left -= 1;
			rectColour.right = rectColour.left + rectColour.Height();
			rectColour.bottom -= 2;

			BW::string colour = cItem->get();

			if (colour != "") 
			{
				int r,g,b,a;
				sscanf( colour.c_str(), "%d , %d , %d , %d", &r, &g, &b, &a );
				dc.FillSolidRect( rectColour, RGB(r,g,b) );
			}

			rectLabel.right = rectValue.left - 1;
		}
		else if (item->getType() == PropertyItem::Type_Vector)
		{
			if (dividerPos_ == 0)
				dividerPos_ = rectValue.Width() / 2;
			rectValue.left = dividerPos_;

			rectLabel.right = rectValue.left - 1;
		}
	}
	else if (item->getType() == PropertyItem::Type_Label )
	{
		// do nothing
	}
	else if (item->getType() == PropertyItem::Type_Label_Highlight )
	{
		// use highlighting
		// change the background colour
		nCrBackground = COLOR_INACTIVECAPTIONTEXT;
		crBackground = ::GetSysColor(nCrBackground);
		dc.FillSolidRect(rectLabel, crBackground);
	}
	else
	{
		if (dividerPos_ == 0)
			dividerPos_ = rectValue.Width() / 2;
		rectValue.left = dividerPos_;
		rectLabel.right = rectValue.left - 1;
	}


	dc.DrawEdge( rectLabel, EDGE_ETCHED,BF_BOTTOMRIGHT );
	if (item->getType() != PropertyItem::Type_Group &&
			item->getType() != PropertyItem::Type_Label &&
			item->getType() != PropertyItem::Type_Label_Highlight)
		dc.DrawEdge( rectValue, EDGE_ETCHED,BF_BOTTOM );

	if (item->getType() == PropertyItem::Type_Colour ||
		item->getType() == PropertyItem::Type_ColourScalar )
	{
		rectValue.left = rectColour.right + 4;
	}

	border = 3;
	rectLabel.right -= border;
	rectLabel.left += border;
	rectLabel.top += border;
	rectLabel.bottom -= border;

	rectValue.right -= border;
	rectValue.left += border;
	rectValue.top += border;
	rectValue.bottom -= border;

	// write the property name in the first rectangle
	// value in the second rectangle
	COLORREF crOldBkColor = dc.SetBkColor(crBackground);
	COLORREF crOldTextColor = dc.SetTextColor(crText);

	CString name( bw_utf8tow( item->name().c_str() ).c_str() );
	DrawText( dc.GetSafeHdc(), name, -1, &rectLabel, DT_LEFT | DT_SINGLELINE);
	if (item->getType() != PropertyItem::Type_Group &&
			item->getType() != PropertyItem::Type_Label &&
			item->getType() != PropertyItem::Type_Label_Highlight)
		dc.DrawText(item->displayValue(), rectValue, DT_LEFT | DT_SINGLELINE);

	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);

	dc.Detach();
}


void PropertyList::OnLButtonUp(UINT nFlags, CPoint point) 
{
	BW_GUARD;

	if (dividerMove_)
	{
		// redraw columns for new divider position
		dividerMove_ = false;

		//if mouse was captured then release it
		if (GetCapture() == this)
			::ReleaseCapture();

		::ClipCursor(NULL);

		DrawDivider(point.x);	// remove the divider
		
		//set the divider position to the new value
		dividerPos_ = point.x + 2;

		//Do this to ensure that the value field(s) are moved to the new divider position
		selChange( false );
		
		//redraw
		Invalidate();
	}
	else
	{
		// select the item under the cursor
		BOOL out;
		UINT index = ItemFromPoint(point,out);
		if (!out && index != (uint16)-1)
			Select( index );

		CListBox::OnLButtonUp(nFlags, point);
	}
}


void PropertyList::OnLButtonDown(UINT nFlags, CPoint point) 
{
	BW_GUARD;

	BOOL out;
	int index = ItemFromPoint(point,out);

	CRect rect;
	GetItemRect( index, rect );

	//Determine the item
	PropertyItem* item = NULL;
	if ((!out) && (index != (uint16)(-1)) && (rect.PtInRect( point )))
		item = (PropertyItem*)GetItemDataPtr( index );

	if ((point.x >= dividerPos_ - 4) && (point.x <= dividerPos_ - 1))
	{
		// get ready to resize the divider
		::SetCursor(cursorSize_);

		// keep mouse inside the control
		CRect windowRect;
		GetWindowRect(windowRect);
		windowRect.left += 10; windowRect.right -= 10;
		::ClipCursor(windowRect);

		//Unselect the item first
		deselectCurrentItem();

		CRect clientRect;
		GetClientRect(clientRect);
		dividerMove_ = true;
		dividerTop_ = clientRect.top;
		dividerBottom_ = clientRect.bottom;
		dividerLastX_ = point.x;

		DrawDivider(dividerLastX_);

		//capture the mouse
		SetCapture();
		SetFocus();

		return;
	}

	// select the item
	CListBox::OnLButtonDown(nFlags, point);

	if (item)
	{
		if (item->getType() == PropertyItem::Type_Group)
		{
			int xBoundLower = item->getGroupDepth() * childIndent;
			int xBoundUpper = xBoundLower + childIndent;
			if (point.x >= xBoundLower  &&  point.x <= xBoundUpper)
			{
				GroupPropertyItem * gItem = (GroupPropertyItem *)(item);
				if (gItem->getExpanded())
					collapseGroup( gItem, index );
				else
					expandGroup( gItem, index );
			}
		}
		else if ((item->getType() == PropertyItem::Type_Colour) ||
				(item->getType() == PropertyItem::Type_ColourScalar) ||
				(item->getType() == PropertyItem::Type_Vector))
		{
			int xBoundLower = item->getGroupDepth() * childIndent;
			int xBoundUpper = xBoundLower + childIndent;
			//COLOURSCALAR - derive from ColourPropertyItem to get rid of this malarky
			if (item->getType() == PropertyItem::Type_ColourScalar)
			{
				if (point.x >= xBoundLower  &&  point.x <= xBoundUpper)
				{
					ColourScalarPropertyItem * cItem = (ColourScalarPropertyItem *)(item);
					if (cItem->getExpanded())
						collapseGroup( cItem, index );
					else
						expandGroup( cItem, index );
				}
			}
			else
			{
				if (point.x >= xBoundLower  &&  point.x <= xBoundUpper)
				{
					ColourPropertyItem * cItem = (ColourPropertyItem *)(item);
					if (cItem->getExpanded())
						collapseGroup( cItem, index );
					else
						expandGroup( cItem, index );
				}
			}
		}
	}

	dividerMove_ = false;
}


void PropertyList::OnMouseMove(UINT nFlags, CPoint point) 
{
	BW_GUARD;

	BOOL out;
	UINT index = ItemFromPoint(point,out);
	
	CRect rect;
	GetItemRect( index, rect );

	//Determine the item
	PropertyItem* item = NULL;
	if ((!out) && (index != (uint16)(-1)) && (rect.PtInRect( point )))
		item = (PropertyItem*)GetItemDataPtr( index );

	// handle the resizing divider
	if (dividerMove_)
	{
		//move divider line
		DrawDivider(dividerLastX_);	// remove old
		DrawDivider(point.x);			// draw new
		dividerLastX_ = point.x;
	}
	else if( item && (item->getType() != PropertyItem::Type_Group) &&
			(item->getType() != PropertyItem::Type_Label) &&
			(item->getType() != PropertyItem::Type_Label_Highlight) &&
			(point.x >= dividerPos_ - 4) && (point.x <= dividerPos_ - 1) )
	{
		//set the cursor to a sizing cursor if the cursor is over the row divider
		::SetCursor(cursorSize_);
	}
	else
	{
		CListBox::OnMouseMove(nFlags, point);
	}
}

void PropertyList::DrawDivider(int xpos)
{
	BW_GUARD;

	CClientDC dc(this);
	int nOldMode = dc.SetROP2(R2_NOT);	// draw inverse of screen colour
	dc.MoveTo(xpos, dividerTop_);
	dc.LineTo(xpos, dividerBottom_);
	dc.SetROP2(nOldMode);
}

void PropertyList::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	setDividerPos( (cx - 12) / 2 );

	if (selectedItem())
	{
		//Do this to ensure that the value field(s) are resized to the new size
		selChange( false );
		
		//Redraw
		Invalidate();
	}
	CListBox::OnSize( nType, cx, cy );
}

void PropertyList::establishGroup( PropertyItem* item )
{
	BW_GUARD;

	BW::string group = item->getGroup().str();

	// build current group
	BW::string currentGroup;
	for (uint i = 0; i < parentGroupStack_.size(); i++)
	{
		currentGroup += parentGroupStack_[i]->name().str();
		currentGroup += "/";
	}
	if (!currentGroup.empty())
		currentGroup.erase( --currentGroup.end() );

	if (currentGroup == group)
		return;

	if (group.empty())
	{
		parentGroupStack_.clear();
		return;
	}

	if (parentGroupStack_.size() == 0)
	{
		makeSubGroup( group, item );
		return;
	}
	
	// look forward from the root
	uint stackIndex = 0;
	BW::string::size_type startIndex = 0;
	BW::string::size_type endIndex = group.find( "/" );
	bool stackTooBig = false;
	bool stackTooSmall = false;
	while (!stackTooBig && !stackTooSmall)
	{
		BW::string name = group.substr( startIndex, 
								endIndex == -1 ? -1 : endIndex - startIndex );
		if (name != parentGroupStack_[stackIndex]->name())
		{
			stackTooBig = true;
			stackTooSmall = true;
		}
		else
		{
			if (endIndex == -1)
				stackTooBig = true;

			startIndex = endIndex + 1;
			endIndex = group.find( "/", startIndex );
		}

		if (++stackIndex == parentGroupStack_.size())
		{
			stackTooSmall = true;
		}
	}

	if (stackTooBig)
	{
		// pop things off the stack
		MF_ASSERT( parentGroupStack_.size() > 0 );
		if( stackTooSmall )
			stackIndex--;
		int diff = static_cast<int>(parentGroupStack_.size() - stackIndex);
		for (int i = 0; i < diff; i++)
			parentGroupStack_.pop_back();
	}

	if (stackTooSmall)
	{
		// make new subgroups
		BW::string subGroup = group.substr( startIndex );
		makeSubGroup( subGroup, item );
	}
}

void PropertyList::makeSubGroup( const BW::string & subGroup, PropertyItem* item )
{
	BW_GUARD;

	BW::string::size_type startIndex = 0;
	BW::string::size_type endIndex = subGroup.find( "/" );
	while (endIndex != BW::string::npos)
	{
		addGroupToStack( Name( subGroup.substr( startIndex, endIndex - startIndex ) ) );
		startIndex = endIndex + 1;
		endIndex = subGroup.find( "/", startIndex );
	} 
	addGroupToStack( Name( subGroup.substr( startIndex ) ), item );
}

void PropertyList::addGroupToStack( const Name& label, PropertyItem* item )
{
	BW_GUARD;

	int groupDepth = static_cast<int>(parentGroupStack_.size());

	GroupPropertyItem * newItem = NULL;
	bool needsInsert = false;
	if (item && item->getType() == PropertyItem::Type_Group)
	{
		MF_ASSERT( label == item->name() );
		newItem = static_cast<GroupPropertyItem*>( item );
		newItem->setGroupDepth( groupDepth );
	}
	else
	{
		newItem = new GroupPropertyItem( label, groupDepth );

		needsInsert = true;
	}

	if (parentGroupStack_.size() > 0)
		parentGroupStack_.back()->addChild( newItem );
	parentGroupStack_.push_back( newItem );

	if (needsInsert)
	{
		int index = -1;
		if (isSorted_)
		{
			index = findSortedIndex( newItem );
		}
		index = InsertString( index, L"" );
		SetItemDataPtr( index, newItem );
		newItem->create( this );
	}
}


void PropertyList::setArrayProperty( PropertyItem* item )
{
	BW_GUARD;

	if ( !arrayCallback_.empty() )
	{
		item->arrayData( arrayIndex_.top(), arrayCallback_.top().getObject(), arrayReadOnly_.top() );
		++arrayIndex_.top();
	}
}


int PropertyList::AddPropItem( PropertyItem * item, int forceIndex )
{
	BW_GUARD;

	establishGroup( item );

	if (parentGroupStack_.size() > 0 &&
		item->getType() != PropertyItem::Type_Group)
	{
		parentGroupStack_.back()->addChild( item );
	}

	if (!arrayCallback_.empty() &&
		arrayPropertItemIndex_.top() != -1 &&
		forceIndex == -1)
	{
		forceIndex = arrayPropertItemIndex_.top();
		++arrayPropertItemIndex_.top();
	}

	int index = -1;
	if (forceIndex > -1)
	{
		if (forceIndex < this->GetCount())
		{
			index = forceIndex;
		}
	}
	else if (isSorted_)
	{
		index = findSortedIndex( item );
	}
	index = InsertString( index, L"" );
	SetItemDataPtr(index, item);
	item->create(this);

	if ( !arrayCallback_.empty() )
	{
		if (arrayPropertItemIndex_.top() == -1)
		{
			arrayPropertItemIndex_.top() = index + 1;
		}
	}

	if( GetCurSel() == LB_ERR )
		SetCurSel( index );
	return index;
}


int PropertyList::findSortedIndex( PropertyItem * item ) const
{
	BW_GUARD;

	// find the correct insertion point from the bottom.
	int index = this->GetCount() - 1;
	for( ;index >= 0; index--)
	{
		PropertyItem * listItem =
				static_cast<PropertyItem*>( this->GetItemDataPtr( index ) );

		if (item->fullName() != listItem->fullName())
		{
			index++;
			if (index == this->GetCount())
			{
				index = -1;
			}
			break;
		}
	}
	return index;
}


void PropertyList::Select(int selected)
{
	selected_ = selected;
}


void PropertyList::OnSelchange() 
{
	BW_GUARD;

	selChange( true );
}

void PropertyList::selChange( bool showDropDown, bool changeFocus )
{
	BW_GUARD;

	selected_ = GetCurSel();
	CRect rect;
	GetItemRect( selected_, rect );
	rect.left = dividerPos_;

	deselectCurrentItem();

	if (selected_ != -1)
	{
		PropertyItem* item = (PropertyItem*) GetItemDataPtr( selected_ );
		if ((item) && (item->getSelectable()))
		{
			if (item->arrayCallback() != NULL && !item->arrayReadOnly())
			{
				rect.right -= rect.Height();
			}
			
			if (changeFocus)
			{
				item->select( rect, showDropDown );
			}
			GetParent()->SendMessage(WM_SELECT_PROPERTYITEM, 0, (LPARAM)(item->getChangeBuddy()));

			if (item->arrayCallback() != NULL && !item->arrayReadOnly())
			{
				CRect butRect( 0, 0, 1, 1 );
				if ( !s_arrayDeleteButton_.GetSafeHwnd() )
				{
					s_arrayDeleteButton_.Create(
						L"-",
						BS_PUSHBUTTON | WS_CHILD,
						butRect,
						this,
						IDC_PROPERTYLIST_ARRAY_DEL );
					s_arrayDeleteButton_.SetFont( GetParent()->GetFont() );
				}
				butRect.left = rect.right;
				butRect.right = butRect.left + rect.Height();
				butRect.top = rect.top;
				butRect.bottom = rect.bottom;
				s_arrayDeleteButton_.MoveWindow(butRect);
				s_arrayDeleteButton_.ShowWindow(SW_SHOW);
				s_arrayDeleteButton_.SetWindowPos( &CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
				registerViewHook( s_arrayDeleteButton_.GetSafeHwnd(), this );
			}
		}
		else
		{
			GetParent()->SendMessage(WM_SELECT_PROPERTYITEM, 0, 0);
		}
	}
}

void PropertyList::clear()
{
	BW_GUARD;

	if (GetSafeHwnd() == NULL)
		return;

	// tell current item it is not selected
	deselectCurrentItem();

	// empty the list box
	ResetContent();
	selected_ = -1;
	parentGroupStack_.clear();

	GetParent()->SendMessage(WM_SELECT_PROPERTYITEM, 0, 0);
}


afx_msg LRESULT PropertyList::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	GetParent()->SendMessage(WM_CHANGE_PROPERTYITEM, wParam, lParam);
	return 0;
}

bool PropertyList::changeSelectItem( int delta )
{
	BW_GUARD;

	if (selected_ == -1)
		return false;

	if (GetCount() == 0)
		return false;

	int newItem = selected_;
	newItem += delta;

	if (newItem < 0)
	{
		deselectCurrentItem();
		Select( -1 );
		return false;
	}

	if (newItem >= GetCount())
	{
		deselectCurrentItem();
		Select( -1 );
		return false;
	}

	Select( newItem );

	SetCurSel(selected_);
	selChange( true );

	return true;
}

bool PropertyList::selectItem( int itemIndex, bool changeFocus )
{
	BW_GUARD;

	if (GetCount() == 0)
		return false;

	if (itemIndex < 0)
	{
		deselectCurrentItem();
		Select( -1 );
		return false;
	}

	if (itemIndex >= GetCount())
	{
		deselectCurrentItem();
		Select( -1 );
		return false;
	}

	Select( itemIndex );

	SetCurSel(selected_);
	selChange( true, changeFocus );

	return true;
}


void PropertyList::deselectCurrentItem()
{
	BW_GUARD;

	if (selectedItem())
	{
		selectedItem()->deselect();
	}

	if ( s_arrayDeleteButton_.GetSafeHwnd() != NULL )
	{
		s_arrayDeleteButton_.ShowWindow(SW_HIDE);
		s_arrayDeleteButton_.DestroyWindow();
	}

	needsDeselect_ = false;
}


/*static*/ void PropertyList::deselectAllItems()
{
	BW_GUARD;

	for (PropertyLists::iterator it = s_propertyLists_.begin();
		it != s_propertyLists_.end(); ++it)
	{
		(*it)->deselectCurrentItem();
	}
}


/*static*/ void PropertyList::update()
{
	BW_GUARD;

	for (PropertyLists::iterator it = s_propertyLists_.begin();
		it != s_propertyLists_.end(); ++it)
	{
		if ((*it)->needsDeselect_)
		{
			(*it)->deselectCurrentItem();
		}
		if ((*it)->needsSelectPrevItem_)
		{
			(*it)->selectPrevItem();
			(*it)->needsSelectPrevItem_ = false;
		}
		if ((*it)->needsSelectNextItem_)
		{
			(*it)->selectNextItem();
			(*it)->needsSelectNextItem_ = false;
		}
	}
}


void PropertyList::selectPrevItem()
{
	BW_GUARD;

	if (GetCount() == 0)
		return;

	int nextItem = selected_;
	if( nextItem == 0)
		nextItem = GetCount() - 1;
	else
		--nextItem;
	Select( nextItem );

	SetCurSel(selected_);
	OnSelchange();
}

void PropertyList::selectNextItem()
{
	BW_GUARD;

	if (GetCount() == 0)
		return;

	int nextItem = selected_;
	nextItem += 1;
	if (nextItem >= GetCount())
		nextItem = 0;
	Select( nextItem );

	SetCurSel(selected_);
	OnSelchange();
}

void PropertyList::setDividerPos( int x )
{
	dividerPos_ = x;
}

void PropertyList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	CListBox::OnVScroll(nSBCode, nPos, pScrollBar);

	//Force a redraw
	Invalidate();
}

void PropertyList::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	if (pScrollBar->GetDlgCtrlID() == IDC_PROPERTYLIST_SLIDER)
	{
		int pos = ((CSliderCtrl*)pScrollBar)->GetPos();
		bool transient = (nSBCode != TB_ENDTRACK);
		if (selectedItem())
		{
			selectedItem()->sliderChange( pos, transient );
		}
	}
	else
	{
		CListBox::OnHScroll(nSBCode, nPos, pScrollBar);
	}
}

void PropertyList::OnSetFocus(CWnd* pOldWnd)
{
	BW_GUARD;

	if ((selected_ == -1) && (GetAsyncKeyState(VK_TAB) < 0))
	{
		if (GetAsyncKeyState(VK_SHIFT) < 0)
		{
			Select( GetCount() - 1 );
		}
		else
		{
			Select( 0 );
		}
			
		SetCurSel( selected_ );
		selChange( true );
	}

	CListBox::OnSetFocus( pOldWnd );
}

void PropertyList::OnKillFocus(CWnd* pNewWnd)
{
	BW_GUARD;

	deselectCurrentItem();

	CListBox::OnKillFocus( pNewWnd );
}

void PropertyList::OnComboChange()
{
	BW_GUARD;

	if (selectedItem())
	{
		selectedItem()->comboChange();
	}
}

void PropertyList::OnBrowse()
{
	BW_GUARD;

	if (selectedItem())
	{
		selectedItem()->onBrowse();
	}
}

void PropertyList::OnDefault()
{
	BW_GUARD;

	if (selectedItem())
	{
		selectedItem()->onDefault();
	}
}


/**
 *	Forwards messages with id between IDC_PROPERTYLIST_CUSTOM_MIN and 
 *	IDC_PROPERTYLIST_CUSTOM_MAX to the property item
 *
 *	@param nID	id of the command, between IDC_PROPERTYLIST_CUSTOM_MIN and MAX
 */
void PropertyList::OnCustom(UINT nID)
{
	BW_GUARD;

	PropTableSetter pts( propertyTable_ );

	if (selectedItem())
	{
		selectedItem()->onCustom(nID);
	}
}


/**
 *	Notifies the array property through the arrayCallback that the array
 *	"Delete Item" button ahs been pressed in one of the array items.
 */
void PropertyList::OnArrayDelete()
{
	BW_GUARD;

	PropertyItem* item = selectedItem();
	if ( item && item->arrayCallback() != NULL )
	{
		BaseView* view = (BaseView*)item->getChangeBuddy();

		int curSel = GetCurSel();
		view->delItem( item->arrayIndex() );
		selectItem( curSel );
	}
}


void PropertyList::OnEditChange( )
{
	BW_GUARD;

	if (selectedItem())
	{
		selectedItem()->editChange();
	}
}

PropertyItem * PropertyList::getHighlightedItem()
{
	BW_GUARD;

	if (selected_ == -1) return NULL;
	void* pItem =  GetItemDataPtr(selected_);

	if (pItem == reinterpret_cast<void*>(-1)) return NULL;

	return static_cast<PropertyItem*>(pItem);
}

void PropertyList::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	BW_GUARD;

	BOOL out;
	int index = ItemFromPoint(point,out);
	PropertyItem* item = 0;
	if (!out && index != (uint16)(-1))
	{
		item = (PropertyItem*) GetItemDataPtr(index);
	}
	
	GetParent()->SendMessage(WM_DBLCLK_PROPERTYITEM, 0, (LPARAM)item);
}

void PropertyList::OnRButtonUp( UINT, CPoint point )
{
	BW_GUARD;

	BOOL out;
	int index = ItemFromPoint(point,out);
	PropertyItem* item = 0;
	if (!out && index != (uint16)(-1))
	{
		Select( index );
		SetCurSel( selected_ );
		selChange( false );
		item = (PropertyItem*) GetItemDataPtr(index);
	}

	GetParent()->SendMessage(WM_RCLK_PROPERTYITEM, 0, (LPARAM)item);
}

/*afx_msg*/ HBRUSH PropertyList::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CListBox::OnCtlColor( pDC, pWnd, nCtlColor );

	
	PropertyItem * item = getHighlightedItem();
	if (item)
	{
		controls::EditNumeric* edit = item->ownEdit();

		if (edit)
		{
			if (edit->isRanged())
			{
				edit->SetBoundsColour( pDC, pWnd, edit->GetMinimum(), edit->GetMaximum() );
			}
		}
	}

	return brush;
}


void PropertyList::collapseGroup(GroupPropertyItem* gItem, int index, bool reselect)
{
	BW_GUARD;

	if (!gItem->getExpanded())
		return;

	delayRedraw_ = true;
	
	PropertyItemVector & children = gItem->getChildren();

	// hide all the children
	for (PropertyItemVector::iterator it = children.begin();
		it != children.end();
		it++)
	{
		// remove all of our children
		for (int i = 0; i < this->GetCount(); ++i)
		{
			PropertyItem * listItem =
					static_cast<PropertyItem*>( this->GetItemDataPtr( i ) );
			if (listItem == (*it))
			{
				DeleteString( i );
				break;
			}
		}

		// remove all of our children's children
		if ((*it)->isGroupType()) 
		{
			GroupPropertyItem * g = (GroupPropertyItem *)(*it);
			collapseGroup( g, index + 1 );
		}
	}

	gItem->setExpanded( false );
	if (reselect)
	{
		selectItem( index );
	}
	delayRedraw_ = false;
}

void PropertyList::expandGroup(GroupPropertyItem* gItem, int index)
{
	BW_GUARD;

	if (gItem->getExpanded())
		return;

	delayRedraw_ = true;

	PropertyItemVector & children = gItem->getChildren();

	// show all the children (one 1 level)
	for (PropertyItemVector::iterator it = children.begin();
		it != children.end(); ++it)
	{
		// find sorted insertion index and insert
		int insertIdx = index + 1;
		if (this->isSorted_ &&
			gItem->getType() != PropertyItem::Type_Colour &&
			gItem->getType() != PropertyItem::Type_Vector)
		{
			for ( ; insertIdx < this->GetCount(); insertIdx++)
			{
				PropertyItem * listItem =
						static_cast<PropertyItem*>( this->GetItemDataPtr( insertIdx ) );
				if ((*it)->fullName() != listItem->fullName())
				{
					break;
				}
			}
		}
		else
		{
			index++;
		}

		if (insertIdx == this->GetCount())
		{
			insertIdx = -1;
		}

		int index = InsertString( insertIdx, L"" );
		SetItemDataPtr( index, *it );
		(*it)->create( this );
	}

	gItem->setExpanded( true );

	delayRedraw_ = false;
	this->GetParent()->PostMessage( WM_UPDATE_CONTROLS );
}


/**
 *	This method is called by array properties when they are about to elect
 *	their item's properties, so the property list knows the views added
 *	correspond to the array.
 *
 *	@param callback		callback with one int parameter: index of the array
 *						item whose "Delete Item" button has been pressed.
 */
void PropertyList::startArray( BWBaseFunctor1<int>* callback, bool readOnly )
{
	BW_GUARD;

	arrayIndex_.push( 0 );
	arrayPropertItemIndex_.push( -1 );
	arrayCallback_.push( callback );
	arrayReadOnly_.push( readOnly );
}


/**
 *	This method must be called to signal that no more property items will be
 *	added for the array, so other non-array property items can be added properly.
 */
void PropertyList::endArray()
{
	BW_GUARD;

	arrayIndex_.pop();
	arrayPropertItemIndex_.pop();
	arrayCallback_.pop();
	arrayReadOnly_.pop();
}


CRect PropertyList::dropTest( CPoint point, const BW::wstring& fileName )
{
	BW_GUARD;

	ScreenToClient( &point );
	BOOL out;
	int index = ItemFromPoint( point, out );

	CRect rect;
	GetItemRect( index, rect );

	//Determine the item
	PropertyItem* item = NULL;
	if ((!out) && (index != (uint16)(-1)) && (rect.PtInRect( point )))
		item = (PropertyItem*)GetItemDataPtr( index );

	//Make sure the item is a string
	if ( item  &&  (item->getType() == PropertyItem::Type_String) )
	{
		StringPropertyItem* stringItem = (StringPropertyItem*)item;
		BW::wstring ext = fileName.substr( fileName.rfind( L"." ) );

		//Ensure that the file type is accepted by the file filter
		if ((!stringItem->fileFilter().empty()) &&
			(bw_utf8tow( stringItem->fileFilter().c_str() ).find(ext) != BW::wstring::npos))
		{
			return rect;
		}
	}

	return CRect(0,0,0,0);
}

bool PropertyList::doDrop( CPoint point, const BW::wstring& fileName )
{
	BW_GUARD;

	ScreenToClient( &point );
	BOOL out;
	int index = ItemFromPoint( point, out );

	CRect rect;
	GetItemRect( index, rect );

	//Determine the item
	PropertyItem* item = NULL;
	if ((!out) && (index != (uint16)(-1)) && (rect.PtInRect( point )))
		item = (PropertyItem*)GetItemDataPtr( index );

	//Make sure the item is a string
	if ( item  &&  (item->getType() == PropertyItem::Type_String))
	{
		StringPropertyItem* stringItem = (StringPropertyItem*)item;
		BW::wstring ext = fileName.substr( fileName.rfind( L".") );

		//Ensure that the file type is accepted by the file filter
		if ((!stringItem->fileFilter().empty()) &&
			(bw_utf8tow( stringItem->fileFilter().c_str() ).find(ext) != BW::wstring::npos))
		{
			selectItem( index );

			stringItem->set( fileName );
			return true;
		}
	}

	return false;
}

/*static*/ WCHAR PropertyList::s_tooltipBuffer_[512];

INT_PTR PropertyList::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	BW_GUARD;

	BOOL out;
	UINT index = ItemFromPoint( point, out );
	
	CRect rect;
	GetItemRect( index, rect );

	//Determine the item
	PropertyItem* item = NULL;
	if ((!out) && (index != (uint16)(-1)) && (rect.PtInRect( point )))
	{
		item = (PropertyItem*)GetItemDataPtr( index );
	}
	
	if (!item)
	{
		if (mainFrame_)
			mainFrame_->SetMessageText( L"" );	
		return -1;
	}

	if (mainFrame_)
	{
		BW::wstring desc = L"";
		if ( !item->UIDescL().empty() )
		{
			desc = bw_utf8tow( item->UIDescL() );
			if ( !item->UIDescExtra().empty() )
			{
				desc = desc + L" (" + item->UIDescExtra() + L")";
			}
		}
		mainFrame_->SetMessageText( desc.c_str() );
	}
	
	//Get the client (area occupied by this control
	RECT rcClient;
	GetClientRect( &rcClient );

	//Fill in the TOOLINFO structure
	pTI->hwnd = GetSafeHwnd();
	pTI->uId = (UINT_PTR)(item);
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->rect = rcClient;

	return pTI->uId; //By returning a unique value per listItem,
					//we ensure that when the mouse moves over another list item,
					//the tooltip will change
}

BOOL PropertyList::OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	//Ensure that we are able to use newlines in the tooltip text
	CToolTipCtrl* pToolTip = AfxGetModuleState()->m_thread.GetDataNA()->m_pToolTip;
	pToolTip->SetMaxTipWidth( SHRT_MAX );
  
	//Handle both ANSI and UNICODE versions of the message
	// Note, we DON'T handle ANSI, since it's win95 ONLY
	TOOLTIPTEXTW* pTTTW = reinterpret_cast<TOOLTIPTEXTW*>( pNMHDR );

	//Ignore messages from the built in tooltip, we are processing them internally
	if( ( pNMHDR->idFrom == reinterpret_cast<UINT_PTR>( m_hWnd ) ) &&
		 ( (pNMHDR->code == TTN_NEEDTEXTW) && (pTTTW->uFlags & TTF_IDISHWND) ) )
	{
      return FALSE;
    }

    *pResult = 0;

	CPoint point;
	GetCursorPos(&point);
	ScreenToClient(&point); 

	BOOL out = TRUE;
	UINT index = ItemFromPoint( point, out );
	if ( ( index == -1 ) || ( out == TRUE ) )
	{
		WARNING_MSG( "Could not get tooltip: could not get item under mouse\n" );
		return FALSE;
	}
	
	CRect rect;
	int err = GetItemRect( index, rect );
	if ( err == LB_ERR )
	{
		WARNING_MSG( "Could not get tooltip: GetItemRect returned LB_ERR\n" );
		return FALSE;
	}

	//Determine the item
	PropertyItem* item = NULL;
	if ( rect.PtInRect( point ) )
	{
		// - Get item data
		void* ptr = GetItemDataPtr( index );
		if ( reinterpret_cast<intptr_t>( ptr ) == -1 )
		{
			WARNING_MSG( "Could not get tooltip: GetItemDataPtr returned -1\n" );
			return FALSE;
		}
		item = reinterpret_cast<PropertyItem*>( ptr );

		// - Get tooltip from item data
		toolTipMsg_ = L"";
		
		// Be careful, putting the wrong thing into CString::Format
		// will output garbage silently
		// See: http://msdn.microsoft.com/en-us/library/ms908328.aspx
		if ( !item->UIDescL().empty() )
		{
			if ( !item->UIDescExtra().empty() )
			{
				CString str1( bw_utf8tow( item->UIDesc() ).c_str() );
				CString str2( item->UIDescExtra().c_str() );
				toolTipMsg_.Format( _T( "%s\n%s" ), (LPCTSTR)str1, (LPCTSTR)str2 );
			}
			else
			{
				CString str( bw_utf8tow( item->UIDesc() ).c_str() );
				toolTipMsg_.Format( _T( "%s" ), (LPCTSTR)str );
			}
		}
		else if ( !item->UIDescExtra().empty() )
		{
			CString str( item->UIDescExtra().c_str() );
			toolTipMsg_.Format( _T( "%s" ), (LPCTSTR)str );
		}

		pTTTW->lpszText = toolTipMsg_.GetBuffer(0);

		*pResult = 0;

		return TRUE;    // message was handled
	}

	return FALSE; // message was not handled
}
BW_END_NAMESPACE

