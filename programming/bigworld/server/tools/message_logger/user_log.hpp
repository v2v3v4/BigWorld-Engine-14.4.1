#ifndef USER_LOG_HPP
#define USER_LOG_HPP

#include "logging_component.hpp"
#include "user_components.hpp"
#include "user_segment.hpp"
#include "unary_integer_file.hpp"


BW_BEGIN_NAMESPACE

/**
 * A UserLog manages a single user's section of a log.  It is mainly
 * responsible for managing the monolithic files in the user's directory.
 * The segmented files are managed by UserSegments.
 */
class UserLog : public SafeReferenceCount
{
public:
	UserLog( uint16 uid, const BW::string &username );
	virtual ~UserLog();

	virtual bool init( const BW::string rootPath );

	bool isGood() const;

	uint16 getUID() const;
	BW::string getUsername() const;

	bool hasActiveSegments() const;
	BW::string activeSegmentSuffix() const;

	int maxHostnameLen() const;
	void maxHostnameLen( int len );

protected:
	// The unix UID associated to the user being referenced by this UserLog.
	uint16 uid_;

	// The name of the user being reference by this UserLog.
	BW::string username_;

	// Full path to the UserLog being referenced.
	BW::string path_;

	// Is the UserLog initialised and ready to use.
	bool isGood_;

	// List of UserSegments currently available for the owning user.
	UserSegments userSegments_;

	// List of UserComponents this user has logged.
	UserComponents userComponents_;

	// The file containing the UID associated to this user.
	UnaryIntegerFile uidFile_;

	// Maximum hostname length
	int maxHostnameLen_;
};

BW_END_NAMESPACE

#endif // USER_LOG_HPP
