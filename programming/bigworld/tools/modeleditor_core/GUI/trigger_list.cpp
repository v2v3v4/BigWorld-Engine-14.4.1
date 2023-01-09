#include "pch.hpp"

#include "windowsx.h"

#include "common/string_utils.hpp"

#include "trigger_list.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(CheckList, CTreeCtrl)
END_MESSAGE_MAP()

void CheckList::capsStr( const BW::string& capsStr )
{
	BW_GUARD;

	BW::vector< BW::string > temp;
	bw_tokenise( capsStr, ";, ", temp );

	int val = 0;
	for (unsigned i=0; i<temp.size(); i++)
	{
		sscanf( temp[i].c_str(), "%d", &val );
		capsSet_.insert( val );
	}
}

BW::string CheckList::caps()
{
	BW_GUARD;

	char buf[256] = "";

	BW::set<int>::iterator it = capsSet_.begin();
	BW::set<int>::iterator end = capsSet_.end();
	
	if (capsSet_.size() > 0)
		bw_snprintf(buf, sizeof(buf), "%d", *it);
	for (++it; it != end; ++it)
		bw_snprintf(buf, sizeof(buf), "%s %d", buf, *it);

	return BW::string( buf );
}

void CheckList::redrawList()
{
	BW_GUARD;

	HTREEITEM item = GetFirstVisibleItem();
	do
	{
		int id = *(int*)(GetItemData( item ));

		SetCheck( item, FALSE);

		BW::set<int>::iterator it = capsSet_.begin();
		BW::set<int>::iterator end = capsSet_.end();

		for (; it != end; ++it)
		{
			if (*it == id)
			{
				SetCheck( item, TRUE);
				break;
			}
		}
	}
	while ( item = GetNextVisibleItem( item ) );
}

void CheckList::updateList()
{
	BW_GUARD;

	capsSet_.clear();
	
	HTREEITEM item = GetFirstVisibleItem();
	do
	{
		if (GetCheck( item ) == BST_CHECKED)
		{
			capsSet_.insert( *(int*)(GetItemData( item )));
		}
	}
	while ( item = GetNextVisibleItem( item ) );
}

CTriggerList::CTriggerList( const BW::string& capsName, BW::vector< DataSectionPtr >& capsList, const BW::string& capsStr /* = "" */ ):
	CDialog( CTriggerList::IDD ),
	capsName_(capsName),
	capsList_(capsList),
	capsStr_(capsStr)
{
	BW_GUARD;

	checkList_.capsStr( capsStr_ );
}

CTriggerList::~CTriggerList()
{
	BW_GUARD;

	BW::vector<int*>::iterator capsDataIt = capsData_.begin();
	BW::vector<int*>::iterator capsDataEnd = capsData_.end();
	for (;capsDataIt != capsDataEnd; ++capsDataIt)
	{
		delete (*capsDataIt);
	}
	capsData_.clear();
}

void CTriggerList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TRIGGER_LIST, checkList_);
}

BOOL CTriggerList::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	SetWindowText( bw_utf8tow( capsName_ ).c_str() );

	checkList_.ModifyStyle( TVS_CHECKBOXES, 0 );
	checkList_.ModifyStyle( 0, TVS_CHECKBOXES );

	BW::vector< DataSectionPtr >::iterator it = capsList_.begin();
	BW::vector< DataSectionPtr >::iterator end = capsList_.end();

	HTREEITEM item;

	// Set containing the list values that are handled by the named capabilities
	BW::set<int> usedValues;

	// Iterate over our named capabilities and add them to the list
	for(;it != end; ++it)
	{
		int id = (*it)->asInt();
		BW::wstring name = (*it)->readWideString( "name", L"" );
		item = checkList_.InsertItem( name.c_str(), NULL );

		int* capNum = new int( id );
		checkList_.SetItemData( item, (DWORD_PTR)capNum );
		capsData_.push_back( capNum );

		usedValues.insert( id );
	}

	// Iterate over all possible values for a capability
	// Add items to the list for them so that they can still
	// be set and unset
	for (int id = 0; id < 32; id++)
	{
		if (usedValues.find(id) == usedValues.end())
		{
			char buf[32];
			bw_snprintf( buf, 32, "Undefined %d", id );
			BW::wstring name;
			bw_utf8tow( buf, name );

			item = checkList_.InsertItem( name.c_str(), NULL );

			int* capNum = new int( id );
			checkList_.SetItemData( item, (DWORD_PTR)capNum );
			capsData_.push_back( capNum );
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BW::string CTriggerList::caps()
{
	BW_GUARD;

	return checkList_.caps();
}

BEGIN_MESSAGE_MAP(CTriggerList, CDialog)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CTriggerList::OnPaint()
{
	BW_GUARD;

	CDialog::OnPaint();

	checkList_.redrawList();
}

void CTriggerList::OnOK()
{
	BW_GUARD;

	checkList_.updateList();

	CDialog::OnOK();
}
BW_END_NAMESPACE

