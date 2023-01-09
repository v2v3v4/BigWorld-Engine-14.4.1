#ifndef ACTIVE_FILES_HPP
#define ACTIVE_FILES_HPP

#include "text_file_handler.hpp"
#include "user_log_writer.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class represents a file that tracks the entries and args files that
 * are currently being written to.  mltar.py respects this list in --move mode
 * and doesn't touch the files in it.
 */
class ActiveFiles : public TextFileHandler
{
public:
	bool init( const BW::string &logPath, UserLogs *pUserLogs );

	virtual bool read();
	virtual bool handleLine( const char *line );

	bool update();

	bool deleteFile();

private:
	// The absolute path to the log directory the active_files file is in.
	BW::string logPath_;

	// A list of UserLogs that should be used when updating the active_files
	UserLogs *pUserLogs_;
};

BW_END_NAMESPACE

#endif // ACTIVE_FILES_HPP
