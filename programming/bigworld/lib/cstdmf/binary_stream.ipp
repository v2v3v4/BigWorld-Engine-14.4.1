/**
 *  This file provides the platform specific implementations of the streaming
 *  operators for all primitive types, as well as the byte-swapping macros
 *  for use when working directly with packet data.  Note that since we are
 *  supporting both big and little endian clients, we can no longer have a
 *  generalised streaming operator template that works for non-primitive types.
 *  That means any struct/class that is written to a network packet must now
 *  provide its own streaming operators.
 *
 *  The important thing to remember here is that the BigWorld network byte order
 *  is little-endian, not big-endian.
 */

// -----------------------------------------------------------------------------
// Section: Basic types
// -----------------------------------------------------------------------------

/**
 *  We need these unions because of strange things that happen with different
 *  compilers when you try to treat, for example, floats as uint32&'s during
 *  (de)streaming operations.
 */

typedef union
{
	uint8 u8;
	int8 i8;
	char c;
	unsigned char uc;
	bool b;
} bw_netbyte;


typedef union
{
	uint16 u16;
	int16 i16;
	short s;
	unsigned short us;
} bw_netshort;


typedef union
{
	uint32 u32;
	int32 i32;
	int i;
	float f;
} bw_netlong;


typedef union
{
	uint64 u64;
	int64 i64;
	long long ll;
	unsigned long long ull;
	double d;
} bw_netlonglong;


// -----------------------------------------------------------------------------
// Section: Byte swapping macros
// -----------------------------------------------------------------------------

// NOTE: BigWorld uses little-endian over the network, not big-endian which is
// the standard network byte order.
// The BW_HTON* macros convert from host to little-endian byte order.
// The BW_NTOH* macros convert from little-endian to host byte order.

#ifdef _BIG_ENDIAN

inline short BW_HTONS( short x )
{
	short res =
		((x & 0x00ff) << 8) |
		((x & 0xff00) >> 8);
	return res;
}


inline long BW_HTONL( long x )
{
	long res =
		((x & 0x000000ff) << 24) |
		((x & 0x0000ff00) << 8) |
		((x & 0x00ff0000) >> 8) |
		((x & 0xff000000) >> 24);
	return res;
}


inline long long BW_HTONLL( long long x )
{
	long long res =
		((x & 0x00000000000000ffULL) << 56) |
		((x & 0x000000000000ff00ULL) << 40) |
		((x & 0x0000000000ff0000ULL) << 24) |
		((x & 0x00000000ff000000ULL) << 8) |
		((x & 0x000000ff00000000ULL) >> 8) |
		((x & 0x0000ff0000000000ULL) >> 24) |
		((x & 0x00ff000000000000ULL) >> 40) |
		((x & 0xff00000000000000ULL) >> 56);
	return res;
}


inline float BW_HTONF( float f )
{
	bw_netlong n;
	n.f = f;
	n.u32 = BW_HTONL( n.u32 );
	return n.f;
}


/**
 *  This one is a bit different, because the 360 will crash with alignment
 *  exceptions if you try to assign floats to non-aligned areas of memory.
 *  Parts of our code try to do exactly that, so a bit more coaxing is needed.
 */
inline void BW_HTONF_ASSIGN( float &dest, float f )
{
	bw_netlong *pDest = (bw_netlong*)&dest;
	pDest->u32 = *(uint32*)&f;
	pDest->u32 = BW_HTONL( pDest->u32 );
}

#else // BIG_ENDIAN

#define BW_HTONS( x ) x
#define BW_HTONL( x ) x
#define BW_HTONLL( x ) x
#define BW_HTONF( x ) x
#define BW_HTONF_ASSIGN( dest, x ) (dest = x)

#endif


/**
 *  This is for streaming/destreaming between two 3 byte memory areas.  If your
 *  input or output is a long, use BW_PACK3()/BW_UNPACK3()
 */
inline void BW_HTON3_ASSIGN( char *pDest, const char *pData )
{
	pDest[0] = pData[0];
	pDest[1] = pData[1];
	pDest[2] = pData[2];
}


/**
 *  Pack the low 3 bytes of an int into a 3-byte memory area.
 */
inline void BW_PACK3( char *pDest, uint32 src )
{
	pDest[0] = (char)src;
	pDest[1] = (char)(src >> 8);
	pDest[2] = (char)(src >> 16);
}


/**
 *  Unpack a 3-byte memory area into an int.
 */
inline uint32 BW_UNPACK3( const char *pData )
{
	const uint8 *data = (const uint8*)pData;
	return data[0] | (data[1] << 8) | (data[2] << 16);
}


// The network-to-host operations are the same as the host-to-network ones.
#define BW_NTOHS( x ) BW_HTONS( x )
#define BW_NTOHL( x ) BW_HTONL( x )
#define BW_NTOHLL( x ) BW_HTONLL( x )
#define BW_NTOHF( x ) BW_HTONF( x )
#define BW_NTOHF_ASSIGN( dest, x ) BW_HTONF_ASSIGN( dest, x )
#define BW_NTOH3_ASSIGN( pDest, pData ) BW_HTON3_ASSIGN( pDest, pData )


// -----------------------------------------------------------------------------
// Section: Output streaming operators
// -----------------------------------------------------------------------------

#if !defined( BW_ENFORCE_ALIGNED_ACCESS )

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netbyte x )
{
	BW_STATIC_ASSERT( sizeof( bw_netbyte ) == 1, bw_netbyte_bad_size );
	*(char*)out.reserve( sizeof( x ) ) = x.c;
	return out;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netshort x )
{
	BW_STATIC_ASSERT( sizeof( bw_netshort ) == 2, bw_netshort_bad_size );
	*(short*)out.reserve( sizeof( x ) ) = BW_HTONS( x.s );
	return out;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netlong x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlong ) == 4, bw_netlong_bad_size );
	*(uint32*)out.reserve( sizeof( x ) ) = BW_HTONL( x.u32 );
	return out;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netlonglong x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlonglong ) == 8, bw_netlonglong_bad_size );
	*(uint64*)out.reserve( sizeof( x ) ) = BW_HTONLL( x.u64 );
	return out;
}

#else // defined( BW_ENFORCE_ALIGNED_ACCESS )

// Section: streaming for CPUs requiring aligned memory access only (ARM)

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netbyte x )
{
	BW_STATIC_ASSERT( sizeof( bw_netbyte ) == 1, bw_netbyte_bad_size );
	*(char*)out.reserve( sizeof( x ) ) = x.c;
	return out;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netshort x )
{
	BW_STATIC_ASSERT( sizeof( bw_netshort ) == 2, bw_netshort_bad_size );
	x.s = BW_HTONS( x.s );
	memcpy( out.reserve( sizeof( x ) ), &x.s, sizeof( x ) );
	return out;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netlong x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlong ) == 4, bw_netlong_bad_size );
	x.u32 = BW_HTONL( x.u32 );
	memcpy( out.reserve( sizeof( x ) ), &x.u32, sizeof( x ) );
	return out;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bw_netlonglong x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlonglong ) == 8, bw_netlonglong_bad_size );
	x.u64 = BW_HTONLL( x.u64 );
	memcpy( out.reserve( sizeof( x ) ), &x.u64, sizeof( x ) );
	return out;
}

#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )


inline BinaryOStream& operator<<( BinaryOStream &out, uint8 x )
{
	bw_netbyte n;
	n.u8 = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, uint16 x )
{
	bw_netshort n;
	n.u16 = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, uint32 x )
{
	bw_netlong n;
	n.u32 = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, uint64 x )
{
	bw_netlonglong n;
	n.u64 = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, int32 x )
{
	bw_netlong n;
	n.i32 = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, int64 x )
{
	bw_netlonglong n;
	n.i64 = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, char x )
{
	bw_netbyte n;
	n.c = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, bool x )
{
	bw_netbyte n;
	n.b = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, short x )
{
	bw_netshort n;
	n.s = x;
	return out << n;
}


inline BinaryOStream& operator<<( BinaryOStream &out, float x )
{
	bw_netlong n;
	n.f = x;
	return out << n;
}

inline BinaryOStream& operator<<( BinaryOStream &out, double x )
{
	bw_netlonglong n;
	n.d = x;
	return out << n;
}

//This is because the ps3 compiler does not like converting to
//int32, it should be removed as soon as a better solution is
//found
#if defined( PLAYSTATION3 )
inline BinaryOStream& operator<<( BinaryOStream &out, long x )
{
	bw_netlong n;
	n.i32 = x;
	return out << n;
}
#endif

/*
inline BinaryOStream& operator<<( BinaryOStream &out, void* x )
{
	bw_netlong n;
	n.p = x;
	return out << n;
}
*/

// Provide the generalised template on intel so the server can compile without
// porting all the damn server interfaces to use MERCURY_[IO]STREAM()
#ifndef _BIG_ENDIAN
template <class TYPE>
inline BinaryOStream& operator<<( BinaryOStream &os, const TYPE &t )
{
	os.insertRaw( t );
	return os;
}
#endif


// -----------------------------------------------------------------------------
// Section: Input streaming operators
// -----------------------------------------------------------------------------

#if !defined( BW_ENFORCE_ALIGNED_ACCESS )

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netbyte &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netbyte ) == 1, bw_netbyte_bad_size );
	x.c = *(char*)in.retrieve( sizeof( x ) );
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netshort &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netshort ) == 2, bw_netshort_bad_size );
	x.s = BW_NTOHS( *(short*)in.retrieve( sizeof( x ) ) );
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netlong &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlong ) == 4, bw_netlong_bad_size );
	x.u32 = BW_NTOHL( *(uint32*)in.retrieve( sizeof( x ) ) );
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netlonglong &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlonglong ) == 8, bw_netlonglong_bad_size );
	x.u64 = BW_NTOHLL( *(uint64*)in.retrieve( sizeof( x ) ) );
	return in;
}

#else // defined( BW_ENFORCE_ALIGNED_ACCESS )

// Section: streaming for CPUs requiring aligned memory access only (ARM)

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netbyte &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netbyte ) == 1, bw_netbyte_bad_size );
	x.c = *(char*)in.retrieve( sizeof( x ) );
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netshort &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netshort ) == 2, bw_netshort_bad_size );
	memcpy( &x.s, in.retrieve( sizeof( x ) ), sizeof( x ) );
	x.s = BW_NTOHS( x.s );
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netlong &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlong ) == 4, bw_netlong_bad_size );
	memcpy( &x.u32, in.retrieve( sizeof( x ) ), sizeof( x ) );
	x.u32 = BW_NTOHL( x.u32 );
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bw_netlonglong &x )
{
	BW_STATIC_ASSERT( sizeof( bw_netlonglong ) == 8, bw_netlonglong_bad_size );
	memcpy( &x.u64, in.retrieve( sizeof( x ) ), sizeof( x ) );
	x.u64 = BW_NTOHLL( x.u64 );
	return in;
}

#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )


inline BinaryIStream& operator>>( BinaryIStream &in, uint8 &x )
{
	bw_netbyte n;
	in >> n;
	x = n.u8;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, uint16 &x )
{
	bw_netshort n;
	in >> n;
	x = n.u16;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, uint32 &x )
{
	bw_netlong n;
	in >> n;
	x = n.u32;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, uint64 &x )
{
	bw_netlonglong n;
	in >> n;
	x = n.u64;
	return in;
}

#if defined( PLAYSTATION3 )
inline BinaryIStream& operator>>( BinaryIStream &in, int8 &x )
{
	bw_netbyte n;
	in >> n;
	x = n.b;
	return in;
}
#endif

inline BinaryIStream& operator>>( BinaryIStream &in, int32 &x )
{
	bw_netlong n;
	in >> n;
	x = n.i32;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, int64 &x )
{
	bw_netlonglong n;
	in >> n;
	x = n.i64;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, char &x )
{
	bw_netbyte n;
	in >> n;
	x = n.c;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, bool &x )
{
	bw_netbyte n;
	in >> n;
	x = n.b;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, short &x )
{
	bw_netshort n;
	in >> n;
	x = n.u16;
	return in;
}


inline BinaryIStream& operator>>( BinaryIStream &in, float &x )
{
	bw_netlong n;
	in >> n;
	x = n.f;
	return in;
}

inline BinaryIStream& operator>>( BinaryIStream &in, double &x )
{
	bw_netlonglong n;
	in >> n;
	x = n.d;
	return in;
}

#ifndef _BIG_ENDIAN
template <class TYPE>
inline BinaryIStream& operator>>( BinaryIStream &is, TYPE &t )
{
	is.extractRaw( t );
	return is;
}
#endif

// binary_stream.ipp
