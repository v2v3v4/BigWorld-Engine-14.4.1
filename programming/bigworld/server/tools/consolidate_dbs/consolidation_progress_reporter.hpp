#ifndef CONSOLIDATE_DBS__CONSOLIDATION_PROGRESS_REPORTER_HPP
#define CONSOLIDATE_DBS__CONSOLIDATION_PROGRESS_REPORTER_HPP

#include "sluggish_progress_reporter.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This object is passed around to various operations to so that there is a
 * 	a single object that knows about the progress of consolidation and can
 * 	report it to DBApp.
 */
class ConsolidationProgressReporter : private SluggishProgressReporter
{
public:

	/**
	 *	Constructor.
	 */
	ConsolidationProgressReporter( DBAppStatusReporter & reporter, int numDBs ) :
		SluggishProgressReporter( reporter ),
		numDBs_( numDBs ),
		doneDBs_( 0 ),
		numEntitiesInCurDB_( 0 ),
		doneEntitiesInCurDB_( 0 )
	{}


	void onStartConsolidateDB( const BW::string & dbName, int numEntities )
	{
		++doneDBs_;
		curDBName_ = dbName;
		numEntitiesInCurDB_ = numEntities;
		doneEntitiesInCurDB_ = 0;

		this->reportProgressNow();
	}


	void onConsolidatedRow()
	{
		++doneEntitiesInCurDB_;
		this->reportProgress();	// SluggishProgressReporter method
	}

private:
	virtual void reportProgressNow();

private:
	int			numDBs_;
	int			doneDBs_;	// Actualy counts the one currently being done
	BW::string curDBName_;
	int			numEntitiesInCurDB_;
	int			doneEntitiesInCurDB_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS__CONSOLIDATION_PROGRESS_REPORTER_HPP
