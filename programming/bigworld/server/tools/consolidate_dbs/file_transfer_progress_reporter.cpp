#include "file_transfer_progress_reporter.hpp"

#include <sstream>


BW_BEGIN_NAMESPACE

/**
 * 	Override SluggishProgressReporter method
 */
void FileTransferProgressReporter::reportProgressNow()
{
	BW::stringstream ss;
	if (startedFiles_ < numFiles_)
	{
		ss << "Data consolidation is waiting for "
			<< " all secondary database file transfers to start ("
			<< startedFiles_ << '/' << numFiles_ << " started)";
	}
	else
	{
		ss << "Transfering secondary databases for consolidation ("
			<< doneBytes_ << '/' << numBytes_ << " bytes) ("
			<< doneFiles_ << '/' << numFiles_ << " files completed)";
	}

	this->reporter().onStatus( ss.str() );
}

BW_END_NAMESPACE

// file_transfer_progress_reporter.cpp

