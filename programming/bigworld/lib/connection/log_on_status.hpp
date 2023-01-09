#ifndef LOG_ON_STATUS_HPP
#define LOG_ON_STATUS_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class LogOnStatus
{
public:

    /// This enumeration contains the possible results of a logon attempt.  If
    /// you update this mapping, you need to make corresponding changes to
    /// bigworld/src/client/connection_control.cpp.
    enum Status
    {
        // client status values
        NOT_SET,
        LOGGED_ON,
        LOGGED_ON_OFFLINE,
        CONNECTION_FAILED,
        DNS_LOOKUP_FAILED,
        UNKNOWN_ERROR,
        CANCELLED,
        ALREADY_ONLINE_LOCALLY,
        PUBLIC_KEY_LOOKUP_FAILED,
        LAST_CLIENT_SIDE_VALUE = 63,

        // server status values
        LOGIN_MALFORMED_REQUEST,
        LOGIN_BAD_PROTOCOL_VERSION,

        LOGIN_CHALLENGE_ISSUED,

        LOGIN_REJECTED_NO_SUCH_USER,
        LOGIN_REJECTED_INVALID_PASSWORD,
        LOGIN_REJECTED_ALREADY_LOGGED_IN,
        LOGIN_REJECTED_BAD_DIGEST,
        LOGIN_REJECTED_DB_GENERAL_FAILURE,
        LOGIN_REJECTED_DB_NOT_READY,
        LOGIN_REJECTED_ILLEGAL_CHARACTERS,
        LOGIN_REJECTED_SERVER_NOT_READY,
        LOGIN_REJECTED_UPDATER_NOT_READY,   // No longer used
        LOGIN_REJECTED_NO_BASEAPPS,
        LOGIN_REJECTED_BASEAPP_OVERLOAD,
        LOGIN_REJECTED_CELLAPP_OVERLOAD,
        LOGIN_REJECTED_BASEAPP_TIMEOUT,
        LOGIN_REJECTED_BASEAPPMGR_TIMEOUT,
        LOGIN_REJECTED_DBAPP_OVERLOAD,
        LOGIN_REJECTED_LOGINS_NOT_ALLOWED,
        LOGIN_REJECTED_RATE_LIMITED,
        LOGIN_REJECTED_BAN,

        LOGIN_REJECTED_CHALLENGE_ERROR,

        LOGIN_REJECTED_AUTH_SERVICE_NO_SUCH_ACCOUNT,
        LOGIN_REJECTED_AUTH_SERVICE_LOGIN_DISALLOWED,
        LOGIN_REJECTED_AUTH_SERVICE_UNREACHABLE,
        LOGIN_REJECTED_AUTH_SERVICE_INVALID_RESPONSE,
        LOGIN_REJECTED_AUTH_SERVICE_GENERAL_FAILURE,

        // Generated client-side
        LOGIN_REJECTED_NO_LOGINAPP,
        LOGIN_REJECTED_NO_LOGINAPP_RESPONSE,
        LOGIN_REJECTED_NO_BASEAPP_RESPONSE,

        LOGIN_REJECTED_IP_ADDRESS_BAN = 244, // Logins from this IP are banned till specified time.
        LOGIN_REJECTED_INACCESSIBLE_REALM = 245, // Logins on specified realms are disabled.
        LOGIN_REJECTED_REGISTRATION_NOT_ALLOWED = 246, // Registration in project is not allowed.
        LOGIN_REJECTED_REGISTRATION_NOT_CONFIRMED = 247, // Email not confirmed
        LOGIN_REJECTED_NOT_REGISTERED = 248,// Account not registered
        LOGIN_REJECTED_ACTIVATING = 249, // Registration not completed
        LOGIN_REJECTED_UNABLE_TO_PARSE_JSON = 250, // Unable to parse JSON.
        LOGIN_REJECTED_USERS_LIMIT = 251,   // Online users limit is reached.
        LOGIN_REJECTED_LOGIN_QUEUE = 252,   // User is in the login queue.

        LOGIN_CUSTOM_DEFINED_ERROR = 254,
        LAST_SERVER_SIDE_VALUE = 255
    };

    /// This is the default constructor.
    LogOnStatus( Status status = NOT_SET ) :
        status_( status )
    {
    }

    /// This method returns true if the logon succeeded.
    virtual bool succeeded() const
    {
        return status_ == LOGGED_ON;
    }

    /// This method returns true if the logon failed.
    virtual bool fatal() const
    {
        return
            status_ == CONNECTION_FAILED ||
            status_ == CANCELLED ||
            status_ == UNKNOWN_ERROR;
    }

    /// This method returns true if the logon was successful, or is still
    /// pending.
    virtual bool okay() const
    {
        return
            status_ == NOT_SET ||
            status_ == LOGGED_ON;
    }

    Status value() const { return status_; }

    void value( Status status ) { status_ = status; }

private:
    Status status_;
};


/*
 *  Overloaded equals operator for comparing LogOnStatus'
 */
inline bool operator==( LogOnStatus a, LogOnStatus b )
{
    return a.value() == b.value();
}


/**
 *  Overloaded not-equals operator for comparing LogOnStatus'
 */
inline bool operator!=( LogOnStatus a, LogOnStatus b )
{
    return a.value() != b.value();
}

BW_END_NAMESPACE

#endif // LOG_ON_STATUS_HPP
