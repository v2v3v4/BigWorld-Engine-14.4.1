#ifndef UPDATE_MOVEMENT_CALLABLE_WATCHER_HPP
#define UPDATE_MOVEMENT_CALLABLE_WATCHER_HPP

#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

class UpdateMovementCallableWatcher : public SimpleCallableWatcher
{
public:
	UpdateMovementCallableWatcher();

private:
	bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest );
	bool onCall( BW::string & output, BW::string & value,
		int parameterCount, BinaryIStream & parameters );
};

BW_END_NAMESPACE

#endif // UPDATE_MOVEMENT_CALLABLE_WATCHER_HPP
