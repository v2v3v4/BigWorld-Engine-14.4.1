#ifndef LOGGING_COMPONENT_HPP
#define LOGGING_COMPONENT_HPP

#include "user_components.hpp"
#include "log_entry_address.hpp"

#include "network/logger_message_forwarder.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class represents a persistent process somewhere that is sending to
 * a log and is responsible for reading and writing users "components" files.
 */
class LoggingComponent
{
public:
	// Candidate for cleanup. only required by the 'reader'
	LoggingComponent( UserComponents *userComponents );

	// Candidate for cleanup. only required by 'writer'
	LoggingComponent( UserComponents *userComponents,
		const Mercury::Address & addr, const LoggerComponentMessage & msg,
		MessageLogger::AppTypeID appTypeID );

	void write( FileStream & out );
	void read( FileStream & in );
	bool written() const;

	bool setAppInstanceID( ServerAppInstanceID appInstanceID );
	MessageLogger::AppInstanceID getAppInstanceID() const;

	MessageLogger::AppTypeID getAppTypeID() const;

	MessageLogger::UserComponentID getUserComponentID() const;

	const Mercury::Address & getAddress() const;

	const LogEntryAddress & getFirstEntry() const;

	BW::string getString() const;

	int getPID() const;
	const BW::string & getComponentName() const;

	void updateFirstEntry( const BW::string & suffix,
									const int numEntries );

	bool isSameComponentAs( const LoggerComponentMessage & otherMessage ) const;

private:

	LoggerComponentMessage msg_;

	Mercury::Address addr_;

	// Process-type ID assigned to cellapp, baseapp etc
	MessageLogger::AppTypeID appTypeID_;

	// on disk unique ID per Component object
	MessageLogger::UserComponentID userComponentID_;

	// ID known as amongst server components, eg. cellapp01
	MessageLogger::AppInstanceID appInstanceID_;

	// The first log entry associated with the logging component.
	LogEntryAddress firstEntry_;

	int fileOffset_;

	BW::string userComponentsFilename_;
};

BW_END_NAMESPACE

#endif // LOGGING_COMPONENT_HPP
