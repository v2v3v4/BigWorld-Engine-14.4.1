#include "user_segment_writer.hpp"

#include "constants.hpp"
#include "log_string_writer.hpp"
#include "metadata.hpp"

#include "mldb/log_storage.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/log_meta_data.hpp"

BW_BEGIN_NAMESPACE


UserSegmentWriter::UserSegmentWriter( const BW::string userLogPath,
	const char * suffix ) :
	UserSegment( userLogPath, suffix )
{ }


bool UserSegmentWriter::init()
{
	char buf[ 1024 ];
	static const char *mode = "a+";

	bw_snprintf( buf, sizeof( buf ), "%s/entries.%s",
		userLogPath_.c_str(), suffix_.c_str() );

	pEntries_ = new FileStream( buf, mode );
	if (!pEntries_->good())
	{
		ERROR_MSG( "UserSegmentWriter::init: "
			"Couldn't open entries file %s for writing: %s\n",
			buf, pEntries_->strerror() );
		isGood_ = false;
		return false;
	}

	// Generate args filename
	bw_snprintf( buf, sizeof( buf ), "%s/args.%s",
		userLogPath_.c_str(), suffix_.c_str() );
	pArgs_ = new FileStream( buf, mode );
	if (!pArgs_->good())
	{
		ERROR_MSG( "UserSegmentWriter::init: "
			"Couldn't open args file %s for writing: %s\n",
			buf, pArgs_->strerror() );
		isGood_ = false;
		return false;
	}

	this->updateEntryBounds();

	pMetadataMLDB_ = new MetadataMLDB();
	if (!pMetadataMLDB_->init( userLogPath_.c_str(), suffix_.c_str(), mode ))
	{
		ERROR_MSG( "UserSegmentWriter::init: "
			"Unable to initialise metadata.\n" );
		return false;
	}

	return true;
}


/**
 *  Add an entry to this segment.
 */
bool UserSegmentWriter::addEntry( LoggingComponent * pComponent,
	UserLogWriter * pUserLog, LogEntry & entry,
	LogStringInterpolator & handler, MemoryIStream & inputStream,
	MessageLogger::NetworkVersion version )
{
	entry.argsOffset_ = pArgs_->length();

	LogStringWriter parser( *pArgs_ );

	if (!handler.streamToLog( parser, inputStream, version ))
	{
		ERROR_MSG( "UserSegmentWriter::addEntry: "
			"Error whilst destreaming args from %s\n",
			pComponent->getString().c_str() );
		return false;
	}

	argsSize_ = pArgs_->length();
	entry.argsLen_ = argsSize_ - entry.argsOffset_;

	entry.metadataOffset_ = pMetadataMLDB_->length();

	// inputStream may have 1 byte remaining for NULL.
	if (version >= LOG_METADATA_VERSION && inputStream.remainingLength() > 1)
	{
		BW::string metadata;
		if (version >= LOG_METADATA_STREAM_VERSION)
		{
			MLMetadata::LogMetadataBuilder builder;
			MLMetadata::MLDBMetadataStream outputStream;
			builder.process( inputStream, outputStream );
			metadata = outputStream.getJSON();
			inputStream.finish();
			if (metadata.length())
			{
				pMetadataMLDB_->writeMetadataToMLDB( metadata );
			}
		}
		else
		{
			// Stream all the remaining data into a metadata buffer
			inputStream >> metadata;

			if (inputStream.error())
			{
				ERROR_MSG( "UserSegmentWriter::addEntry: "
					"Error whilst destreaming metadata from %s.\n",
					pComponent->getString().c_str() );
				return false;
			}

			pMetadataMLDB_->writeMetadataToMLDB( metadata );
		}
	}

	entry.metadataLen_ = pMetadataMLDB_->length() - entry.metadataOffset_;

	// If this is the component's first log entry, we need to write the
	// component to disk as well.
	if (!pComponent->written())
	{
		pComponent->updateFirstEntry( suffix_, numEntries_ );

 		pUserLog->updateComponent( pComponent );
				//components_.write( component );

		if (!pComponent->written())
		{
			ERROR_MSG( "UserSegment::addEntry: "
				"Failed to write %s to user components file.\n",
				pComponent->getString().c_str() );
		}
	}

	*pEntries_ << entry;
	pEntries_->commit();

	if (numEntries_ == 0)
	{
		start_ = entry.time_;
	}
	end_ = entry.time_;
	numEntries_++;

	return true;
}


/**
 * This method returns whether the current segment has been completely filled.
 *
 * @returns true if segment is full, false if not full.
 */
bool UserSegmentWriter::isFull( const LogStorageMLDB * pLogStorage ) const
{
	int currSegmentSize = int( numEntries_ * sizeof( LogEntry ) ) + argsSize_;

	return (currSegmentSize >= pLogStorage->getMaxSegmentSize());
}

BW_END_NAMESPACE

// user_segment_writer.cpp
