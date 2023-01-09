#ifndef DISPATCHER_COUPLING_HPP
#define DISPATCHER_COUPLING_HPP

#include "event_dispatcher.hpp"
#include "frequent_tasks.hpp"
#include "cstdmf/profiler.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This class is used to join to Dispatchers. Generally, there is only one
 *	Dispatcher per thread. To support more than one, this class can be used to
 *	attach one to another.
 */
class DispatcherCoupling : public FrequentTask
{
public:
	DispatcherCoupling( EventDispatcher & mainDispatcher,
				EventDispatcher & childDispatcher ) :
		FrequentTask( "DispatcherCoupling" ),
			mainDispatcher_( mainDispatcher ),
			childDispatcher_( childDispatcher )
	{
		mainDispatcher.addFrequentTask( this );
	}

	~DispatcherCoupling()
	{
		mainDispatcher_.cancelFrequentTask( this );
	}

private:
	void doTask()
	{
		childDispatcher_.processOnce();
	}

	EventDispatcher & mainDispatcher_;
	EventDispatcher & childDispatcher_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // DISPATCHER_COUPLING_HPP

