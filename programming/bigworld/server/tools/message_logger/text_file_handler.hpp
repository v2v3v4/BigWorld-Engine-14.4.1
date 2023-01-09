#ifndef TEXT_FILE_HANDLER_HPP
#define TEXT_FILE_HANDLER_HPP

#include "file_handler.hpp"

#include <stdlib.h>


BW_BEGIN_NAMESPACE

/**
 * Wraps line-based ASCII files.
 */
class TextFileHandler : public FileHandler
{
public:
	TextFileHandler();
	virtual ~TextFileHandler();

	bool init( const char *filename, const char *mode );
	bool close();
	bool isOpened() { return (fp_ != NULL); }
	bool reopenIfChanged();

	virtual long length();

	virtual bool read();
	virtual bool handleLine( const char *line ) = 0;

	bool writeLine( const char *line ) const;

protected:
	FILE *fp_;

private:
	bool reopen();

	enum InodeStatus
	{
		INODE_STATUS_ERROR,
		INODE_STATUS_UNCHANGED,
		INODE_STATUS_CHANGED
	};

	InodeStatus getInodeStatus();
};

BW_END_NAMESPACE

#endif // TEXT_FILE_HANDLER_HPP
