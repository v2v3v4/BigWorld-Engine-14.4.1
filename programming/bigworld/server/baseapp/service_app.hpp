#ifndef SERVICE_APP_HPP
#define SERVICE_APP_HPP

#include "baseapp.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class specialises BaseApp to make it a ServiceApp.
 */
class ServiceApp : public BaseApp
{
public:
	ENTITY_APP_HEADER( ServiceApp, baseApp )

	ServiceApp( Mercury::EventDispatcher & mainDispatcher,
		  Mercury::NetworkInterface & internalInterface ) :
		BaseApp( mainDispatcher, internalInterface, true )
	{
	}
};

BW_END_NAMESPACE

#endif // SERVICE_APP_HPP
