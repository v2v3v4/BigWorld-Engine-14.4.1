#ifndef LOG_COMPONENT_NAMES_MLDB_HPP
#define LOG_COMPONENT_NAMES_MLDB_HPP

#include "../log_component_names.hpp"
#include "../text_file_handler.hpp"
#include "../types.hpp"


BW_BEGIN_NAMESPACE


/**
 * Handles the mapping between ids and component names, i.e. 0 -> cellapp,
 * 1 -> baseapp etc.  This is shared amongst all users, and is based on the
 * order in which messages from unique components are delivered.
 */
class LogComponentNamesMLDB : public TextFileHandler, public LogComponentNames
{
public:
	bool init( const char *logLocation, const char *mode );

protected:
	virtual void flush();
	virtual bool handleLine( const char *line );

	bool writeComponentNameToDB( const BW::string & componentName );

};

BW_END_NAMESPACE

#endif // LOG_COMPONENT_NAMES_MLDB_HPP
