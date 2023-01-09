#include "pch.hpp"
#include "worldeditor/gui/controls/directory_browser.hpp"

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNAMIC(DirectoryBrowser, CListBox)

DirectoryBrowser::DirectoryBrowser()
: currentPath_("")
, startDirectory_("")
{
}

DirectoryBrowser::~DirectoryBrowser()
{
}

namespace
{

class SmartTruncator
{
	CFont* saveFont_;
	CDC* dc_;
	CWnd* wnd_;
	int maxWidth_;
	int currentMax_;
	static int const RIGHT_MARGIN = 10;// give the user some space to live
public:
	SmartTruncator( CDC* dc, CWnd* wnd ) : dc_( dc ), wnd_( wnd )
	{
		BW_GUARD;

		RECT rect;
		wnd_->GetClientRect( &rect );
		maxWidth_ = rect.right;
		currentMax_ = 0;
		saveFont_ = dc->SelectObject( wnd_->GetFont() );
	}
	int getWidth()
	{
		BW_GUARD;

		return ( currentMax_ + RIGHT_MARGIN < maxWidth_ )	?	currentMax_ + RIGHT_MARGIN	:
																maxWidth_;
	}
	CString feedFileName( CString fileName )
	{
		BW_GUARD;

		CString s = fileName;
		CSize size = dc_->GetOutputTextExtent( fileName );
		if( size.cx > currentMax_ )
			currentMax_ = size.cx;
		if( size.cx > maxWidth_ - RIGHT_MARGIN )
		{
			while( s.GetLength() )
			{
				s.Delete( s.GetLength() - 1 );
				fileName = s + L"....";// one more dot for border :)
				if( dc_->GetOutputTextExtent( fileName ).cx < maxWidth_ - RIGHT_MARGIN )
				{
					fileName = s + L"...";
					break;
				}
			}
		}
		return fileName;
	}
	~SmartTruncator()
	{
		BW_GUARD;

		dc_->SelectObject( saveFont_ );
	}
};

}

void DirectoryBrowser::fill()
{
	BW_GUARD;

	MF_ASSERT(GetParent());
	GetParent()->SendMessage(WM_CHANGE_DIRECTORY, 0, (LPARAM)currentPath_.GetBuffer());

	SetRedraw(FALSE);
	ResetContent();

	SmartTruncator SmartTruncator( GetDC(), this );
	mFileNameIndex.RemoveAll();
	mFileNameIndex.Add( L"" );// guard from 1

	if (currentPath_.IsEmpty())
	{
		if (basePaths_.size() == 1)
		{
			currentPath_ = *basePaths_.begin();
		}
		else
		{
			// at the base level (no hierarchy)
			for (BW::vector<CString>::iterator it = basePaths_.begin();
				it != basePaths_.end();
				it++)
			{
				CString newFileName = SmartTruncator.feedFileName( *it );
				if( newFileName != *it )
				{
					INT_PTR index = mFileNameIndex.GetSize();
					mFileNameIndex.Add( *it );
					SetItemData( AddString( newFileName ), index );
				}
				else
				{
					SetItemData( AddString( *it ), 0 );
				}
			}
			SetColumnWidth( SmartTruncator.getWidth() );

			SetRedraw();
			InvalidateRect(NULL);
			return;
		}
	}

	CString searchString = currentPath_ + L"*.*";
	CFileFind finder;
	if ( !finder.FindFile(searchString) )
	{
		currentPath_ = "";
		fill();
		return;
	}

	BOOL moreFiles = TRUE;
	while ( moreFiles )
	{
		moreFiles = finder.FindNextFile();

		if ( finder.IsDirectory() )
		{
			CString filename = finder.GetFileName();
			if ( filename == L"." || filename == L"CVS" ||
				filename == ".svn" || filename == L".bwthumbs" ) 
				continue;

			CString newFileName = SmartTruncator.feedFileName( filename );
			if( newFileName != filename )
			{
				DWORD_PTR index = mFileNameIndex.GetSize();
				mFileNameIndex.Add( filename );
				SetItemData( AddString( newFileName ), index );
			}
			else
			{
				SetItemData( AddString(filename), 0 );
			}
		}
	}

	SetColumnWidth( SmartTruncator.getWidth() );

	SetRedraw();
	InvalidateRect(NULL);
}

void DirectoryBrowser::initialise(const BW::vector<CString>& basePaths)
{
	BW_GUARD;

	MF_ASSERT(basePaths_.empty());

	for (BW::vector<CString>::const_iterator it = basePaths.begin();
		it != basePaths.end();
		it++)
	{
		CString path = *it;

		// convert to \ and ensure there is a \ at the end
		path.Replace('/', '\\');
		if(path.Right(1)!="\\")
			path += "\\";

		basePaths_.push_back(path);
	}


	for (BW::vector<CString>::const_iterator it = basePaths_.begin();
		it != basePaths_.end();
		it++)
	{
		if (startDirectory_.Find(*it) != -1)
		{
			currentPath_ = startDirectory_;
			break;
		}
	}

	fill();
}

void DirectoryBrowser::setStartDirectory(const CString& directory)
{
	BW_GUARD;

	startDirectory_ = directory;
	startDirectory_.Replace('/', '\\');
}


BEGIN_MESSAGE_MAP(DirectoryBrowser, CListBox)
	//{{AFX_MSG_MAP(DirectoryBrowser)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSectionChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// DirectoryBrowser message handlers


void DirectoryBrowser::OnSectionChange() 
{
	BW_GUARD;

	if (GetCurSel() == -1)
		return;

	CString selection;
	GetText(GetCurSel(), selection);

	// Xiaoming Shi : long file names friendly, for bug 4695{
	if( DWORD_PTR itemData = GetItemData( GetCurSel() ) )
	{
		selection = mFileNameIndex[ itemData ];
	}
	// Xiaoming Shi : long file names friendly, for bug 4695}
	if(selection == "..")
	{
		if (std::find(basePaths_.begin(), basePaths_.end(), currentPath_) == basePaths_.end())
		{
			currentPath_ = currentPath_.Left(currentPath_.ReverseFind('\\'));
			currentPath_ = currentPath_.Left(currentPath_.ReverseFind('\\'));
			currentPath_ += '\\';
		}
		else
		{
			currentPath_ = "";
		}
	}
	else
	{
		//otherwise add another sub-dir to the path
		currentPath_ += selection;

		if(currentPath_.Right(1)!="\\")
			currentPath_ += "\\";
	}

	fill();
}

BW_END_NAMESPACE


