#ifndef OPTIONS_DIALOG_HPP
#define OPTIONS_DIALOG_HPP

#include "wtl.hpp"

#include "jit_compiler_options.hpp"

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class OptionsDialog : 
	public ATL::CDialogImpl<OptionsDialog>
{
public:
	enum { IDD = IDR_OPTIONS_DIALOG };

public:
	explicit OptionsDialog(JitCompilerOptions & options);
	~OptionsDialog();

	bool run();

	BEGIN_MSG_MAP_EX(OptionsDialog)
		MSG_WM_INITDIALOG(onInitDialog)
		MSG_WM_CLOSE(onClose)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, onOk)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, onCancel)
		MESSAGE_HANDLER_EX(WM_HSCROLL, onTrackbar)
	END_MSG_MAP()

private:

	// Message Map Functions
	int onInitDialog(ATL::CWindow handle, LPARAM data);
	void onClose();
	void onOk(UINT notifyCode, int controlID, CWindow handle);
	void onCancel(UINT notifyCode, int controlID, CWindow handle);
	LRESULT onTrackbar(UINT message, WPARAM wParam, LPARAM lParam);

	void updateThreadsLabel(const int threadCount);

	void setPathSetting(WTL::CEdit & control, const BW::string & path);
	BW::string getPathSetting(const WTL::CEdit & control) const;

private:

	JitCompilerOptions & options_;

	WTL::CButton balloonCheckbox_;
	WTL::CTrackBarCtrl threadCountSlider_;
	WTL::CStatic threadCountLabel_;
	WTL::CEdit outputPathTextbox_;
	WTL::CEdit interPathTextbox_;
	WTL::CEdit cachePathTextbox_;
	WTL::CButton cacheReadCheckbox_;
	WTL::CButton cacheWriteCheckbox_;
	WTL::CButton forceRebuildCheckbox_;

};

BW_END_NAMESPACE

#endif // OPTIONS_DIALOG_HPP
