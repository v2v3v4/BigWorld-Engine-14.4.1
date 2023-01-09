#ifndef _RESOURCE_FILE_DIALOG__
#define _RESOURCE_FILE_DIALOG__

#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

class ResourceFileDialog : public CFileDialog
{
public:
	ResourceFileDialog( BOOL bOpenFileDialog, LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL )
		: CFileDialog( bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags | OFN_EXPLORER | OFN_NOCHANGEDIR,
		lpszFilter,pParentWnd )
	{}
	virtual BOOL OnFileNameOK()
	{
		BW_GUARD;

		BW::string filename = BWResource::dissolveFilename( (LPCTSTR)GetPathName() );
		if( filename[ 1 ] == ':' ) // absolute folder
		{
			BW::string msg = L("CONTROLS/RESOURCE_FILE_DIALOG/MUST_BE_IN_RESOURCE_DIR");
			for( int i = 0; i < BWResource::getPathNum(); ++i )
			{
				msg += '\t';
				msg += BWResource::getPath( i );
				if( i != BWResource::getPathNum() - 1 )
					msg += L("CONTROLS/RESOURCE_FILE_DIALOG/OR");
			}
			std::replace( msg.begin(), msg.end(), '/', '\\' );
			MessageBox( msg.c_str() );
			return 1;
		}
		return 0;
	}
};

BW_END_NAMESPACE

#endif//_RESOURCE_FILE_DIALOG__
