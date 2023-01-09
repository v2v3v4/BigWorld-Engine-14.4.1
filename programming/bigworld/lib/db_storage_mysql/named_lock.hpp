#ifndef MYSQL_NAMEDLOCK_HPP
#define MYSQL_NAMEDLOCK_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class MySql;

namespace MySQL
{
/**
 * 	This class obtains and releases an named lock.
 */
class NamedLock
{
public:
	NamedLock( const BW::string & lockName = BW::string() );
	~NamedLock();

	bool lock( MySql & connection );
	bool unlock();

	const BW::string& lockName() const 	{ return lockName_; }
	bool isLocked() const 					{ return pConnection_ != NULL; }

private:
	MySql *		pConnection_;
	BW::string lockName_;
};

}	// namespace MySQL

BW_END_NAMESPACE

#endif // MYSQL_NAMEDLOCK_HPP
