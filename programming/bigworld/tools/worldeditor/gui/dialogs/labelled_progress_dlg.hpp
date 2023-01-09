#ifndef LABELLED_PROGRESS_DLG_HPP
#define LABELLED_PROGRESS_DLG_HPP

#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

/**
 * This modal dialog is used when you have a long-running process
 * on the main thread.
 *
 * It offers a method to pump the windows message queue and return
 * whether the user attempted to cancel the process.
**/
class LabelledProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(LabelledProgressDlg)

	DECLARE_AUTO_TOOLTIP( LabelledProgressDlg, CDialog )

public:
	LabelledProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~LabelledProgressDlg();


	// This will start a new stage, spreading stepCount steps across fraction
	// of the total progress bar.
	void setStage( const BW::string& stage, const float fraction, const int stepCount, bool cancellable = false );
	bool step( int progress = 1 );
	void length( int len );
	void SetPos( int pos );


	// This is latched once it's true.
	// Also, once true, the stage and progress display
	// are used to indicate that the task is cancelling
	// rather than the stage the task had specified.
	bool cancelled()			{ return cancelled_; } const

// Dialog Data
	enum { IDD = IDD_LABELLEDPROGRESS };

private:
	void fullProgressBar( CProgressCtrl& progressbar );
	void checkCancel();
	bool cancelled_;
	float fractionComplete_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStatic stage_;
	CProgressCtrl progress_;
	CButton buttonCancel_;

	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnOK();
};

BW_END_NAMESPACE

#endif // LABELLED_PROGRESS_DLG_HPP
