#ifndef STREAM_TO_QUERY_HELPER_HPP
#define STREAM_TO_QUERY_HELPER_HPP

#include "../query.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

class ChildQuery;
class MySql;
class Query;
class QueryRunner;


/**
 *	This class is used as a helper when running PropertyMapping::fromStreamToDatabase.
 */
class StreamToQueryHelper
{
public:
	StreamToQueryHelper( MySql & connection, DatabaseID parentID );
	~StreamToQueryHelper();

	void executeBufferedQueries( DatabaseID parentID );

	ChildQuery & createChildQuery( const Query & query );

	MySql & connection()				{ return connection_; }

	DatabaseID parentID() const			{ return parentID_; }
	void parentID( DatabaseID id )		{ parentID_ = id; }

	bool hasBufferedQueries() const		{ return !bufferedQueries_.empty(); }

private:
	MySql & connection_;
	DatabaseID parentID_;

	typedef BW::list< ChildQuery * > BufferedQueries;
	BufferedQueries bufferedQueries_;
};

/**
 *	This class represents a query for a child table that needs to be queued
 *	for execution after it's parent query has been executed. This is required
 *	for situations such as writing out sub-tables of entities that require
 *	the databaseID of the entity record in order to maintain data integrity.
 */
class ChildQuery
{
public:
	ChildQuery( MySql & connection, const Query & query );

	void execute( DatabaseID parentID );

	QueryRunner & queryRunner()			{ return queryRunner_; }
	StreamToQueryHelper & helper()		{ return helper_; }

private:
	QueryRunner queryRunner_;
	StreamToQueryHelper helper_;
};

BW_END_NAMESPACE

#endif // STREAM_TO_QUERY_HELPER_HPP
