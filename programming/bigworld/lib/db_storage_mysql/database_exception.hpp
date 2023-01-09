#ifndef DATABASE_EXCEPTION_HPP
#define DATABASE_EXCEPTION_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"
#include <mysql/mysql.h>


BW_BEGIN_NAMESPACE

/**
 *	This class is an exception that can be thrown when a database query fails.
 */
class DatabaseException : public std::exception
{
public:
	DatabaseException( MYSQL * pConnection );
	~DatabaseException() throw();

	virtual const char * what() const throw() { return errStr_.c_str(); }

	bool shouldRetry() const;
	bool isLostConnection() const;

private:
	BW::string errStr_;
	unsigned int errNum_;
};

BW_END_NAMESPACE

#endif // DATABASE_EXCEPTION_HPP
