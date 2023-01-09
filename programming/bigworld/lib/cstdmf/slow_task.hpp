#ifndef SLOW_TASK_HPP
#define SLOW_TASK_HPP

BW_BEGIN_NAMESPACE

/**
 *	This class will be called when a slow task is started or ended.
 *	Classes that want to be notified when a long task is being
 *	processed should inherit from this class.
 */
class SlowTaskHandler
{
public:
	virtual ~SlowTaskHandler(){}

	virtual void startSlowTask() = 0;
	virtual void stopSlowTask() = 0;

	static SlowTaskHandler*& handler()
	{
		static SlowTaskHandler* s_handler = NULL;
		return s_handler;
	}

	static void handler( SlowTaskHandler* sth )
	{
		handler() = sth;
	}
};


/**
 *	This classes can be used as a scoped guard and provides
 *	a safe and automatical way to calling SlowTaskHandler.
 */
class SlowTask
{
public:
	SlowTask()
	{
		if (SlowTaskHandler::handler())
		{
			SlowTaskHandler::handler()->startSlowTask();
		}
	}

	~SlowTask()
	{
		if (SlowTaskHandler::handler())
		{
			SlowTaskHandler::handler()->stopSlowTask();
		}
	}
};

BW_END_NAMESPACE

#endif//SLOW_TASK_HPP
