#ifndef USERMAP_HPP
#define USERMAP_HPP

#include "network/machine_guard.hpp"


BW_BEGIN_NAMESPACE

class UserMap
{
public:
	UserMap();

	void add( const UserMessage &um );
	UserMessage* add( struct passwd *ent );

	bool getEnv( UserMessage & um, bool userAlreadyKnown = false );
	UserMessage* fetch( uint16 uid );
	bool setEnv( const UserMessage &um );
	void flush();

	friend class BWMachined;

protected:
	typedef BW::map< uint16, UserMessage > Map;
	Map map_;
	UserMessage notfound_;

	void queryUserConfs();
};

BW_END_NAMESPACE

#endif
