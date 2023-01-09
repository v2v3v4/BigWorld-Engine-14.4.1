#include "pch.hpp"

#include "basictypes.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/watcher.hpp"

#ifdef _WIN32
#ifndef _XBOX360
#include <Winsock2.h>
#endif
#elif defined( PLAYSTATION3 )
#include <netinet/in.h>
#else
#include <arpa/inet.h>
#endif

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Direction3D
// -----------------------------------------------------------------------------

/**
 *  Output streaming for directions.
 */
BinaryOStream& operator<<( BinaryOStream &out, const Direction3D &d )
{
	return out << d.roll << d.pitch << d.yaw;
}


/**
 *  Input streaming for directions.
 */
BinaryIStream& operator>>( BinaryIStream &in, Direction3D &d )
{
	return in >> d.roll >> d.pitch >> d.yaw;
}


/**
 *	This function implements the output streaming operator for Direction3D.
 */
std::ostream& operator <<( std::ostream & stream, const Direction3D & t )
{
	char buf[128];
	bw_snprintf( buf, sizeof(buf),
			"(%1.1f, %1.1f, %1.1f)", t.roll, t.pitch, t.yaw );
	stream << buf;

    return stream;
}


/**
 *	This function implements the input streaming operator for Direction3D.
 */
std::istream& operator >>( std::istream & stream, Direction3D & t )
{
	char dummy;
    stream >> dummy >> t.roll >> dummy >> t.pitch >> dummy >> t.yaw >>  dummy;

    return stream;
}


// -----------------------------------------------------------------------------
// Section: Address
// -----------------------------------------------------------------------------

namespace Mercury
{

char Address::s_stringBuf[ 2 ][ Address::MAX_STRLEN ];
int Address::s_currStringBuf = 0;
const Address Address::NONE( 0, 0 );

/**
 *	This method write the IP address to the input string.
 */
int Address::writeToString( char * str, int length ) const
{
	uint32	hip = ntohl( ip );
	uint16	hport = ntohs( port );

	return bw_snprintf( str, length,
		"%d.%d.%d.%d:%d",
		(int)(uchar)(hip>>24),
		(int)(uchar)(hip>>16),
		(int)(uchar)(hip>>8),
		(int)(uchar)(hip),
		(int)hport );
}


/**
 *	This operator returns the address as a string.
 *	Note that it uses a static string, so the address is only valid until
 *	the next time this operator is called. Use with care when dealing
 *	with multiple threads.
 */
char * Address::c_str() const
{
	char * buf = Address::nextStringBuf();
	this->writeToString( buf, MAX_STRLEN );
    return buf;
}


/**
 *	This operator returns the address as a string excluding the port.
 *	Note that it uses a static string, so the address is only valid until
 *	the next time an address is converted to a string. Use with care when
 *	dealing with multiple threads.
 */
const char * Address::ipAsString() const
{
	uint32	hip = ntohl( ip );
	char * buf = Address::nextStringBuf();

	bw_snprintf( buf, MAX_STRLEN, "%d.%d.%d.%d",
		(int)(uchar)(hip>>24),
		(int)(uchar)(hip>>16),
		(int)(uchar)(hip>>8),
		(int)(uchar)(hip) );

    return buf;
}


#if ENABLE_WATCHERS
/**
 * 	This method returns a watcher for this address.
 */
Watcher & Address::watcher()
{
	// TODO: This is deprecated. The above should be used instead.
	static MemberWatcher<char *,Address>	* watchMe = NULL;

	if (watchMe == NULL)
	{
		watchMe = new MemberWatcher<char *,Address>(
			*((Address*)NULL),
			&Address::c_str,
			static_cast< void (Address::*)( char* ) >( NULL )
			);
	}

	return *watchMe;
}


/**
 *	This function converts a watcher string to an address.
 */
bool watcherStringToValue( const char * valueStr, Address & value )
{
	int a1, a2, a3, a4, a5;

	if (sscanf( valueStr, "%d.%d.%d.%d:%d",
				&a1, &a2, &a3, &a4, &a5 ) != 5)
	{
		WARNING_MSG( "watcherStringToValue: "
				"Cannot convert '%s' to an Address.\n", valueStr );
		return false;
	}

	value.ip = (a1 << 24)|(a2 << 16)|(a3 << 8)|a4;

	value.port = uint16(a5);
	value.port = ntohs( value.port );
	value.ip = ntohl( value.ip );

	return true;
}


/**
 *	This function converts an address to a watcher string.
 */
BW::string watcherValueToString( const Address & value )
{
	return value.c_str();
}


#endif


/**
 *  Output streaming for addresses.  Note that we don't use the streaming
 *  operators because they will do endian conversions on big endian systems.
 *  These values need to be in the same byte order on both systems so we just
 *  use the raw methods.
 */
BinaryOStream& operator<<( BinaryOStream &os, const Address &a )
{
	os.insertRaw( a.ip );
	os.insertRaw( a.port );
	os << a.salt;

	return os;
}


/**
 *  Input streaming for addresses.
 */
BinaryIStream& operator>>( BinaryIStream &is, Address &a )
{
	is.extractRaw( a.ip );
	is.extractRaw( a.port );
	is >> a.salt;

	return is;
}


/**
 *  This method returns the next buffer to be used for making string
 *  representations of addresses.  It just flips between the two available
 *  buffers.
 */
char * Address::nextStringBuf()
{
	s_currStringBuf = (s_currStringBuf + 1) % 2;
	return s_stringBuf[ s_currStringBuf ];
}

} // namespace Mercury


#if defined( MF_SERVER )
const char * EntityMailBoxRef::componentAsStr( Component component )
{
	switch (component)
	{
		case EntityMailBoxRef::CELL:	return "cell";
		case EntityMailBoxRef::BASE:	return "base";
		case EntityMailBoxRef::SERVICE:	return "service";
		case EntityMailBoxRef::CLIENT:	return "client";
		case EntityMailBoxRef::BASE_VIA_CELL:	return "base_via_cell";
		case EntityMailBoxRef::CLIENT_VIA_CELL:	return "client_via_cell";
		case EntityMailBoxRef::CELL_VIA_BASE:	return "cell_via_base";
		case EntityMailBoxRef::CLIENT_VIA_BASE:	return "client_via_base";
	}

	return "<invalid>";
}
#endif

BW_END_NAMESPACE


// basictypes.cpp

