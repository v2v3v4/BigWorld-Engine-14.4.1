#ifndef UNARY_INTEGER_FILE_HPP
#define UNARY_INTEGER_FILE_HPP

#include "text_file_handler.hpp"

#include <stdlib.h>


BW_BEGIN_NAMESPACE

/**
 * A file containing a single number represented in ascii.  The version file
 * and the uid file in each user log directory use this.
 */
class UnaryIntegerFile: public TextFileHandler
{
public:
	UnaryIntegerFile();

	using TextFileHandler::init;
	bool init( const char *path, const char *mode, int v );
	virtual bool handleLine( const char *line );
	virtual void flush();
	bool set( int v );

	int getValue() const;

	bool deleteFile();

protected:
	int v_;
};

BW_END_NAMESPACE

#endif // UNARY_INTEGER_FILE_HPP
