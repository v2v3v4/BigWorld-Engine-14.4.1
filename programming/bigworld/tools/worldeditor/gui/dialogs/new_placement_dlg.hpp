#ifndef NEW_PLACEMENT_DLG_HPP
#define NEW_PLACEMENT_DLG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class NewPlacementDlg : public CDialog
{
	DECLARE_DYNAMIC(NewPlacementDlg)

public:
// Dialog Data
	enum { IDD = IDD_RANDNEWPRESET };

	NewPlacementDlg(CWnd* pParent = NULL);
	~NewPlacementDlg();

	void existingNames(BW::vector<BW::wstring> const &names);

	CString newName_;

protected:
	CEdit newNameCtrl_;
	BW::vector<BW::wstring> existingNames_;

	void DoDataExchange(CDataExchange* pDX);

	BOOL OnInitDialog();
	void OnOK();

	DECLARE_MESSAGE_MAP()

	bool isValidName(CString const &str) const;
};

BW_END_NAMESPACE

#endif // NEW_PLACEMENT_DLG_HPP
