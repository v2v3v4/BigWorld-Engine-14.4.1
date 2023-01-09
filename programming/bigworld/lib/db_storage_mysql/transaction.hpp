#ifndef MYSQL_TRANSACTION_HPP
#define MYSQL_TRANSACTION_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class MySql;

/**
 *	This class handles a MySQL transaction. The transaction will be rolled back
 *	if commit() has not be called when an object of this type leaves scope.
 */
class MySqlTransaction
{
public:
	MySqlTransaction( MySql & sql );
	// MySqlTransaction( MySql & sql, int & errorNum );
	~MySqlTransaction();

	bool shouldRetry() const;

	void commit();

private:
	MySqlTransaction( const MySqlTransaction& );
	void operator=( const MySqlTransaction& );

	MySql & sql_;
	bool committed_;
};

BW_END_NAMESPACE

#endif // MYSQL_TRANSACTION_HPP
