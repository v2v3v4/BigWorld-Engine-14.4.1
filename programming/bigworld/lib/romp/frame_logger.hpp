#ifndef FRAME_LOGGER_HPP
#define FRAME_LOGGER_HPP

#include "cstdmf/main_loop_task.hpp"

BW_BEGIN_NAMESPACE

/** 
 *	This class records per-frame statistics and writes them to a log file.
 *	It can be activated and customised through the Debug/FrameLogger watchers.
 */
class FrameLogger
{
public:	
	static void init();
};

BW_END_NAMESPACE

#endif // FRAME_LOGGER_HPP
