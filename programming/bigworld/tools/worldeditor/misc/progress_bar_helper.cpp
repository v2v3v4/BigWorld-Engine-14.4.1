#include "pch.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"

BW_BEGIN_NAMESPACE

ProgressBar* ProgressBar::s_currentProgressBar_ = NULL;

/************************************************************************/
/*     SingleTaskProgressBarManager                                     */
/************************************************************************/

SingleTaskProgressBar::SingleTaskProgressBar():
	progressDlg_(NULL)
{
	BW_GUARD;
}

SingleTaskProgressBar::~SingleTaskProgressBar()
{
	BW_GUARD;
	this->progressFinished();
}

void SingleTaskProgressBar::startProgress( const BW::string& sMsg, float steps, bool escapable )
{
	BW_GUARD;
	this->removeCurrentTask();
	this->createTask( sMsg, steps, escapable );
}

void SingleTaskProgressBar::progressFinished()
{
	BW_GUARD;
	this->removeCurrentTask();
}

bool SingleTaskProgressBar::step( float progress /*= 1 */ )
{
	BW_GUARD;
	if ( progressDlg_ != NULL )
	{
		return progressDlg_->step( (int)progress );
	}
	return false;
}

void SingleTaskProgressBar::removeCurrentTask()
{
	BW_GUARD;
	if ( progressDlg_ != NULL )
	{
		delete progressDlg_;
		progressDlg_ = NULL;
		WorldManager::instance().endModalOperation();
	}
}

void SingleTaskProgressBar::createTask( const BW::string& sMsg, float steps, bool escapable )
{
	BW_GUARD;
	if ( progressDlg_ != NULL )
	{
		return;
	}
	progressDlg_ = new LabelledProgressDlg();
	progressDlg_->CenterWindow();
	progressDlg_->setStage( sMsg, 1, (int)steps, escapable );
	WorldManager::instance().beginModalOperation();
}

bool SingleTaskProgressBar::isCancelled()
{
	BW_GUARD;
	if ( progressDlg_ != NULL )
	{
		return progressDlg_->cancelled();
	}
	return true;
}

void SingleTaskProgressBar::length( float length )
{
	BW_GUARD;
	if ( progressDlg_ != NULL )
	{
		progressDlg_->length( (int)length );
	}
}

bool SingleTaskProgressBar::set( float done /*= 0.f */ )
{
	BW_GUARD;
	if ( progressDlg_ != NULL )
	{
		progressDlg_->SetPos( (int)done );
		return true;
	}
	return false;
}

/************************************************************************/
/*  MultiTaskProgressBarManager                                         */
/************************************************************************/
MultiTaskProgressBar::MultiTaskProgressBar() :
	taskCount_(0),
	currentTask_(0),
	progressDlg_(NULL),
	inited_(false)
{

}

MultiTaskProgressBar::~MultiTaskProgressBar()
{
	BW_GUARD;
	this->finishAll();
}

bool MultiTaskProgressBar::init( int taskCnt )
{
	if ( inited_ )
	{
		return false;
	}
	this->finishAll();
	inited_ = true;
	taskCount_ = taskCnt;
	progressDlg_ = new LabelledMultiTaskProgressDlg();
	progressDlg_->setTaskCnt( taskCount_ );

	return true;
}

void MultiTaskProgressBar::moveToNextTask()
{
	if ( !inited_ && progressDlg_  == NULL)
	{
		return;
	}
	currentTask_ ++;
	progressDlg_->setCurrentTask( currentTask_ );
}

void MultiTaskProgressBar::finishAll()
{
	taskCount_ = 0;
	inited_ = false;
	currentTask_ = 0;
	if ( progressDlg_ )
	{
		delete progressDlg_;
		progressDlg_ = NULL;
	}
}


bool MultiTaskProgressBar::isInited()
{
	return inited_;
}


void MultiTaskProgressBar::startProgress( const BW::string& sMsg, float steps, bool escapable )
{
	if ( !inited_ && progressDlg_  == NULL)
	{
		return;
	}
	progressDlg_->setCurrentTaskInfo( sMsg, steps, escapable );
}

void MultiTaskProgressBar::length( float length )
{
	if ( !inited_ && progressDlg_  == NULL)
	{
		return;
	}
	progressDlg_->setCurrentLength( length );
}

void MultiTaskProgressBar::progressFinished()
{
	this->moveToNextTask();
	return;
}

bool MultiTaskProgressBar::step( float progress/* = 1*/ )
{
	if ( !inited_ && progressDlg_  == NULL)
	{
		return false;
	}
	return progressDlg_->step( progress );
}

bool MultiTaskProgressBar::set( float done/* = 0.f*/ )
{
	if ( !inited_ && progressDlg_  == NULL)
	{
		return false;
	}
	return progressDlg_->set( done );
}

bool MultiTaskProgressBar::isCancelled()
{
	if ( !inited_ && progressDlg_  == NULL)
	{
		return false;
	}
	return progressDlg_->isCancelled();
}


/************************************************************************/
/*      ProgressBarTask                                                 */
/************************************************************************/

ProgressBarTask::ProgressBarTask( const BW::string& sMsg, float steps, bool escapable ):
	stepped_(false),
	progressbar_(NULL)
{
	BW_GUARD;

	//if there is no progress bar in using at the moment, we create a singleProgressBar as default
	if ( ProgressBar::getCurrentProgressBar() == NULL )
	{
		progressbar_ = new SingleTaskProgressBar();
		ProgressBar::setCurrentProgressBar( progressbar_ );
	}

	ProgressBar::getCurrentProgressBar()->startProgress( sMsg, steps, escapable );
}

ProgressBarTask::~ProgressBarTask()
{
	BW_GUARD;
	//If it didn't make any progress, play a progress animation.
	if( !stepped_ )
	{
		this->length( 2 );
		for ( int i = 0; i < 2; i++ )
		{
			this->step();
			Sleep( 20 );
		}
	}
	MF_ASSERT( ProgressBar::getCurrentProgressBar() != NULL );
	ProgressBar::getCurrentProgressBar()->progressFinished();
	if ( progressbar_ )
	{
		if ( progressbar_ == ProgressBar::getCurrentProgressBar() )
		{
			ProgressBar::setCurrentProgressBar( NULL );
		}	
		delete progressbar_;
		progressbar_ = NULL;
	}
}

bool ProgressBarTask::step( float progress /*= 1 */ )
{
	BW_GUARD;
	stepped_ = true;
	MF_ASSERT( ProgressBar::getCurrentProgressBar() != NULL );
	return ProgressBar::getCurrentProgressBar()->step( progress );
}

bool ProgressBarTask::isCancelled()
{
	BW_GUARD;
	MF_ASSERT( ProgressBar::getCurrentProgressBar() != NULL );
	return ProgressBar::getCurrentProgressBar()->isCancelled();
}

void ProgressBarTask::length( float length )
{
	BW_GUARD;
	MF_ASSERT( ProgressBar::getCurrentProgressBar() != NULL );
	return ProgressBar::getCurrentProgressBar()->length( length );
}

bool ProgressBarTask::set( float done /*= 0.f */ )
{
	BW_GUARD;
	MF_ASSERT( ProgressBar::getCurrentProgressBar() != NULL );
	return ProgressBar::getCurrentProgressBar()->set( done );
}

/************************************************************************/
/*      ScopedProgressBar                                               */
/************************************************************************/
ScopedProgressBar::ScopedProgressBar( int taskCnt ):
	progressbar_(NULL),
		taskCnt_(taskCnt)
{
	MF_ASSERT( taskCnt > 0 );

	if (ProgressBar::getCurrentProgressBar())
	{
		return;
	}

	if (taskCnt > 1)
	{
		MultiTaskProgressBar * progressbar = new MultiTaskProgressBar();
		progressbar->init( taskCnt );
		progressbar_ = progressbar;
	}
	else if (taskCnt == 1)
	{
		progressbar_ = new SingleTaskProgressBar();
	}

	ProgressBar::setCurrentProgressBar( progressbar_ );
}

ScopedProgressBar::~ScopedProgressBar()
{
	if ( progressbar_ )
	{
		if ( taskCnt_ > 1 )
		{
			MultiTaskProgressBar* progressbar = (MultiTaskProgressBar*) progressbar_;
			progressbar->finishAll();
		}
		delete progressbar_;
		progressbar_ = NULL;
		ProgressBar::setCurrentProgressBar( NULL );
	}
	
}
BW_END_NAMESPACE

