#include "consolidation_progress_reporter.hpp"

#include <sstream>


BW_BEGIN_NAMESPACE

/**
 * 	Override SluggishProgressReporter method
 */
void ConsolidationProgressReporter::reportProgressNow()
{
	// Generate string
	BW::stringstream ss;
	ss << "Consolidating " << curDBName_ << " (" << doneEntitiesInCurDB_
		<< '/' << numEntitiesInCurDB_ << " entities)"
		<< " (" << doneDBs_ << '/' << numDBs_ << " databases)";

	this->reporter().onStatus( ss.str() );
}

BW_END_NAMESPACE

// consolidation_progress_reporter.cpp

