#ifndef DEL_BOTS_CALLABLE_WATCHER_HPP
#define DEL_BOTS_CALLABLE_WATCHER_HPP

#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

class DelBotsCallableWatcher : public SimpleCallableWatcher
{
public:
	DelBotsCallableWatcher();

private:
	bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest );
	bool onCall( BW::string & output, BW::string & value,
		int parameterCount, BinaryIStream & parameters );
};

BW_END_NAMESPACE

#endif // DEL_BOTS_CALLABLE_WATCHER_HPP
