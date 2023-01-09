#ifndef CVS_INFO_DIALOG_HPP
#define CVS_INFO_DIALOG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/misc/cvswrapper.hpp"

BW_BEGIN_NAMESPACE

class CVSInfoDialog : public CDialog, public CVSLog
{
public:
	CVSInfoDialog( const BW::wstring& title );
	~CVSInfoDialog();

	virtual void add( const BW::wstring& msg );

// Dialog Data
	enum { IDD = IDD_MODELESS_INFO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	BW::wstring title_;

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};

BW_END_NAMESPACE

#endif // CVS_INFO_DIALOG_HPP
