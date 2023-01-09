#ifndef BASIC_TYPES_HPP
#define BASIC_TYPES_HPP

#ifdef _XBOX360
#include "net_360.hpp"
#endif

/// @todo It would be better not to have any include in the common header file.
#include "cstdmf/stdmf.hpp"

#include "math/vector3.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_hash.hpp"

#include "bwentity/bwentity_api.hpp"

#include "space/forward_declarations.hpp"

BW_BEGIN_NAMESPACE

class Watcher;

// Fwd decl for streaming operators
class BinaryIStream;
class BinaryOStream;

/**
 *	The number of bytes in the header of a UDP packet.
 *
 * 	@ingroup common
 */
const int UDP_OVERHEAD = 28;

/**
 *	The number of bits per byte.
 *
 * 	@ingroup common
 */
const int NETWORK_BITS_PER_BYTE = 8;

/**
 *  Standard localhost address.  Assumes little endian host byte order.
 *
 * 	@ingroup common
 */
const uint32 LOCALHOST = 0x0100007F;

/**
 *  Standard broadcast address.  Assumes little endian host byte order.
 *
 * 	@ingroup common
 */
const uint32 BROADCAST = 0xFFFFFFFF;


/**
 * Watcher Nub packet size.
 *
 * @ingroup common
 */
#ifndef _XBOX
static const int WN_PACKET_SIZE = 0x10000;
#else
static const int WN_PACKET_SIZE = 8192;
#endif

static const int WN_PACKET_SIZE_TCP = 0x1000000;


/**
 *	Minimum recommended socket buffer sizes (in bytes). These values are used
 *	as default values when system variables cannot be read.
 *
 *	@ingroup common
 */
const int MIN_SND_SKT_BUF_SIZE = 1048576;
const int MIN_RCV_SKT_BUF_SIZE = 16777216;


/**
 *	A game timestamp. This is synchronised between all clients and servers,
 *	and is incremented by one every game tick.
 *
 * 	@ingroup common
 */
typedef uint32 GameTime;


/**
 *	A unique identifier representing an entity. An entity's ID is constant
 *	between all its instances on the baseapp, cellapp and client.
 *	EntityIDs are however not persistent and should be assumed to be different
 *	upon each run of the server.
 *
 *	A value of zero is considered a null value as defined by NULL_ENTITY_ID.
 *	The range of IDs greater than 1<<30 is reserved for client only entities.
 *
 *	This type replaces the legacy ObjectID type.
 *
 * 	@ingroup common
 */
typedef int32 EntityID;

/**
 *	The null value for an EntityID meaning that no entity is referenced.
 *
 * 	@ingroup common
 */
const EntityID NULL_ENTITY_ID = 0;

/**
 * This is the beginning of the range of entity IDs
 */
const EntityID FIRST_ENTITY_ID = 1;

/**
 * This is the first local entity ID. Anything that's greater or
 * equal to it is local. Anything below is a server-generated ID.
 * 0x7F000000 leaves ~2.1 bn unique IDs for server and 16 m unique
 * client-only IDs
 */
const EntityID FIRST_LOCAL_ENTITY_ID = 0x7F000000;


// Deprecated. Use NULL_ENTITY_ID instead.
const EntityID NULL_ENTITY = NULL_ENTITY_ID;




/**
 *	A unique ID representing the instance ID of a specific component type.
 *
 *	@ingroup common
 */
typedef int32 ServerAppInstanceID;

/**
 *	A unique ID representing a cell application.
 *
 * 	@ingroup common
 */
typedef ServerAppInstanceID CellAppID;

/**
 *	A unique ID representing a base application.
 *
 * 	@ingroup common
 */
typedef ServerAppInstanceID BaseAppID;

/**
 *	A unique ID representing a login application.
 *
 * 	@ingroup common
 */
typedef ServerAppInstanceID LoginAppID;

/**
 *	A unique ID representing a login application.
 *
 * 	@ingroup common
 */
typedef ServerAppInstanceID DBAppID;

/**
 *	The authentication key used between ClientApp and BaseApp.
 */
typedef uint32 SessionKey;

/**
 *	A 3D vector used for storing object positions.
 *
 * 	@ingroup common
 */
typedef Vector3 Position3D;

/**
 *	A unique ID representing a class of entity.
 *
 * 	@ingroup common
 */
typedef uint16 EntityTypeID;
const EntityTypeID INVALID_ENTITY_TYPE_ID = EntityTypeID(-1);

/**
 *  An ID representing an entities position in the database
 */
typedef int64 DatabaseID;
const DatabaseID PENDING_DATABASE_ID = -1;

/**
 *  printf format string for DatabaseID e.g.
 *		DatabaseID dbID = &lt;something>;
 *		printf( "Entity %" FMT_DBID " is not valid!", dbID );
 */
#ifndef _WIN32
#ifndef _LP64
	#define FMT_DBID "lld"
#else
	#define FMT_DBID "ld"
#endif
#else
	#define FMT_DBID "I64d"
#endif

/**
 *	This is a 32 bit ID that represents a grid square. The most significant
 *	16 bits is the signed X position. The least significant 16 bits is the
 *	signed Y position.
 */
typedef uint32 GridID;

/**
 *	The type used as a sequence number for the events of an entity.
 */
typedef int32 EventNumber;
const EventNumber INITIAL_EVENT_NUMBER = 1;

typedef BW::vector< EventNumber > CacheStamps;


/**
 *	The type used as a sequence number for volatile updates.
 */
typedef uint16 VolatileNumber;

typedef uint8 DetailLevel;

const int MAX_DATA_LOD_LEVELS = 6;

/**
 * 	This structure stores a 3D direction.
 *
 * 	@ingroup common
 */
struct BWENTITY_API Direction3D
{
	Direction3D() {};
	explicit Direction3D( const Vector3 & v ) :
		roll ( v[0] ),
		pitch( v[1] ),
		yaw  ( v[2] ) {}

	Vector3 asVector3() const { return Vector3( roll, pitch, yaw ); }

	float roll;		///< The roll component of the direction
	float pitch;	///< The pitch component of the direction
	float yaw;		///< The yaw component of the direction
};

BinaryIStream & operator>>( BinaryIStream & is, Direction3D & d );
BinaryOStream & operator<<( BinaryOStream & is, const Direction3D & d );

std::istream & operator>>( std::istream & is, Direction3D & d );
std::ostream & operator<<( std::ostream & os, const Direction3D & d );

inline bool operator==( const Direction3D & f1, const Direction3D & f2 );
inline bool operator!=( const Direction3D & f1, const Direction3D & f2 );


namespace Mercury
{
	/**
	 *	This class encapsulates an IP address and UDP/TCP port.
	 *
	 *	@ingroup mercury
	 */
	class Address
	{
	public:
		/// @name Construction/Destruction
		// @{
		Address();
		Address( uint32 ipArg, uint16 portArg );
		// @}

		uint32	ip;		///< IP address.
		uint16	port;	///< The port.
		uint16	salt;	///< Different each time.

		int writeToString( char * str, int length ) const;

		// TODO: Remove this operator
		operator char*() const	{ return this->c_str(); }
		char * c_str() const;
		const char * ipAsString() const;

		bool isNone() const			{ return this->ip == 0; }

		static Watcher & watcher();

		static const Address NONE;

	private:
		/// Temporary storage used for converting the address to a string.  At
		/// present we support having two string representations at once.
		static const int MAX_STRLEN = 32;
		static char s_stringBuf[ 2 ][ MAX_STRLEN ];
		static int s_currStringBuf;
		static char * nextStringBuf();
	};

	inline bool operator==(const Address & a, const Address & b);
	inline bool operator!=(const Address & a, const Address & b);
	inline bool operator<(const Address & a, const Address & b);

	BinaryIStream& operator>>( BinaryIStream &in, Address &a );
	BinaryOStream& operator<<( BinaryOStream &out, const Address &a );

	bool watcherStringToValue( const char *, Address & );
	BW::string watcherValueToString( const Address & );
} // namespace Mercury


#if defined( MF_SERVER ) || defined( __APPLE__ )
/**
 *	This structure is a packed version of a mailbox for an entity
 */
class EntityMailBoxRef
{
public:
	EntityID			id;
	Mercury::Address	addr;

	enum Component
	{
		CELL = 0,
		BASE = 1,
		CLIENT = 2,
		BASE_VIA_CELL = 3,
		CLIENT_VIA_CELL = 4,
		CELL_VIA_BASE = 5,
		CLIENT_VIA_BASE = 6,
		SERVICE = 7
	};

	EntityMailBoxRef():
		id( 0 ),
		addr( Mercury::Address::NONE )
	{}
	
	bool hasAddress() const 		{ return addr != Mercury::Address::NONE; }

	Component component() const		{ return (Component)(addr.salt >> 13); }
	void component( Component c )	{ addr.salt = type() | (uint16(c) << 13); }

	EntityTypeID type() const		{ return addr.salt & 0x1FFF; }
	void type( EntityTypeID t )		{ addr.salt = (addr.salt & 0xE000) | t; }

	void init() { id = 0; addr.ip = 0; addr.port = 0; addr.salt = 0; }
	void init( EntityID i, const Mercury::Address & a,
		Component c, EntityTypeID t )
	{ id = i; addr = a; addr.salt = (uint16(c) << 13) | t; }

	static const char * componentAsStr( Component component );

	const char * componentName() const
	{
		return componentAsStr( this->component() );
	}
};
#endif // defined( MF_SERVER ) || defined( __APPLE__ )


/**
 *	This class maintains a set of 32 boolean capabilities.
 *
 *	@ingroup common
 */
class Capabilities
{
	typedef uint32 CapsType;
public:
	Capabilities();
	void add( uint cap );
	bool has( uint cap ) const;
	bool empty() const;
	bool match( const Capabilities& on, const Capabilities& off ) const;
	bool matchAny( const Capabilities& on, const Capabilities& off ) const;

	/// This is the maximum cap that a Capability object can store.
	static const uint s_maxCap_ = std::numeric_limits<CapsType>::digits - 1;
private:

	/// This member stores the capabilities bitmask.
	CapsType	caps_;
};


/// This type identifies an entry in the data for a space
class SpaceEntryID : public Mercury::Address { };
inline bool operator==(const SpaceEntryID & a, const SpaceEntryID & b);
inline bool operator!=(const SpaceEntryID & a, const SpaceEntryID & b);
inline bool operator<(const SpaceEntryID & a, const SpaceEntryID & b);

#include "basictypes.ipp"

BW_END_NAMESPACE


BW_HASH_NAMESPACE_BEGIN

template<>
struct hash< BW::Mercury::Address >
{
	typedef BW::Mercury::Address argument_type;
	typedef std::size_t result_type;
	result_type operator()( const argument_type & addr ) const
	{
		return BW::hash< BW::uint64 >()(
				((BW::uint64)(addr.ip)) << 32 | addr.port );
	}
};

BW_HASH_NAMESPACE_END


#endif // BASIC_TYPES_HPP
