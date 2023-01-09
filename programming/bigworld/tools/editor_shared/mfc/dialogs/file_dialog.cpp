#include "../../dialogs/file_dialog.hpp"

#include <afxdlgs.h>

#include "cstdmf/guard.hpp"

namespace
{
	static const size_t BUFFER_SIZE_CHARS = 32 * 1024;

	TCHAR * s_pBuffer = NULL;
	TCHAR * s_pOldBuffer = NULL;
	CFileDialog * s_fileDialogImpl = NULL;
}

BW_BEGIN_NAMESPACE


//==============================================================================
BWFileDialog::BWFileDialog(
	bool openFileDialog, const wchar_t * defaultExt,
	const wchar_t * initialFileName,
	FDFlags flags, const wchar_t * filter, void * parentWnd )
{
	BW_GUARD;
	s_fileDialogImpl = 
			new CFileDialog( openFileDialog, defaultExt, initialFileName,
				( flags & FD_HIDEREADONLY	? OFN_HIDEREADONLY : 0 ) |
				( flags & FD_OVERWRITEPROMPT ? OFN_OVERWRITEPROMPT : 0 ) |
				( flags & FD_FILEMUSTEXIST ? OFN_FILEMUSTEXIST : 0 ) |
				( flags & FD_ALLOWMULTISELECT ? OFN_ALLOWMULTISELECT : 0 ) |
				( flags & FD_PATHMUSTEXIST ? OFN_PATHMUSTEXIST : 0 ),
				filter, ( CWnd * ) parentWnd, 0  );
	OPENFILENAME & ofn = s_fileDialogImpl->GetOFN();
	s_pOldBuffer = ofn.lpstrFile;
	if( flags & FD_ALLOWMULTISELECT ) 
	{
		ofn.nMaxFile = BUFFER_SIZE_CHARS;
		s_pBuffer = (TCHAR *) malloc( BUFFER_SIZE_CHARS * sizeof(TCHAR));
		memset(s_pBuffer, 0, BUFFER_SIZE_CHARS * sizeof(TCHAR));
		ofn.lpstrFile = s_pBuffer;
	}
}


//==============================================================================
BWFileDialog::~BWFileDialog()
{
	BW_GUARD;
	OPENFILENAME & ofn = s_fileDialogImpl->GetOFN();
	ofn.lpstrFile = s_pOldBuffer;
	free( s_pBuffer );
	s_pBuffer = NULL;
	delete s_fileDialogImpl;
	s_fileDialogImpl = NULL;
}

//==============================================================================
bool BWFileDialog::showDialog()
{
	BW_GUARD;
	return s_fileDialogImpl->DoModal() == IDOK;
}

//==============================================================================
void BWFileDialog::initialDir( const wchar_t * initialDir )
{
	BW_GUARD;
	s_fileDialogImpl->m_ofn.lpstrInitialDir = initialDir;
}


//==============================================================================
BW::wstring BWFileDialog::getFileName() const
{
	BW_GUARD;
	return BW::wstring (
		s_fileDialogImpl->GetPathName().GetString() );
}

//==============================================================================
BW::vector< BW::wstring > BWFileDialog::getFileNames() const
{
	BW_GUARD;
	BW::vector< BW::wstring > fileNames;
	POSITION pos ( s_fileDialogImpl->GetStartPosition() );
	while( pos )
	{
		fileNames.push_back(
			BW::wstring( s_fileDialogImpl->GetNextPathName( pos ).GetString() ) );
	}
	return fileNames;
}

BW_END_NAMESPACE