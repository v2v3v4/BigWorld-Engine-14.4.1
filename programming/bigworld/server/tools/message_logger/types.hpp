#ifndef LOGGING_TYPES_HPP
#define LOGGING_TYPES_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

namespace MessageLogger
{

typedef BW::vector< BW::string > StringList;


//namespace AbstractWriterFormat
//{

/*
 *	AppTypeID represents the ID in the logs that is associated with a BigWorld
 *	server process type such as CellAppMgr,LoginApp, etc...
 */
typedef uint32	AppTypeID;

/*
 *	UserComponentID represents the ID in the user logs that is associated with
 *	a BigWorld server process type, that has been sending logs. It is used to
 *	uniquely refer to a process instance that has been running at some point.
 */
typedef uint32	UserComponentID;

/*
 *	CategoryID represents a category string associated with a log message.
 */
typedef uint16	CategoryID;


/*
 *	IPAddress represents the dotted quad component of an IPv4 address.
 */
typedef uint32 IPAddress;

/*
 *	FormatStringOffsetId represents either a byte offset into the format strings
 *	file or an ID mapped in the formatStrings DB table.
 *
 *	NOTE: To maintain safe backwards compatibility with the MLDB storage method
 *	this should always remain the same type as returned by FileStream::tell
 *	ie. the return type of ftell()
 */
typedef uint32 FormatStringOffsetId;

/*
 * This represents the offset of one log entry's metadata in the metadata file
 *
 */
typedef uint32 MetaDataOffset;

//} // namespace AbstractWriterFormat


//namespace NetworkFormat
//{

/**
 *	This type represents the component's ID (e.g., cellapp01)
 */
typedef ServerAppInstanceID AppInstanceID;

//} // namespace NetworkFormat


/**
 * This type represents a Linux user's uid (e.g. 10025)
 */
typedef uint16 UID;

} // namespace MessageLogger

BW_END_NAMESPACE

#endif // LOGGING_TYPES_HPP
