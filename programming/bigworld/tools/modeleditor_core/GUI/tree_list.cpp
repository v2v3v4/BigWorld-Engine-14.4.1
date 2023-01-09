#include "pch.hpp"

#include "me_app.hpp"

#include "tools/common/delay_redraw.hpp"

#include "tree_list.hpp"
#include "../resource.h"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE
// TreeList

/*static*/ const wchar_t TreeList::LOCKED_TAG[] = L" <locked>";

IMPLEMENT_DYNCREATE(TreeList, CFormView) 

TreeList::TreeList( UINT dialogID, TreeRoot* treeRoot, const BW::string& what ):
	selID_ ("",""),
	selItem_(NULL),
	search_str_ (""),
	ignoreSelChange_ (false),
	sameClicked_( false ),
	CFormView( dialogID ),
	treeRoot_ ( treeRoot ),
	what_(what),
	locked_( false )
{
}

TreeList::~TreeList()
{
	BW_GUARD;

	BW::vector<BW::string*>::iterator pathDataIt = pathData_.begin();
	BW::vector<BW::string*>::iterator pathDataEnd = pathData_.end();
	while (pathDataIt != pathDataEnd)
	{
		delete (*pathDataIt);
		pathDataIt++;
	}
	pathData_.clear();
}

void TreeList::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_SEARCH_BKG, search_bkg_);
	DDX_Control(pDX, IDC_SEARCH_TEXT, search_);
	DDX_Control(pDX, IDC_SEARCH_BUTTON, search_button_);
	DDX_Control(pDX, IDC_SEARCH_CANCEL, search_cancel_);
	DDX_Control(pDX, IDC_SEARCH_TREE, tree_);
}

BEGIN_MESSAGE_MAP(TreeList, CFormView)

	ON_WM_SIZE()
	
	ON_STN_CLICKED(IDC_SEARCH_BUTTON, OnEnSetFocusSearch)
	ON_EN_CHANGE(IDC_SEARCH_TEXT, OnEnChangeSearch)
	ON_STN_CLICKED(IDC_SEARCH_CANCEL, OnStnClickedCancelSearch)
	
	ON_NOTIFY(TVN_SELCHANGED, IDC_SEARCH_TREE, OnTvnSelChangedTree)
	ON_NOTIFY(NM_CLICK, IDC_SEARCH_TREE, OnNMClickSearchTree)

END_MESSAGE_MAP()

void TreeList::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	int yPos = search_bkg_.GetParent()->GetScrollPos( SB_VERT );
	
	search_bkg_.SetWindowPos( &CWnd::wndBottom, 12, 12 - yPos, cx - 24, 23, 0 );
    CRect rect;
    search_bkg_.GetWindowRect( &rect );
    ScreenToClient( &rect );

    search_.SetWindowPos( 0, rect.left + 22, rect.top + 5, rect.Width() - 40, rect.Height() - 7, SWP_NOZORDER );
    search_button_.SetWindowPos( 0, rect.left + 4, rect.top + 4, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
    search_cancel_.SetWindowPos( 0, rect.left + rect.Width() - 19, rect.top + 4, 0, 0, SWP_NOSIZE | SWP_NOZORDER );

	CRect rect2;
    tree_.GetWindowRect( &rect2 );
    tree_.SetWindowPos( 0, rect.left , rect.top + 32, rect.Width(), rect2.Height(), SWP_NOZORDER );

	CFormView::OnSize( nType, cx, cy );
}

void TreeList::OnUpdateTreeList()
{
	BW_GUARD;

	if (MeApp::instance().mutant())
	{
		//Save the focus
		CWnd* currFocus = GetFocus();

		//Save the current selection
		StringPair currID = selID_;

		bool isParent = (tree_.GetParentItem( selItem_ ) == NULL);
		
		//Firstly we need to delete any model path data stored
		BW::vector<BW::string*>::iterator pathDataIt = pathData_.begin();
		BW::vector<BW::string*>::iterator pathDataEnd = pathData_.end();
		while (pathDataIt != pathDataEnd)
		{
			delete (*pathDataIt);
			pathDataIt++;
		}
		pathData_.clear();

		//We need to scope the "DelayRedraw" since it interfers with "EnsureVisible"
		{
			DelayRedraw temp( &tree_ );
		
			//Now delete the old tree
			ignoreSelChange_ = true;
			tree_.DeleteAllItems();
			ignoreSelChange_ = false;

			selItem_ = NULL;
			
			for (unsigned m=0; m<treeRoot_->size(); m++)
			{
				BW::string modelName = (*treeRoot_)[m].first.first;

				if ((locked_) && (m != 0))
					modelName += bw_wtoutf8( LOCKED_TAG );
				
				HTREEITEM model = tree_.InsertItem( bw_utf8tow( modelName ).c_str() );
				
				//Not pretty but needs to be done since it is possible to have multiple
				//model names the same and thus we can't just use a map.
				BW::string* fileName = new BW::string( (*treeRoot_)[m].first.second );
				tree_.SetItemData( model, ( DWORD_PTR )fileName );
				pathData_.push_back( fileName );

				if ((isParent) && (StringPair( modelName, *fileName ) == currID))
				{
					selItem_ = model;
					selID_ = currID;
				}

				tree_.SetItemState( model, TVIS_EXPANDED | TVIS_BOLD, TVIS_EXPANDED | TVIS_BOLD );

				if ((what_== "MODELEDITOR/ANIMATIONS" ) && (!MeApp::instance().mutant()->canAnim( *fileName )))
				{
					tree_.InsertItem( Localise(L"MODELEDITOR/GUI/TREE_LIST_DLG/NON_ANIMATED"), model );
				}
				else if ((*treeRoot_)[m].second.size() == 0)
				{
					tree_.InsertItem( Localise(L"MODELEDITOR/GUI/TREE_LIST_DLG/NO_ANIMATIONS", Localise( bw_utf8tow( what_ ).c_str() ) ), model );
				}
				else
				{
					for (unsigned a=0; a<(*treeRoot_)[m].second.size(); a++)
					{
						BW::string name = (*treeRoot_)[m].second[a];
						BW::string nameLower = name;

						//convert it to lowercase
						std::transform (nameLower.begin(),
							nameLower.end(), nameLower.begin(), tolower);

						if (nameLower.find( search_str_, 0 ) != BW::string::npos)
						{
							HTREEITEM anim = tree_.InsertItem( bw_utf8tow( name ).c_str(), model );
							if ((!isParent) && (StringPair( name, *fileName ) == currID))
							{
								selItem_ = anim;
								selID_ = currID;
							}
						}
					}
				}
			}
		}

		if (selItem_)
		{
			tree_.SelectItem( selItem_ );
			tree_.EnsureVisible( selItem_ );
		}
		else
		{
			tree_.SelectItem( tree_.GetFirstVisibleItem() );
			tree_.EnsureVisible( tree_.GetFirstVisibleItem() );
		}

		//Restore the focus
		if (currFocus)
			currFocus->SetFocus();
	}
}

void TreeList::OnTvnSelChangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	if (ignoreSelChange_) return;
	
	//Ensure that we defocus the current field before making a new selection.
	//We do this to ensure it is updated...
	tree_.SetFocus();

	//Save the item index
	selItem_ = tree_.GetSelectedItem();

	CString itemText = tree_.GetItemText( selItem_ );

	BW::string* modelPath = (BW::string*)(tree_.GetItemData(selItem_));
	
	//Save the selection ID
	HTREEITEM parent = tree_.GetParentItem( selItem_ );

	if (parent == NULL)
	{
		int lPos =  itemText.Find( LOCKED_TAG );
		locked_ = (lPos != -1);
		if (locked_) 
		{
			size_t len = wcslen( LOCKED_TAG );
			MF_ASSERT( len <= INT_MAX );
			itemText.Delete( lPos, ( int ) len );
		}
	}
	else
	{
		CString text = tree_.GetItemText( parent );
		int lPos =  text.Find( LOCKED_TAG );
		locked_ = (lPos != -1);
	}

	BW::string model = *(BW::string*)(tree_.GetItemData( parent ? parent : selItem_ ));
	selID_ = StringPair( bw_wtoutf8(itemText.GetString()), model );

	//Inform children of change
	selChange( selID_ );

	sameClicked_ = false;

	*pResult = 0;
}


void TreeList::selectItem( const StringPair& sp )
{
	HTREEITEM model = tree_.GetRootItem();

	while (model)
	{
		if (*(BW::string*)(tree_.GetItemData( model )) == sp.second)
		{
			HTREEITEM action = tree_.GetChildItem( model );

			while (action)
			{
				CString itemText = tree_.GetItemText( action );

				if (bw_wtoutf8(itemText.GetString()) == sp.first)
				{
					tree_.SelectItem( action );

					return;
				}

				action = tree_.GetNextItem( action, TVGN_NEXT );
			}
		}

		model = tree_.GetNextItem( model, TVGN_NEXT );
	}

	tree_.SelectItem( NULL );
}


void TreeList::OnNMClickSearchTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	//Save the item index
	POINT point;
	::GetCursorPos( &point );
	tree_.ScreenToClient(&point);
	HTREEITEM item = tree_.HitTest( point, 0 );

	//Ensure that we defocus the current field before making a new selection.
	//We do this to ensure it is updated...
	tree_.SetFocus();

	if ( item == NULL )
		return;

	CString itemText = tree_.GetItemText( item );

	BW::string* modelPath = (BW::string*)(tree_.GetItemData(item));
	
	//Save the selection ID
	HTREEITEM parent = tree_.GetParentItem( item );

	if (parent == NULL)
	{
		int lPos =  itemText.Find( LOCKED_TAG );
		locked_ = (lPos != -1);
		if (locked_)
		{
			size_t len = wcslen( LOCKED_TAG );
			MF_ASSERT( len <= INT_MAX );
			itemText.Delete( lPos, ( int ) len );
		}
	}
	else
	{
		CString text = tree_.GetItemText( parent );
		int lPos =  text.Find( LOCKED_TAG );
		locked_ = (lPos != -1);
	}

	BW::string model = *(BW::string*)(tree_.GetItemData( parent ? parent : item ));
	StringPair selection( bw_wtoutf8(itemText.GetString()), model );

	if ( selID_ == selection )
		sameClicked_ = true;
		
	selClick( selID_ );

	*pResult = 0;
}

void TreeList::OnEnChangeSearch()
{
	BW_GUARD;

	//First lets get the value entered
	CString search_cstr;
	search_.GetWindowText( search_cstr );
	search_str_ = bw_wtoutf8( search_cstr.GetString() );

	//convert it to lowercase
	std::transform (search_str_.begin(), search_str_.end(), search_str_.begin(), tolower);

	search_cancel_.ShowWindow( search_str_ != "" );

	OnUpdateTreeList();
}

void TreeList::OnEnSetFocusSearch()
{
	BW_GUARD;

	search_.SetFocus();
}

void TreeList::OnStnClickedCancelSearch()
{
	BW_GUARD;

	search_.SetWindowText(L"");
}
BW_END_NAMESPACE

