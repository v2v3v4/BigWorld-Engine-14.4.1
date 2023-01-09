#pragma once

BW_BEGIN_NAMESPACE

class MyFolderFilter : public IFolderFilter
{
public:
	MyFolderFilter();
	
public: // IUnknown implementation

	STDMETHOD( QueryInterface )( IN REFIID /*riid*/, IN OUT LPVOID* /*ppvObj*/ );
	STDMETHOD_( ULONG, AddRef )( VOID );
	STDMETHOD_( ULONG, Release )( VOID );

public: // IFolderFilter implementation

	STDMETHOD( ShouldShow )( IN IShellFolder* /*pIShellFolder*/, IN LPCITEMIDLIST /*pidlFolder*/, IN LPCITEMIDLIST IN /*pidlItem*/ );
	STDMETHOD( GetEnumFlags )( IN IShellFolder* /*pIShellFolder*/, IN LPCITEMIDLIST /*pidlFolder*/, IN HWND* /*phWnd*/, OUT LPDWORD /*pdwFlags*/ );

protected:
	ULONG	m_ulRef;

public:
	LPCTSTR		m_pszFilter;
};

class DirDialog
{
public:
	DirDialog();
	virtual ~DirDialog();

	
	CString windowTitle_;				// window title text
	CString promptText_;				// inner window text

	CString fakeRootDirectory_;			// restrict choice to directories below
	CString startDirectory_;			// start off in directory


	BOOL doBrowse(CWnd *pwndParent = NULL);		// spawn the window


	CString userSelectedDirectory_;		// directory returned

	static CString basePath() { return s_basePath_; } // A getter for the default directory
	static BW::vector<CString> paths() { return s_paths_; } // A getter for the paths vector


private:
	MyFolderFilter* folderFilter_;

	static CString s_basePath_;
	static BW::vector<CString> s_paths_;
	static bool isPathOk( const wchar_t* path );
	static int __stdcall browseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

BW_END_NAMESPACE

