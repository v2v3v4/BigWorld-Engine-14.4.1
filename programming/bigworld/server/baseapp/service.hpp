#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "base.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.Service
 *	@components{ base }
 *
 *	Service is a special type of Base. It is stateless and has no corresponding
 *	Cell or Client parts.
 */

/**
 *	This class is used to represent a service fragment. A Service is a special
 *	type of base that run on ServiceApps.
 */
class Service: public Base
{
	Py_Header( Service, Base )

public:
	Service( EntityID id, DatabaseID dbID, EntityType * pType );
};

BW_END_NAMESPACE

#endif // SERVICE_HPP
