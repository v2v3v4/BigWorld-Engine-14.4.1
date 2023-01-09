#include "batch_compiler.hpp"

#include "asset_pipeline/conversion/content_addressable_cache.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "asset_pipeline/compiler/asset_compiler_options.hpp"

#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/multi_file_system.hpp"

#include "cstdmf/command_line.hpp"
#include "cstdmf/date_time_utils.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "cstdmf/timestamp.hpp"

#include <time.h>

DECLARE_DEBUG_COMPONENT2( ASSET_PIPELINE_CATEGORY, 0 )

BW_BEGIN_NAMESPACE

THREADLOCAL( BatchCompiler::TaskRecord * ) BatchCompiler::s_currentTaskRecord = NULL;
THREADLOCAL( uint64 ) BatchCompiler::s_currentTaskStartTime = 0;

DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( false )

namespace BatchCompiler_Locals
{
	BatchCompiler * batchCompiler_ = NULL;

	BOOL WINAPI ConsoleHandler( DWORD ctrl_type )
	{
		if ( ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT ) 
		{
			if (batchCompiler_->terminating())
			{
				// Stop all the executing threads immediately
				BgTaskManager::instance().stopAll( true, false );
			}
			else
			{
				// Allow the batch compiler to terminate gracefully
				batchCompiler_->terminate();
			}
			return TRUE;
		}
		return FALSE;
	}
}

BatchCompiler::BatchCompiler()
: AssetCompiler()
, totalDuration_( 0 )
, cacheReadCount_( 0 )
, cacheReadMissCount_( 0 )
, cacheWriteCount_( 0 )
, cacheWriteMissCount_( 0 )
, filesIterated_( 0 )
, directoriesIterated_( 0 )
, taskCount_( 0 )
, taskFailedCount_( 0 )
, taskUpToDateCount_( 0 )
, taskSkippedCount_( 0 )
{
	addToolsResourcePaths();
}

BatchCompiler::~BatchCompiler()
{
	taskRecordsMutex_.grab();
	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		 it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		bw_safe_delete( it->second );
	}
	taskRecords_.clear();
	taskRecordsMutex_.give();
}

void BatchCompiler::build( const BW::vector< BW::string > & paths )
{
	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::FileType ft;

	uint64 startTime = timestamp();

	MF_ASSERT( taskQueue_.empty() );

	for (BW::vector< BW::string >::const_iterator 
		it = paths.begin(); it != paths.end(); ++it)
	{
		ft = fs->getFileType( *it );
		if (ft == IFileSystem::FT_FILE)
		{
			INFO_MSG( "========== Processing File: %s ==========\n", it->c_str() );
			ConversionTask & task = taskFinder_.getTask( *it );
			if (task.converterId_ == ConversionTask::s_unknownId)
			{
				ERROR_MSG( "Could not find task for file %s\n", it->c_str() );
			}
			else
			{
				// Queue the non root task
				queueTask( task );
			}
		}
		else if (ft == IFileSystem::FT_DIRECTORY)
		{
			INFO_MSG( "========== Processing Directory: %s ==========\n", it->c_str() );
			taskFinder_.findTasks( *it );
		}
	}

	INFO_MSG( "========== Found: %d tasks, Searched: %d files, %d directories ==========\n",
		taskQueue_.size(), 
		filesIterated_, 
		directoriesIterated_ );

	taskProcessor_.processTasks();
	INFO_MSG( "========== Conversion: %d succeeded, %d failed, %d converted, %d up-to-date, %d skipped ==========\n",
		taskCount_ - taskFailedCount_, 
		taskFailedCount_, 
		taskCount_ - ( taskFailedCount_ + taskUpToDateCount_ + taskSkippedCount_ ),
		taskUpToDateCount_,
		taskSkippedCount_ );

	uint64 endTime = timestamp();
	totalDuration_ = (double)(((int64)(endTime - startTime)) / stampsPerSecondD());
}

void BatchCompiler::outputReport( const StringRef & filename )
{
	BW_GUARD;

	taskRecordsMutex_.grab();

	// Create a new html file
	DataResource reportResource( filename.to_string(), RESOURCE_TYPE_XML, true );
	DataSectionPtr rootSection = reportResource.getRootSection();
	rootSection->delChildren();
	rootSection->save( "html" );

	// Create the html head
	DataSectionPtr headSection = rootSection->newSection( "head" );

	// Insert the css style
	DataSectionPtr css = BWResource::openSection( "resources/report_style.css" );
	if (css != NULL)
	{
		DataSectionPtr styleSection = headSection->newSection( "style" );
		DataSectionPtr typeSection = styleSection->newSection( "type" );
		typeSection->isAttribute( true );
		typeSection->setString( "text/css" );
		styleSection->setString( StringRef( css->asBinary()->cdata(), css->asBinary()->len() ) );
		styleSection->noXMLEscapeSequence( true );
	}

	// Insert the javascript script
	DataSectionPtr javascript = BWResource::openSection( "resources/report_script.js" );
	if (javascript != NULL)
	{
		DataSectionPtr scriptSection = headSection->newSection( "script" );
		DataSectionPtr typeSection = scriptSection->newSection( "type" );
		typeSection->isAttribute( true );
		typeSection->setString( "text/javascript" );
		scriptSection->setString( StringRef( javascript->asBinary()->cdata(), javascript->asBinary()->len() ) );
		scriptSection->noXMLEscapeSequence( true );
	}

	time_t ttime = ::time( NULL );
	BW::string formattedTime;
	DateTimeUtils::format( formattedTime, ttime, true );

	// Create the html body
	DataSectionPtr bodySection = rootSection->newSection( "body" );

	// Output the heading
	StringBuilder heading(1024);
	heading.append( "Batch Compiler " );
	heading.append( formattedTime );
	writeHeading( bodySection, heading.string() );

	// Output the command line information
	DataSectionPtr infoSection = writeSection( bodySection, "", "" );
	writeText( infoSection, bw_wtoutf8( GetCommandLine() ), "" );

	writeBreak( bodySection );

	// Output the summary
	DataSectionPtr summarySection = writeSection( bodySection, "", "Group" );
	writeText( summarySection, "Summary", "font-size:150%;font-weight:bold;" );

	BW::string discoverySummary = bw_format( "Found: %d tasks, Searched: %d files, %d directories",
		taskQueue_.size(), 
		filesIterated_, 
		directoriesIterated_ );
	writeText( summarySection, discoverySummary, "" );

	BW::string conversionSummary = bw_format( "Conversion: %d succeeded, %d failed, %d converted, %d up-to-date, %d skipped",
		taskCount_ - taskFailedCount_, 
		taskFailedCount_, 
		taskCount_ - ( taskFailedCount_ + taskUpToDateCount_ + taskSkippedCount_ ),
		taskUpToDateCount_,
		taskSkippedCount_ );
	writeText( summarySection, conversionSummary, "" );

	writeText( summarySection, 
		bw_format( "Cache: %d Reads, %d Failed Reads, %d Writes, %d Failed Writes", 
			cacheReadCount_, cacheReadMissCount_, cacheWriteCount_, cacheWriteMissCount_ ), "" );
	writeText( summarySection, bw_format( "Total Duration: %f seconds", totalDuration_ ), "" );

	// Output the tasks with failures
	DataSectionPtr subgroupSection = writeSection( summarySection, "", "SubGroup" );
	DataSectionPtr linkSection = writeLink( subgroupSection, "javascript:Toggle('Failures');", "", "color:#000000;font-size:125%;font-weight:bold;" );
	DataSectionPtr failuresSection = writeSection( subgroupSection, "Failures", "Toggleable" );
	uint32 numFailures = 0;
	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		if (it->second->hasError_)
		{
			++numFailures;
			BW::string link = bw_format( "javascript:GoTo('%d');", it->second->id_ );
			writeLink( failuresSection, link, it->first->source_, "" );
			writeBreak( failuresSection );
		}
	}
	linkSection->setString( bw_format( "%s (%d)", "Failures", numFailures ) );
	if (numFailures == 0)
	{
		subgroupSection->delChild(failuresSection);
	}

	// Output the tasks with warnings
	subgroupSection = writeSection( summarySection, "", "SubGroup" );
	linkSection = writeLink( subgroupSection, "javascript:Toggle('Warnings');", "", "color:#000000;font-size:125%;font-weight:bold;" );
	DataSectionPtr warningsSection = writeSection( subgroupSection, "Warnings", "Toggleable" );
	uint32 numWarnings = 0;
	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		if (!it->second->hasError_ && it->second->hasWarning_)
		{
			++numWarnings;
			BW::string link = bw_format( "javascript:GoTo('%d');", it->second->id_ );
			writeLink( warningsSection, link, it->first->source_, "" );
			writeBreak( warningsSection );
		}
	}
	linkSection->setString( bw_format( "%s (%d)", "Warnings", numWarnings ) );
	if (numWarnings == 0)
	{
		subgroupSection->delChild(warningsSection);
	}

	// Output the tasks that were converted
	subgroupSection = writeSection( summarySection, "", "SubGroup" );
	linkSection = writeLink( subgroupSection, "javascript:Toggle('Converted');", "", "color:#000000;font-size:125%;font-weight:bold;" );
	DataSectionPtr convertedSection = writeSection( subgroupSection, "Converted", "Toggleable" );
	uint32 numconverted = 0;
	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		if (!it->second->hasError_ && !it->second->upToDate_ && !it->second->skipped_)
		{
			++numconverted;
			BW::string link = bw_format( "javascript:GoTo('%d');", it->second->id_ );
			writeLink( convertedSection, link, it->first->source_, "" );
			writeBreak( convertedSection );
		}
	}
	linkSection->setString( bw_format( "%s (%d)", "Converted", numconverted ) );
	if (numconverted == 0)
	{
		subgroupSection->delChild(convertedSection);
	}

	// Output the tasks that were up to date
	subgroupSection = writeSection( summarySection, "", "SubGroup" );
	linkSection = writeLink( subgroupSection, "javascript:Toggle('UpToDate');", "", "color:#000000;font-size:125%;font-weight:bold;" );
	DataSectionPtr upToDateSection = writeSection( subgroupSection, "UpToDate", "Toggleable" );
	uint32 numUpToDate = 0;
	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		if (!it->second->hasError_ && !it->second->hasWarning_ && it->second->upToDate_ && !it->second->skipped_)
		{
			++numUpToDate;
			BW::string link = bw_format( "javascript:GoTo('%d');", it->second->id_ );
			writeLink( upToDateSection, link, it->first->source_, "" );
			writeBreak( upToDateSection );
		}
	}
	linkSection->setString( bw_format( "%s (%d)", "Up To Date", numUpToDate ) );
	if (numUpToDate == 0)
	{
		subgroupSection->delChild(upToDateSection);
	}

	// Output the tasks that were skipped
	subgroupSection = writeSection( summarySection, "", "SubGroup" );
	linkSection = writeLink( subgroupSection, "javascript:Toggle('Skipped');", "", "color:#000000;font-size:125%;font-weight:bold;" );
	DataSectionPtr skippedSection = writeSection( subgroupSection, "Skipped", "Toggleable" );
	uint32 numSkipped = 0;
	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		if (!it->second->hasError_ && !it->second->hasWarning_ && it->second->upToDate_ && it->second->skipped_)
		{
			++numSkipped;
			BW::string link = bw_format( "javascript:GoTo('%d');", it->second->id_ );
			writeLink( skippedSection, link, it->first->source_, "" );
			writeBreak( skippedSection );
		}
	}
	linkSection->setString( bw_format( "%s (%d)", "Skipped", numSkipped ) );
	if (numSkipped == 0)
	{
		subgroupSection->delChild(skippedSection);
	}

	writeBreak( bodySection );

	DataSectionPtr tasksSection = writeSection( bodySection, "", "Group" );
	writeText( tasksSection, "Tasks", "font-size:150%;font-weight:bold;" );

	for ( BW::map<const ConversionTask *, TaskRecord *>::iterator 
		it = taskRecords_.begin(); it != taskRecords_.end(); ++it )
	{
		BW::string link = bw_format( "javascript:Toggle('%d');", it->second->id_ );
		subgroupSection = writeSection( tasksSection, "", "SubGroup" );
		linkSection = writeLink( subgroupSection, link, it->first->source_, "color:#000000;font-size:112.5%;font-weight:bold;" );
		DataSectionPtr taskSection = writeSection( subgroupSection, bw_format( "%d", it->second->id_ ), "Toggleable" );
		if (it->second->hasError_)
		{
			writeText( taskSection, "FAILED", "font-weight:bold;" );
		}
		else if (it->second->hasWarning_)
		{
			writeText( taskSection, "SUCCEEDED - WARNINGS", "font-weight:bold;" );
		}
		else if (!it->second->upToDate_ && !it->second->skipped_)
		{
			writeText( taskSection, "SUCCEEDED", "font-weight:bold;" );
		}
		else if (!it->second->skipped_)
		{
			writeText( taskSection, "SUCCEEDED - UP TO DATE", "font-weight:bold;" );
		}
		else
		{
			writeText( taskSection, "SUCCEEDED - SKIPPED", "font-weight:bold;" );
		}
		writeText( taskSection, bw_format( "Duration: %f seconds", it->second->duration_ ), "" );
		if (!it->second->outputs_.empty())
		{
			writeBreak( taskSection );
			writeText( taskSection, "Outputs:", "" );
			for ( BW::vector<BW::string>::const_iterator
				outputIt = it->second->outputs_.begin(); outputIt != it->second->outputs_.end(); ++outputIt)
			{
				writeText( taskSection, *outputIt, "" );
			}
		}
		if (!it->first->subTasks_.empty())
		{
			writeBreak( taskSection );
			writeText( taskSection, "Sub Tasks:", "" );
			for ( ConversionTask::SubTaskList::const_iterator
				subIt = it->first->subTasks_.begin(); subIt != it->first->subTasks_.end(); ++subIt )
			{
				if (subIt->first->converterId_ == ConversionTask::s_unknownId)
				{
					writeText( taskSection, bw_format( "Unknown: %s", subIt->first->source_ ), "" );
					continue;
				}
				BW::map<const ConversionTask *, TaskRecord *>::iterator recordIt =
					taskRecords_.find( subIt->first );
				MF_ASSERT( recordIt != taskRecords_.end() );
				TaskRecord * subTaskRecord = recordIt->second;
				BW::string subLink = bw_format( "javascript:GoTo('%d');", subTaskRecord->id_ );
				writeLink( taskSection, subLink, subIt->first->source_, "" );
				writeBreak( taskSection );
			}
		}
		if (!it->second->log_.empty())
		{
			writeBreak( taskSection );
			writeText( taskSection, "Log:", "" );
			char * line = strtok( const_cast< char * >( it->second->log_.c_str() ), "\n" );
			while (line != NULL)
			{
				writeText( taskSection, line, "" );
				line = strtok( NULL, "\n" );
			}
		}
	}

	reportResource.save();

	taskRecordsMutex_.give();
}

DataSectionPtr BatchCompiler::writeSection( DataSectionPtr pDataSection, const StringRef & id, const StringRef & classType )
{
	BW_GUARD;

	DataSectionPtr section = pDataSection->newSection( "div" );
	if (id.length() > 0)
	{
		DataSectionPtr idSection = section->newSection( "id" );
		idSection->isAttribute( true );
		idSection->setString( id );
	}
	if (classType.length() > 0)
	{
		DataSectionPtr classSection = section->newSection( "class" );
		classSection->isAttribute( true );
		classSection->setString( classType );
	}
	return section;
}

DataSectionPtr BatchCompiler::writeBreak( DataSectionPtr pDataSection)
{
	BW_GUARD;
	return pDataSection->newSection( "br" );
}

DataSectionPtr BatchCompiler::writeHeading( DataSectionPtr pDataSection, const StringRef & heading, uint8 size )
{
	BW_GUARD;

	char * headingSize = "h1";
	switch (size)
	{
	case 2: headingSize = "h2"; break;
	case 3: headingSize = "h3"; break;
	case 4: headingSize = "h4"; break;
	case 5: headingSize = "h5"; break;
	case 6: headingSize = "h6"; break;
	}
	DataSectionPtr section = pDataSection->newSection( headingSize );
	section->setString( heading );
	section->noXMLEscapeSequence( true );
	return section;
}

DataSectionPtr BatchCompiler::writeText( DataSectionPtr pDataSection, const StringRef & text, const StringRef & style )
{
	BW_GUARD;

	DataSectionPtr section = writeSection( pDataSection, "", "" );
	if (style.length() > 0)
	{
		DataSectionPtr styleSection = section->newSection( "style" );
		styleSection->isAttribute( true );
		styleSection->setString( style );
	}
	section->setString( text );
	section->noXMLEscapeSequence( true );
	return section;
}

DataSectionPtr BatchCompiler::writeLink( DataSectionPtr pDataSection, const StringRef & link, const StringRef & text, const StringRef & style )
{
	BW_GUARD;

	DataSectionPtr section = pDataSection->newSection( "a" );
	if (link.length() > 0)
	{
		DataSectionPtr linkSection = section->newSection( "href" );
		linkSection->isAttribute( true );
		linkSection->setString( link );
	}
	if (style.length() > 0)
	{
		DataSectionPtr styleSection = section->newSection( "style" );
		styleSection->isAttribute( true );
		styleSection->setString( style );
	}
	section->setString( text );
	section->noXMLEscapeSequence( true );
	return section;
}

bool BatchCompiler::shouldIterateFile( const BW::StringRef & file )
{
	BW_GUARD;

	if (!AssetCompiler::shouldIterateFile( file ))
	{
		return false;
	}

	++filesIterated_;
	return true;
}

bool BatchCompiler::shouldIterateDirectory( const BW::StringRef & directory )
{
	BW_GUARD;

	if (!AssetCompiler::shouldIterateDirectory( directory ))
	{
		return false;
	}

	++directoriesIterated_;
	return true;
}

void BatchCompiler::onTaskStarted( ConversionTask & conversionTask )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord == NULL );

	// Create a new task record
	s_currentTaskRecord = new TaskRecord();

	// Insert it in our task record map
	taskRecordsMutex_.grab();
	taskRecords_[&conversionTask] = s_currentTaskRecord;
	taskRecordsMutex_.give();

	// Set the unique task record id
	s_currentTaskRecord->id_ = InterlockedIncrement( &taskCount_ );
	s_currentTaskStartTime = timestamp();

	INFO_MSG( "------ Task started: %s ------\n", conversionTask.source_.c_str() );

	AssetCompiler::onTaskStarted( conversionTask );
}

void BatchCompiler::onTaskResumed( ConversionTask & conversionTask )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord == NULL );

	// Retrieve the task record from the task record map
	taskRecordsMutex_.grab();
	BW::map<const ConversionTask *, TaskRecord *>::iterator it =
		taskRecords_.find( &conversionTask );
	MF_ASSERT( it != taskRecords_.end() );
	s_currentTaskRecord = it->second;
	taskRecordsMutex_.give();

	s_currentTaskStartTime = timestamp();

	AssetCompiler::onTaskResumed( conversionTask );
}

void BatchCompiler::onTaskSuspended( ConversionTask & conversionTask )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord != NULL );

	// Update the tasks duration
	uint64 currentTime = timestamp();
	s_currentTaskRecord->duration_ += (double)(((int64)(currentTime - s_currentTaskStartTime)) / stampsPerSecondD());

	// Clear the current task record
	s_currentTaskRecord = NULL;
	s_currentTaskStartTime = 0;

	AssetCompiler::onTaskSuspended( conversionTask );
}

void BatchCompiler::onTaskCompleted( ConversionTask & conversionTask )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord != NULL );

	if (conversionTask.status_ == ConversionTask::FAILED)
	{
		InterlockedIncrement( &taskFailedCount_ );
	}
	else if (s_currentTaskRecord->skipped_)
	{
		InterlockedIncrement( &taskSkippedCount_ );
	}
	else if (s_currentTaskRecord->upToDate_)
	{
		InterlockedIncrement( &taskUpToDateCount_ );
	}

	// Update the tasks duration
	uint64 currentTime = timestamp();
	s_currentTaskRecord->duration_ += (double)(((int64)(currentTime - s_currentTaskStartTime)) / stampsPerSecondD());

	// Clear the current task record
	s_currentTaskRecord= NULL;
	s_currentTaskStartTime = 0;

	AssetCompiler::onTaskCompleted( conversionTask );
}

void BatchCompiler::onPreCreateDependencies( ConversionTask & conversionTask )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord != NULL );

	s_currentTaskRecord->skipped_ = false;

	AssetCompiler::onPreCreateDependencies( conversionTask );
}

void BatchCompiler::onPreConvert( ConversionTask & conversionTask )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord != NULL );

	s_currentTaskRecord->upToDate_ = false;
	s_currentTaskRecord->skipped_ = false;

	AssetCompiler::onPreConvert( conversionTask );
}

void BatchCompiler::onOutputGenerated( const BW::string & filename )
{
	BW_GUARD;

	MF_ASSERT( s_currentTaskRecord != NULL );

	s_currentTaskRecord->outputs_.push_back( filename );

	AssetCompiler::onOutputGenerated( filename );
}

void BatchCompiler::onCacheRead( const BW::string & filename )
{
	BW_GUARD;

	InterlockedIncrement( &cacheReadCount_ );

	AssetCompiler::onCacheRead( filename );
}

void BatchCompiler::onCacheReadMiss( const BW::string & filename )
{
	BW_GUARD;

	InterlockedIncrement( &cacheReadMissCount_ );

	AssetCompiler::onCacheReadMiss( filename );
}

void BatchCompiler::onCacheWrite( const BW::string & filename )
{
	BW_GUARD;

	InterlockedIncrement( &cacheWriteCount_ );

	AssetCompiler::onCacheWrite( filename );
}

void BatchCompiler::onCacheWriteMiss( const BW::string & filename )
{
	BW_GUARD;

	InterlockedIncrement( &cacheWriteMissCount_ );

	AssetCompiler::onCacheWriteMiss( filename );
}


/*
 * Override from DebugMessageCallback.
 */
bool BatchCompiler::handleMessage( DebugMessagePriority messagePriority, 
								   const char * pCategory,
								   DebugMessageSource messageSource,
								   const LogMetaData & metaData,
								   const char * pFormat, 
								   va_list argPtr )
{
	BW_GUARD;

#if !ASSET_PIPELINE_CAPTURE_ASSERTS
	if (messagePriority == DebugMessagePriority::MESSAGE_PRIORITY_CRITICAL)
	{
		return false;
	}
#endif

	bool handled = AssetCompiler::handleMessage( messagePriority,
												pCategory,
												messageSource,
												metaData,
												pFormat,
												argPtr);

	if (s_CreatingDependencies || s_Converting)
	{
		switch (messagePriority)
		{
		case DebugMessagePriority::MESSAGE_PRIORITY_CRITICAL:
			if (s_currentTaskRecord != NULL)
			{
				s_currentTaskRecord->hasError_ = true;

				char buf[2048];
				bw_vsnprintf( buf, 2048, pFormat, argPtr );
				char * line = strtok( buf, "\n" );
				// Ignore the first three lines of the assert message
				for ( int i = 0; i < 3; ++i )
				{
					line = strtok( NULL, "\n" );
				}
				// Print out the next two lines of the assert message.
				// The assertion and the file. Don't print out the callstack
				if (line != NULL)
				{
					size_t logSize = s_currentTaskRecord->log_.size();
					s_currentTaskRecord->log_.append( line );
					s_currentTaskRecord->log_.append( "\n" );
					const char * msg = s_currentTaskRecord->log_.c_str() + logSize;
					fprintf( stdout, "%d>  %s", s_currentTaskRecord->id_, msg );
				}
				line = strtok( NULL, "\n" );
				if (line != NULL)
				{
					size_t logSize = s_currentTaskRecord->log_.size();
					s_currentTaskRecord->log_.append( line );
					s_currentTaskRecord->log_.append( "\n" );
					const char * msg = s_currentTaskRecord->log_.c_str() + logSize;
					fprintf( stdout, "%d>  %s", s_currentTaskRecord->id_, msg );
				}

				handled = true;
			}
			break;

		case DebugMessagePriority::MESSAGE_PRIORITY_ERROR:
			if (s_currentTaskRecord != NULL)
			{
				s_currentTaskRecord->hasError_ = true;

				char reformat[2048];
				sprintf( reformat, "ERROR: %s", pFormat );
				size_t logSize = s_currentTaskRecord->log_.size();
				size_t msgSize = _vscprintf( reformat, argPtr );
				s_currentTaskRecord->log_.resize( logSize + msgSize );
				char * msg = const_cast< char * >( s_currentTaskRecord->log_.c_str() + logSize );
				MF_VERIFY( _vsnprintf( msg, msgSize, reformat, argPtr ) >= 0 );
				fprintf( stdout, "%d>  %s", s_currentTaskRecord->id_, msg );

				handled = true;
			}
			break;

		case DebugMessagePriority::MESSAGE_PRIORITY_WARNING:
			if (s_currentTaskRecord != NULL)
			{
				s_currentTaskRecord->hasWarning_ = true;

				char reformat[2048];
				sprintf( reformat, "WARNING: %s", pFormat );
				size_t logSize = s_currentTaskRecord->log_.size();
				size_t msgSize = _vscprintf( reformat, argPtr );
				s_currentTaskRecord->log_.resize( logSize + msgSize );
				char * msg = const_cast< char * >( s_currentTaskRecord->log_.c_str() + logSize );
				MF_VERIFY( _vsnprintf( msg, msgSize, reformat, argPtr ) >= 0 );
				fprintf( stdout, "%d>  %s", s_currentTaskRecord->id_, msg );

				handled = true;
			}
			break;
		}
	}
	else if (pCategory != NULL && strcmp( pCategory, ASSET_PIPELINE_CATEGORY ) == 0)
	{
		char reformat[2048];
		s_currentTaskRecord == NULL ?
			sprintf( reformat, pFormat[0] == '=' ? "%s" : "  %s", pFormat ) :
			sprintf( reformat, pFormat[0] == '-' ? "%d>%s" : "%d>  %s", s_currentTaskRecord->id_, pFormat );

		vfprintf( stdout, reformat, argPtr );

		handled = true;
	}

	return handled;
}

// =============================================================================

class BatchCompilerOptions : public AssetCompilerOptions
{
public:
	typedef BW::vector< BW::string > Paths;

	BatchCompilerOptions()
		: AssetCompilerOptions()
		, reportPath_()
	{
	}

	virtual bool parseCommandLine( const BW::string & commandString ) override;

	void resolveInputPaths();

	bool generateReport() const
	{
		return !reportPath_.empty();
	}

	const BW::string & getReportPath() const
	{
		return reportPath_;
	}

	const Paths & getInputPaths() const
	{
		return inputPaths_;
	}

private:

	bool parseInputPaths( const CommandLine & commandLine );

	Paths parsedPaths_;
	Paths inputPaths_;

	BW::string reportPath_;
};

bool BatchCompilerOptions::parseCommandLine( const BW::string & commandString )
{
	CommandLine	commandLine( commandString.c_str() );

	if (!parseInputPaths( commandLine ))
	{
		ERROR_MSG("ERROR: No valid inputs\n");
		return false;
	}

	if (commandLine.hasParam( "report" ))
	{
		BW::string report = commandLine.getParam( "report" );
		reportPath_ = AssetCompiler::sanitisePathParam( BWResource::getFilePath( report ).c_str() )
			+ BWResource::getFilename( report );
		BWResource::ensureAbsolutePathExists( reportPath_ );
	}

	return AssetCompilerOptions::parseCommandLine( commandString );
}

bool BatchCompilerOptions::parseInputPaths( const CommandLine & commandLine )
{
	MultiFileSystem* fs = BWResource::instance().fileSystem();
	bool hasInput = false;

	// Parse parameter list for input candidates and add them to the list
	for (uint i = 1; i < commandLine.size(); ++i)
	{
		const char * input = commandLine.getParamByIndex( i );
		if (strlen( input ) == 0)
		{
			continue;
		}
		if (CommandLine::hasSwitch(input))
		{
			++i;
			continue;
		}

		hasInput = true;
		BW::string inputPath = input;
		BWResource::resolveToAbsolutePath( inputPath );
		inputPath = AssetCompiler::sanitisePathParam( inputPath.c_str() );

		auto ft = fs->getFileType( inputPath );
		if (ft != IFileSystem::FT_FILE &&
			ft != IFileSystem::FT_DIRECTORY)
		{
			WARNING_MSG("Invalid input: %s\n", input);
			continue;
		}

		parsedPaths_.push_back( inputPath );
	}

	return hasInput != parsedPaths_.empty();
}

void BatchCompilerOptions::resolveInputPaths()
{
	MultiFileSystem* fs = BWResource::instance().fileSystem();

	// If no inputs were specified then use the base resource paths
	if (parsedPaths_.empty())
	{
		const int numPaths = BWResource::getPathNum();
		for (int i = 0; i < numPaths; ++i)
		{
			BW::string inputPath = AssetCompiler::sanitisePathParam( BWResource::getPath( i ).c_str() );
			parsedPaths_.push_back( inputPath );
		}
	}

	// Prune subpaths from the inputs
	inputPaths_.clear();
	std::copy_if(std::begin(parsedPaths_), std::end(parsedPaths_), std::back_inserter(inputPaths_),
		[this, fs]( const BW::string & inputPath ) -> bool
	{
		// Always include files
		if (fs->getFileType( inputPath ) == IFileSystem::FT_FILE)
		{
			return true;
		}

		const bool subPath = std::any_of(std::begin(parsedPaths_), std::end(parsedPaths_), 
			[&inputPath](const BW::string & path) -> bool
		{
			return path.compare(0, path.length(), inputPath) == 0 && inputPath.length() != path.length();
		});

		return !subPath;
	});
}

// =============================================================================

int bw_main( int argc, char* argv[] )
{
	BW_GUARD;

	SetConsoleCtrlHandler( BatchCompiler_Locals::ConsoleHandler, TRUE );

	BW_SYSTEMSTAGE_MAIN();
#ifdef ENABLE_MEMTRACKER
	MemTracker::instance().setCrashOnLeak( true );
#endif

	// Initialise the file systems
	bool bInitRes = BWResource::init( BW::BWResource::appDirectory(), false );

	if ( !AutoConfig::configureAllFrom( "resources.xml" ) )
	{
		ERROR_MSG("Couldn't load auto-config strings from resource.xml\n" );
	}

	int retval = 0;

	BatchCompilerOptions options;
	if (options.parseCommandLine( bw_wtoutf8( GetCommandLine() ) ) )
	{
		BatchCompiler bc;
        BatchCompiler_Locals::batchCompiler_ = &bc;
		options.resolveInputPaths();
		options.apply( bc );
		bc.initPlugins();
		bc.initCompiler();
		bc.build( options.getInputPaths() );
		if (options.generateReport())
		{
			bc.outputReport( options.getReportPath() );
		}
		bc.finiCompiler();
		bc.finiPlugins();
        BatchCompiler_Locals::batchCompiler_ = NULL;

		retval = bc.getFailedCount();
	}

	// Cleanup the file systems
	BWResource::fini();

	// DataSectionCensus is created on first use, so delete at end of App
	DataSectionCensus::fini();

	return retval;
}

BW_END_NAMESPACE

int main( int argc, char* argv[] )
{
	return BW_NAMESPACE bw_main( argc, argv );
}
