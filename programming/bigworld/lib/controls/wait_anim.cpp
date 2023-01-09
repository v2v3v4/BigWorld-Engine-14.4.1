#include "pch.hpp"
#include "wait_anim.hpp"
#include "cstdmf/timestamp.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{

const int DEFAULT_HIDE_WAIT_MILLIS = 500;


WaitAnim::WaitAnim() :
	CAnimateCtrl(),
	inited_( false ),
	width_( 0 ),
	height_( 0 ),
	maxFPS_( 10 ),
	numFrames_(),
	working_( false ),
	workingStopped_( false ),
	workingStoppedTimer_( 0 ),
	hideWaitMillis_( 0 ),
	frame_( 0 ),
	frameTimestamp_( 0 )
{
}


bool WaitAnim::init( int resID, int width, int height, int maxFPS, int numFrames )
{
	if (width < 1 || height < 1 || maxFPS < 1 || maxFPS > 1000 || numFrames < 1)
	{
		return false;
	}

	if (!this->Open( MAKEINTRESOURCE( resID ) ))
	{
		return false;
	}

	width_ = width;
	height_ = height;
	maxFPS_ = maxFPS;
	numFrames_ = numFrames;

	inited_ = true;
	return true;
}


void WaitAnim::update()
{
	BW_GUARD;

	if (!inited_)
	{
		return;
	}

	if (workingStopped_ &&
		(timestamp() - workingStoppedTimer_) * 1000 / stampsPerSecond() >= hideWaitMillis_)
	{
		workingStopped_ = false;
		working_ = false;
	}

	if (working_)
	{
		if (!this->IsWindowVisible())
		{
			this->ShowWindow( SW_SHOW );
		}
		this->Seek( frame_ % numFrames_ );
		if ((timestamp() - frameTimestamp_) * 1000 / stampsPerSecond() > 1000 / maxFPS_)
		{
			frame_++;
			frameTimestamp_ = timestamp();
		}
	}
	else
	{
		if (this->IsWindowVisible())
		{
			this->ShowWindow( SW_HIDE );
		}
	}
}


void WaitAnim::show()
{
	working_ = true;
	workingStopped_ = false;
}


void WaitAnim::hide( int waitMillis )
{
	if (waitMillis == USE_DEFAULT_MILLIS)
	{
		hideWaitMillis_ = DEFAULT_HIDE_WAIT_MILLIS;
	}
	else
	{
		hideWaitMillis_ = waitMillis;
	}

	if (working_ && !workingStopped_)
	{
		workingStopped_ = true;
		workingStoppedTimer_ = timestamp();
	}
}

} // namespace controls

BW_END_NAMESPACE

