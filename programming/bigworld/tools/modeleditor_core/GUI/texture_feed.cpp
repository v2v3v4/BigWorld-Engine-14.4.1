#include "pch.hpp"
#include "texture_feed.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(CTextureFeed, CDialog)
	ON_EN_CHANGE(IDC_TEXTURE_FEED_NAME, OnEnChangeTextureFeedName)
	ON_BN_CLICKED(IDC_REMOVE_TEXTURE_FEED, OnBnClickedRemoveTextureFeed)
END_MESSAGE_MAP()

CTextureFeed::CTextureFeed( const BW::wstring& feedName ):
	feedName_(feedName),
	CDialog(CTextureFeed::IDD)
{}

void CTextureFeed::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_TEXTURE_FEED_NAME, name_);
	DDX_Control(pDX, IDOK, ok_);

	CDialog::DoDataExchange(pDX);
}

BOOL CTextureFeed::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	name_.SetWindowText( feedName_.c_str() );
	name_.SetFocus();
	name_.SetSel(0,-1); //TODO: Move this so that it works ;-)

	checkComplete();

	return TRUE;
}


void CTextureFeed::checkComplete()
{
	BW_GUARD;

	if (feedName_ != L"")
	{
		ok_.ModifyStyle( WS_DISABLED, 0 );
	}
	else
	{
		ok_.ModifyStyle( 0, WS_DISABLED );
	}

	ok_.RedrawWindow();
}

void CTextureFeed::OnEnChangeTextureFeedName()
{
	BW_GUARD;

	CString feedName_cstr;
	name_.GetWindowText( feedName_cstr );
	feedName_ = BW::wstring( feedName_cstr );
	checkComplete();
}

void CTextureFeed::OnBnClickedRemoveTextureFeed()
{
	BW_GUARD;

	feedName_ = L"";
	OnOK();
}
BW_END_NAMESPACE

