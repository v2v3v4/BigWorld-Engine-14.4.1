#include "database_reply_handler.hpp"

#include "loginapp.hpp"

#include "connection/log_on_status.hpp"

#include "cstdmf/debug.hpp"
#include "network/nub_exception.hpp"
#include "server/nat_config.hpp"


BW_BEGIN_NAMESPACE

/**
 *  Constructor.
 */
DatabaseReplyHandler::DatabaseReplyHandler(
        LoginApp & loginApp,
        const Mercury::Address & clientAddr,
        Mercury::Channel * pChannel,
        Mercury::ReplyID replyID,
        LogOnParamsPtr pParams ) :
    loginApp_( loginApp ),
    clientAddr_( clientAddr ),
    pChannel_( pChannel ),
    replyID_( replyID ),
    pParams_( pParams )
{
}


/**
 *  This method is called when a message comes back from the system.
 *  It deletes itself at the end.
 */
void DatabaseReplyHandler::handleMessage(
    const Mercury::Address & /*source*/,
    Mercury::UnpackedMessageHeader & header,
    BinaryIStream & data,
    void * /*arg*/ )
{
    uint8 status;
    data >> status;

    if (status != LogOnStatus::LOGGED_ON)
    {
        if (status == LogOnStatus::LOGIN_REJECTED_IP_ADDRESS_BAN)
        {
            ::time_t timeout = 0;
            {
                BW::string timeoutStr;
                data >> timeoutStr;
                BW::istringstream(timeoutStr) >> timeout;
            }

            if (timeout == 0)
            {
                ERROR_MSG( "DatabaseReplyHandler::handleMessage(): "
                        "Wrong data received along with "
                        "LogOnStatus::LOGIN_REJECTED_IP_ADDRESS_BAN "
                        "client IP address %s\n", clientAddr_.c_str() );
                loginApp_.handleFailure( clientAddr_, pChannel_.get(),
                        replyID_, status );
                delete this;
                return;
            }

            // seconds from now
            timeout = std::max( ::time_t(0), timeout - ::time(NULL) ); 
            if (timeout)
            {
                INFO_MSG( "DatabaseReplyHandler::handleMessage(): "
                        "banning IP address %s for %ld seconds\n",
                        clientAddr_.c_str(), int64(timeout) );
            }
            loginApp_.handleBanIP( clientAddr_, pChannel_.get(), replyID_,
                                    pParams_, timeout );
            delete this;
            return;
        }

        if (data.remainingLength() > 0)
        {
            BW::string msg;
            data >> msg;
            loginApp_.handleFailure( clientAddr_, pChannel_.get(), replyID_,
                status, msg.c_str(), pParams_ );
        }
        else
        {
            loginApp_.handleFailure( clientAddr_, pChannel_.get(), replyID_,
                status, 
                "Database returned an unelaborated error. Check DBApp log.",
                pParams_ );
        }

        LoginApp & app = loginApp_;
        if ((app.systemOverloaded() == 0 &&
                status == LogOnStatus::LOGIN_REJECTED_BASEAPP_OVERLOAD) ||
            status == LogOnStatus::LOGIN_REJECTED_CELLAPP_OVERLOAD ||
            status == LogOnStatus::LOGIN_REJECTED_DBAPP_OVERLOAD)
        {
            DEBUG_MSG( "DatabaseReplyHandler::handleMessage(%s): "
                    "failure due to overload (status=%x)\n",
                clientAddr_.c_str(), status );
            app.systemOverloaded( status );
        }
        delete this;
        return;
    }

    if (data.remainingLength() < int(sizeof( LoginReplyRecord )))
    {
        ERROR_MSG( "DatabaseReplyHandler::handleMessage: "
                        "Login failed. Expected %" PRIzu " bytes got %d\n",
                sizeof( LoginReplyRecord ), data.remainingLength() );

        if (data.remainingLength() == sizeof(LoginReplyRecord) - sizeof(int))
        {
            ERROR_MSG( "DatabaseReplyHandler::handleMessage: "
                    "This can occur if a login is attempted to an entity type "
                    "that is not a Proxy.\n" );

            loginApp_.handleFailure( clientAddr_, pChannel_.get(), replyID_,
                LogOnStatus::LOGIN_CUSTOM_DEFINED_ERROR,
                "Database returned a non-proxy entity type.",
                pParams_ );
        }
        else
        {
            loginApp_.handleFailure( clientAddr_, pChannel_.get(), replyID_,
                LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE,
                "Database returned an unknown error.",
                pParams_ );
        }

        delete this;
        return;
    }

    LoginReplyRecord lrr;
    data >> lrr;

    BW::string serverMsg;

    if (data.remainingLength() > 0)
    {
        data >> serverMsg;
    }

    // If the client has an external address, send them the firewall
    // address instead!

    if (NATConfig::isExternalIP( clientAddr_.ip ))
    {
        INFO_MSG( "DatabaseReplyHandler::handleMessage: "
                "Redirecting external client %s to firewall.\n",
            clientAddr_.c_str() );
        lrr.serverAddr.ip = NATConfig::externalIPFor( lrr.serverAddr.ip );
    }

    loginApp_.sendAndCacheSuccess( clientAddr_, pChannel_.get(),
            replyID_, lrr, serverMsg, pParams_ );

    delete this;
}


/**
 *  This method is called when no message comes back from the system,
 *  or some other error occurs. It deletes itself at the end.
 */
void DatabaseReplyHandler::handleException(
    const Mercury::NubException & ne,
    void * /*arg*/ )
{
    loginApp_.handleFailure( clientAddr_, pChannel_.get(), replyID_,
        LogOnStatus::LOGIN_REJECTED_DBAPP_OVERLOAD, "No reply from DBApp.",
        pParams_ );

    WARNING_MSG( "DatabaseReplyHandler: got an exception (%s)\n",
            Mercury::reasonToString( ne.reason() ) );

    delete this;
}


void DatabaseReplyHandler::handleShuttingDown( const Mercury::NubException & ne,
        void * )
{
    INFO_MSG( "DatabaseReplyHandler::handleShuttingDown: Ignoring %s\n",
        clientAddr_.c_str() );
    delete this;
}

BW_END_NAMESPACE

// database_reply_handler.cpp

