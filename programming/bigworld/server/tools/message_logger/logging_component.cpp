#include "logging_component.hpp"

#include "user_log.hpp"
#include "user_components.hpp"


BW_BEGIN_NAMESPACE

LoggingComponent::LoggingComponent( UserComponents *userComponents ) :
	userComponentsFilename_( userComponents->filename() )
{ }


LoggingComponent::LoggingComponent( UserComponents * userComponents,
		const Mercury::Address & addr, const LoggerComponentMessage & msg,
		MessageLogger::AppTypeID appTypeID ) :
	msg_( msg ),
	addr_( addr ),
	appTypeID_( appTypeID ),
	userComponentID_( userComponents->getNextUserComponentID() ),
	appInstanceID_( 0 ),
	fileOffset_( -1 ),
	userComponentsFilename_( userComponents->filename() )
{ }


/**
 *
 */
void LoggingComponent::updateFirstEntry( const BW::string & suffix,
											const int numEntries )
{
	firstEntry_ = LogEntryAddress( suffix, numEntries );
}


/**
 *
 */
bool LoggingComponent::isSameComponentAs(
	const LoggerComponentMessage & otherMessage ) const
{
	return ((msg_.version_ == otherMessage.version_) &&
			(msg_.uid_ == otherMessage.uid_) &&
			(msg_.pid_ == otherMessage.pid_) &&
			(msg_.componentName_ == otherMessage.componentName_));
}


/**
 *  This method is not const as you'd probably expect because it needs to set
 *  this object's fileOffset_ field prior to writing.
 */
void LoggingComponent::write( FileStream &os )
{
	fileOffset_ = os.tell();
	os << addr_ << userComponentID_ << appInstanceID_ << appTypeID_;
	msg_.write( os );
	firstEntry_.write( os );
	os.commit();
}


/**
 *
 */
void LoggingComponent::read( FileStream &is )
{
	fileOffset_ = is.tell();
	is >> addr_ >> userComponentID_ >> appInstanceID_ >> appTypeID_;
	msg_.read( is );
	firstEntry_.read( is );
}


/**
 *
 */
bool LoggingComponent::written() const
{
	return fileOffset_ != -1;
}


/**
 *
 */
bool LoggingComponent::setAppInstanceID( ServerAppInstanceID appInstanceID )
{
	appInstanceID_ = appInstanceID;

	// If this component has already been written to disk (which is almost
	// certain) then we need to overwrite the appid field in the components file
	if (this->written())
	{
		FileStream file( userComponentsFilename_.c_str(), "r+" );

		// Seek to the exact offset of the app id
		file.seek( fileOffset_ );
		file >> addr_ >> userComponentID_;

		// Overwrite old ID
		file << appInstanceID_;
		file.commit();
	}

	return true;
}


/**
 *	This method returns the component app instance ID.
 */
MessageLogger::AppInstanceID LoggingComponent::getAppInstanceID() const
{
	return appInstanceID_;
}


/**
 *	This method returns the log-global BigWorld server component ID as is
 *	generated from the top level 'component_names' file.
 */
MessageLogger::AppTypeID LoggingComponent::getAppTypeID() const
{
	return appTypeID_;
}


/**
 *	This method returns the ID of the BigWorld server component that has
 *	generated logs in a specific user's log directory.
 */
MessageLogger::UserComponentID LoggingComponent::getUserComponentID() const
{
	return userComponentID_;
}


const Mercury::Address & LoggingComponent::getAddress() const
{
	return addr_;
}


/**
 *
 */
const LogEntryAddress & LoggingComponent::getFirstEntry() const
{
	return firstEntry_;
}


BW::string LoggingComponent::getString() const
{
	char buf[ 256 ];
	bw_snprintf( buf, sizeof( buf ), "%s%02d (id:%d) %s",
		msg_.componentName_.c_str(), appInstanceID_, userComponentID_,
		addr_.c_str() );
	return BW::string( buf );
}


/**
 *
 */
int LoggingComponent::getPID() const
{
	return msg_.pid_;
}


/**
 *
 */
const BW::string & LoggingComponent::getComponentName() const
{
	return msg_.componentName_;
}

BW_END_NAMESPACE

// logging_component.cpp
