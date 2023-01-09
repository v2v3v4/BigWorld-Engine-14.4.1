#ifndef BINARY_FILE_HANDLER_HPP
#define BINARY_FILE_HANDLER_HPP

#include "file_handler.hpp"

#include "network/file_stream.hpp"


BW_BEGIN_NAMESPACE

/**
 * Wraps binary-format files that are accessed via a FileStream.
 */
class BinaryFileHandler : public FileHandler
{
public:
	BinaryFileHandler();
	virtual ~BinaryFileHandler();

	bool init( const char *path, const char *mode );

	virtual long length();

protected:
	FileStream *pFile_;
};

BW_END_NAMESPACE

#endif // BINARY_FILE_HANDLER_HPP
