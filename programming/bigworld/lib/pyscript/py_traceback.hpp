#ifndef PY_TRACEBACK_HPP
#define PY_TRACEBACK_HPP

BW_BEGIN_NAMESPACE

// Forward declaration
namespace Mercury
{
class EventDispatcher;
} // namespace Mercury 

namespace Script
{
	void initExceptionHook( Mercury::EventDispatcher * pEventDispatcher );
} // namespace Script

BW_END_NAMESPACE

#endif // PY_TRACEBACK_HPP
