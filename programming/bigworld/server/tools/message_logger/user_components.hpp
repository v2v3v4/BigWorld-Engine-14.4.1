#ifndef USER_COMPONENTS_HPP
#define USER_COMPONENTS_HPP

#include "binary_file_handler.hpp"
#include "types.hpp"
#include "mldb/log_component_names.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class LoggingComponent;
class LoggerComponentMessage;

class UserComponentVisitor
{
public:
	virtual bool onComponent( const LoggingComponent &component ) = 0;
};


/**
 * The registry of processes within each UserLog.  Contains static info
 * about each process such as name, pid, etc.
 */
class UserComponents : public BinaryFileHandler
{
public:
	UserComponents();
	virtual ~UserComponents();

	bool init( const char *root, const char *mode );

	virtual bool read();
	virtual void flush();
	bool write( LoggingComponent *logComponent );

	// Candidate for cleanup. Only used by the reader
	const LoggingComponent * getComponentByID(
		MessageLogger::UserComponentID componentID ) const;

	// Candidate for cleanup. Only used by the writer.
	LoggingComponent *getComponentByAddr( const Mercury::Address & addr );

	// Candidate for cleanup. Only used by writer.
	LoggingComponent * getComponentFromMessage(
								const LoggerComponentMessage & msg,
								const Mercury::Address & addr,
								LogComponentNamesMLDB & logComponentNames );

	bool erase( const Mercury::Address &addr );

	const char *getFilename() const;

	MessageLogger::UserComponentID getNextUserComponentID();

	bool visitAllWith( UserComponentVisitor &visitor ) const;

private:
	BW::string filename_;

	typedef BW::map< Mercury::Address, LoggingComponent* > AddrMap;
	AddrMap addrMap_;

	typedef BW::map< MessageLogger::UserComponentID, LoggingComponent* > IDMap;
	IDMap idMap_;

	MessageLogger::UserComponentID idTicker_;
};

BW_END_NAMESPACE

#endif // USER_COMPONENTS_HPP
