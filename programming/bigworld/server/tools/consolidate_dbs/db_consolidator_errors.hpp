#ifndef DB_CONSOLIDATOR_ERRORS_HPP
#define DB_CONSOLIDATOR_ERRORS_HPP

#include "cstdmf/debug.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This class collects all the errors during consolidation.
 */
class DBConsolidatorErrors
{
public:
	void addSecondaryDB( const BW::string & secondaryDBFileName )
	{
		HACK_BACKTRACE();

		erroneousSecondaryDBs_.insert( secondaryDBFileName );
	}

	bool hasErrors() const 
		{ return !erroneousSecondaryDBs_.empty(); }

	bool secondaryDBHasError( const BW::string & secondaryDBFileName ) const
	{
		return (erroneousSecondaryDBs_.find( secondaryDBFileName ) !=
				erroneousSecondaryDBs_.end());
	}

private:
	typedef BW::set< BW::string > Container;
	Container erroneousSecondaryDBs_;
};

BW_END_NAMESPACE

#endif // DB_CONSOLIDATOR_ERRORS_HPP

