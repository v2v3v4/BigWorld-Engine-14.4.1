#ifndef WATCHER_PROTOCOL_HPP
#define WATCHER_PROTOCOL_HPP

#include "cstdmf/watcher.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;

/**
 * This class implements a basic set of decoding functionality for
 * Watcher protocol v2.
 */
class WatcherProtocolDecoder
{
public:
	virtual bool decode( BinaryIStream & stream );
	virtual bool decodeNext( BinaryIStream & stream );

	virtual int readSize( BinaryIStream & stream );

	virtual bool defaultHandler( BinaryIStream & stream, Watcher::Mode mode );
	virtual bool intHandler( BinaryIStream & stream, Watcher::Mode mode );
	virtual bool uintHandler( BinaryIStream & stream, Watcher::Mode mode );
	virtual bool floatHandler( BinaryIStream & stream, Watcher::Mode mode );
	virtual bool boolHandler( BinaryIStream & stream, Watcher::Mode mode );
	virtual bool stringHandler( BinaryIStream & stream, Watcher::Mode mode );
	virtual bool tupleHandler( BinaryIStream & stream, Watcher::Mode mode );
};

BW_END_NAMESPACE

#endif
