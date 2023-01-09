#include "watcher_protocol.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/watcher.hpp"


BW_BEGIN_NAMESPACE

bool WatcherProtocolDecoder::decode( BinaryIStream & stream )
{

	bool status = true;

	while (status && stream.remainingLength())
	{
		status = this->decodeNext(stream);
	}

	return status;
}


bool WatcherProtocolDecoder::decodeNext( BinaryIStream & stream )
{
	bool status = false;

	uchar type;
	uchar mode;

	stream >> type;
	if (stream.error())
	{
		ERROR_MSG( "WatcherProtocolDecoder::decodeNext: Failed to read type "
					"from stream. Unable to continue.\n" );
		return false;
	}

	stream >> mode;
	if (stream.error())
	{
		ERROR_MSG( "WatcherProtocolDecoder::decodeNext: Failed to read mode "
					"from stream. Unable to continue.\n" );
		return false;
	}

	switch ((WatcherDataType)type)
	{
	case WATCHER_TYPE_INT:
		status = this->intHandler( stream, (Watcher::Mode)mode );
		break;

	case WATCHER_TYPE_UINT:
		status = this->uintHandler( stream, (Watcher::Mode)mode );
		break;

	case WATCHER_TYPE_FLOAT:
		status = this->floatHandler( stream, (Watcher::Mode)mode );
		break;

	case WATCHER_TYPE_BOOL:
		status = this->boolHandler( stream, (Watcher::Mode)mode );
		break;

	case WATCHER_TYPE_STRING:
		status = this->stringHandler( stream, (Watcher::Mode)mode );
		break;

	case WATCHER_TYPE_TUPLE:
		status = this->tupleHandler( stream, (Watcher::Mode)mode );
		break;


	default:
		status = this->defaultHandler( stream, (Watcher::Mode)mode );
		break;
	}

	return status;
}


int WatcherProtocolDecoder::readSize( BinaryIStream & stream )
{
	return stream.readStringLength();
}


bool WatcherProtocolDecoder::defaultHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	int size = this->readSize( stream );
	if (stream.error())
	{
		ERROR_MSG( "WatcherProtocolDecoder::defaultHandler: Failed to read "
					"the size of current data block to de-stream. Unable to "
					"continue.\n" );
		return false;
	}

	if (size > stream.remainingLength())
	{
		ERROR_MSG( "WatcherProtocolDecoder::defaultHandler: Requested size "
					"of data block to de-stream > remaining stream size "
					"(%d > %d). Unable to continue.\n", size,
					stream.remainingLength() );
		return false;
	}

	stream.retrieve( size );
	if (stream.error())
	{
		ERROR_MSG( "WatcherProtocolDecoder::defaultHandler: Failed to "
					"de-stream %d bytes as requested.\n", size );
		return false;
	}

	return true;
}


bool WatcherProtocolDecoder::intHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	return this->defaultHandler( stream, mode );
}


bool WatcherProtocolDecoder::uintHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	return this->defaultHandler( stream, mode );
}


bool WatcherProtocolDecoder::floatHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	return this->defaultHandler( stream, mode );
}


bool WatcherProtocolDecoder::boolHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	return this->defaultHandler( stream, mode );
}


bool WatcherProtocolDecoder::stringHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	return this->defaultHandler( stream, mode );
}


bool WatcherProtocolDecoder::tupleHandler( BinaryIStream & stream,
	Watcher::Mode mode )
{
	return this->defaultHandler( stream, mode );
}

BW_END_NAMESPACE

// watcher_protocol.cpp

