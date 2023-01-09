#ifndef FILE_DIALOG_HPP
#define FILE_DIALOG_HPP

#include "folder_guard.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class works exactly the same as CFileDialog except that it
 *	preserves the current working folder.
 */
class BWFileDialog
{
public:
	enum FDFlags
	{
		FD_HIDEREADONLY		= 1,
		FD_OVERWRITEPROMPT	= 1 << 1,
		FD_FILEMUSTEXIST	= 1 << 2,
		FD_PATHMUSTEXIST	= 1 << 3,
		FD_ALLOWMULTISELECT = 1 << 4,
		FD_WRITABLE_AND_EXISTS = FD_HIDEREADONLY | FD_FILEMUSTEXIST,
		FD_FILE_PATH_MUST_EXIST = FD_FILEMUSTEXIST | FD_PATHMUSTEXIST
	};

	BWFileDialog(
		 bool openFileDialog, const wchar_t * defaultExt,
		 const wchar_t * initialFileName,
		 FDFlags flags, const wchar_t * filter, void * parentWnd = 0 );
	~BWFileDialog();

	bool showDialog();
	void initialDir( const wchar_t * initialDir );
	BW::wstring getFileName() const;
	BW::vector< BW::wstring > getFileNames() const;

private:
	FolderGuard				folderGuard_;
};

BW_END_NAMESPACE

#endif // FILE_DIALOG_HPP
