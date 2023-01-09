#ifndef LABELLED_MULTI_TASK_PROGRESS_DLG_HPP
#define LABELLED_MULTI_TASK_PROGRESS_DLG_HPP

#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include <afxwin.h>
#include "afxcmn.h"

BW_BEGIN_NAMESPACE

/**
 * This modal dialog is used when you have a long-running process
 * on the main thread.
 *
 * It offers a method to pump the windows message queue and return
 * whether the user attempted to cancel the process.
**/
class LabelledMultiTaskProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(LabelledMultiTaskProgressDlg)

	DECLARE_AUTO_TOOLTIP( LabelledMultiTaskProgressDlg, CDialog )

public:
	LabelledMultiTaskProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~LabelledMultiTaskProgressDlg();
	virtual BOOL OnInitDialog();


	void setTaskCnt( int taskCnt );
	void setCurrentLength( float length );
	void setCurrentTask( int task );
	bool step( float progress = 1 );
	void setCurrentTaskInfo( const BW::string& sMsg, float length, bool escapable );
	bool isCancelled();
	bool set( float done = 0.f );
// Dialog Data
	enum { IDD = IDD_LABELLED_MULTI_TASK_PROGRESS };


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

private:
	void recalcProgress();
	void playFinishingAnimation();
	void fillProgressBar( CProgressCtrl& progressbar );

	CProgressCtrl entireProgress_;
	CProgressCtrl currentProgress_;
	CStatic labelledMessage_;
	CButton btnCancel_;
	int taskCnt_;
	int curTask_;
	float curProgress_;
	float curLength_;
	bool cancelled_;
	bool freezeEntireProgressMovement_;
public:
	afx_msg void OnBnClickedCancel();	
};

BW_END_NAMESPACE

#endif // LABELLED_MULTI_TASK_PROGRESS_DLG_HPP
