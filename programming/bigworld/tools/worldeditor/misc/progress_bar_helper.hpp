#ifndef PROGRESS_BAR_HELPER_HPP
#define PROGRESS_BAR_HELPER_HPP


#include "chunk/chunk_item.hpp"
#include "common/popup_menu.hpp"
#include "romp/progress.hpp"
#include "worldeditor/gui/dialogs/labelled_progress_dlg.hpp"
#include "worldeditor/gui/dialogs/labelled_multitask_progress_dlg.hpp"

BW_BEGIN_NAMESPACE

class ProgressBar
{
public:
	virtual ~ProgressBar() {}

	virtual void startProgress( const BW::string& sMsg, float steps, bool escapable ) = 0;
	virtual void length( float length ) = 0;
	virtual void progressFinished() = 0;
	virtual bool step( float progress = 1 ) = 0;
	virtual bool set( float done = 0.f ) = 0;
	virtual bool isCancelled() = 0;

public:
	static ProgressBar* getCurrentProgressBar() { return s_currentProgressBar_; }
	static void setCurrentProgressBar( ProgressBar* current) { s_currentProgressBar_ = current; }

private:
	static ProgressBar* s_currentProgressBar_;
};

class SingleTaskProgressBar : public ProgressBar
{
public:
	SingleTaskProgressBar();
	virtual ~SingleTaskProgressBar();

	void startProgress( const BW::string& sMsg, float steps, bool escapable );
	void length( float length );
	void progressFinished();
	bool step( float progress = 1 );
	bool set( float done = 0.f );
	bool isCancelled();

private:
	void removeCurrentTask();
	void createTask( const BW::string& sMsg, float steps, bool escapable );

private:
	LabelledProgressDlg* progressDlg_;
};

class ProgressBarTask : public Progress
{
public:
	ProgressBarTask( const BW::string& sMsg, float steps, bool escapable );
	virtual ~ProgressBarTask();

	bool step( float progress = 1 );
	void setStepCounts( float steps );
	bool isCancelled();
	bool set( float done = 0.f );

	void length( float length );

private:
	bool stepped_;
	SingleTaskProgressBar* progressbar_;
};

class MultiTaskProgressBar : public ProgressBar
{
public:
	MultiTaskProgressBar();
	virtual ~MultiTaskProgressBar();

	bool init( int taskCnt );
	void moveToNextTask();
	void finishAll();
	bool isInited();

	void startProgress( const BW::string& sMsg, float steps, bool escapable );
	void length( float length );
	void progressFinished();
	bool step( float progress = 1 );
	bool set( float done = 0.f );
	bool isCancelled();


private:
	int taskCount_;
	int currentTask_;
	bool inited_;

	LabelledMultiTaskProgressDlg* progressDlg_;
};

class ScopedProgressBar
{
public:
	ScopedProgressBar( int taskCnt = 1 );
	~ScopedProgressBar();

private:
	ProgressBar* progressbar_;
	int taskCnt_;
};

BW_END_NAMESPACE

#endif //PROGRESS_BAR_HELPER_HPP