#include "pch.hpp"
#include "bwlockd_connection.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#ifndef _NAVGEN
#include "appmgr/commentary.hpp"

#endif // _NAVGEN

#include "Iphlpapi.h"
#pragma comment( lib, "Iphlpapi.lib" )

#include <sstream>
#include <limits>


DECLARE_DEBUG_COMPONENT2( "BWLockDConnection", 2 );


BW_BEGIN_NAMESPACE

namespace BWLock
{

const unsigned char	BWLOCKCOMMAND_INVALID = 0;
const unsigned char	BWLOCKCOMMAND_CONNECT = 'C';
const unsigned char	BWLOCKCOMMAND_SETUSER = 'A';
const unsigned char	BWLOCKCOMMAND_SETSPACE = 'S';
const unsigned char	BWLOCKCOMMAND_LOCK = 'L';
const unsigned char	BWLOCKCOMMAND_UNLOCK = 'U';
const unsigned char	BWLOCKCOMMAND_GETSTATUS = 'G';

const unsigned char	BWLOCKFLAG_SUCCESS = 0;

const unsigned int BWLOCK_MAX_USERNAME_LENGTH = MAX_COMPUTERNAME_LENGTH + MAX_ADAPTER_ADDRESS_LENGTH * 2 + 35;
const unsigned int BWLOCK_MAX_SPACE_LENGTH = 1024;
const unsigned int BWLOCK_MAX_DESCRIPTION_LENGTH = 10240;

#pragma pack( push, 1 )


struct Command
{
	unsigned int size_;
	unsigned char id_;
	unsigned char flag_;
	Command( unsigned char id ) : id_( id ), flag_( 0 ), size_( 0xffffffff )
	{}
};


struct SetUserCommand : public Command
{
	char username_[ BWLOCK_MAX_USERNAME_LENGTH + 1 ];
	SetUserCommand( const BW::string& username )
		: Command( BWLOCKCOMMAND_SETUSER )
	{
		BW_GUARD;

		strncpy( username_, username.c_str(), BWLOCK_MAX_USERNAME_LENGTH );
		username_[ BWLOCK_MAX_USERNAME_LENGTH ] = 0;
		size_ = static_cast<uint>(sizeof( Command ) + strlen( username_ ));
	}
};


struct SetSpaceCommand : public Command
{
	char spacename_[ BWLOCK_MAX_SPACE_LENGTH + 1 ];
	SetSpaceCommand( const BW::string& spacename )
		: Command( BWLOCKCOMMAND_SETSPACE )
	{
		BW_GUARD;

		strncpy( spacename_, spacename.c_str(), BWLOCK_MAX_SPACE_LENGTH );
		spacename_[ BWLOCK_MAX_SPACE_LENGTH ] = 0;
		size_ = static_cast<uint>(sizeof( Command ) + strlen( spacename_ ));
	}
};


struct LockCommand : public Command
{
	short left_;
	short top_;
	short right_;
	short bottom_;
	char desc_[ BWLOCK_MAX_DESCRIPTION_LENGTH + 1 ];
	LockCommand( short left, short top, short right, short bottom, const BW::string& desc )
		: Command( BWLOCKCOMMAND_LOCK ), left_( left ), top_( top ), right_( right ), bottom_( bottom )
	{
		BW_GUARD;

		strncpy( desc_, desc.c_str(), BWLOCK_MAX_DESCRIPTION_LENGTH );
		desc_[ BWLOCK_MAX_DESCRIPTION_LENGTH ] = 0;
		size_ = static_cast<uint>( sizeof( LockCommand ) - 
						BWLOCK_MAX_DESCRIPTION_LENGTH - 1 + strlen( desc_ ));
	}
};


struct UnlockCommand : public Command
{
	short left_;
	short top_;
	short right_;
	short bottom_;
	char desc_[ BWLOCK_MAX_DESCRIPTION_LENGTH + 1 ];
	UnlockCommand( short left, short top, short right, short bottom, const BW::string& desc )
		: Command( BWLOCKCOMMAND_UNLOCK ), left_( left ), top_( top ), right_( right ), bottom_( bottom )
	{
		BW_GUARD;

		strncpy( desc_, desc.c_str(), BWLOCK_MAX_DESCRIPTION_LENGTH );
		desc_[ BWLOCK_MAX_DESCRIPTION_LENGTH ] = 0;
		size_ = static_cast<uint>( sizeof( UnlockCommand ) - 
						BWLOCK_MAX_DESCRIPTION_LENGTH - 1 + strlen( desc_ ));
	}
};


struct GetStatusCommand : public Command
{
	GetStatusCommand() : Command( BWLOCKCOMMAND_GETSTATUS )
	{
		size_ = sizeof( GetStatusCommand );
	}
};


#pragma pack( pop )


struct GetCstr
{
	BW::string operator()( const unsigned char* data, int& offset ) const
	{
		BW_GUARD;

		int size = *(int*)data;
		BW::string result;
		while( offset < size && data[ offset ] )
		{
			result += (char)data[ offset ];
			++offset;
		}
		return result;
	}
}
getCstr;


struct GetString
{
	BW::string operator()( const unsigned char* data, int& offset ) const
	{
		BW_GUARD;

		BW::string result;
		result.assign( (char*)data + offset + sizeof( int ), *(int*)( data + offset ) );
		offset += sizeof( int ) + *(int*)( data + offset );
		return result;
	}
}
getString;


struct getNum
{
	const unsigned char* data_;
	int& offset_;
	getNum( const unsigned char* data, int& offset ) : data_( data ), offset_( offset )
	{}
#define CONVERT_OPERATOR( T )				\
	operator T()							\
	{										\
		BW_GUARD;							\
											\
		T t = *(T*)( data_ + offset_ );		\
		offset_ += sizeof( T );				\
		return t;							\
	}
	CONVERT_OPERATOR( short )
	CONVERT_OPERATOR( int )
	CONVERT_OPERATOR( float )
};


/** Get the branch for the current space */
BW::string getCurrentTag( const BW::string& space )
{
	// TODO:UNICODE: Do we need to keep legacy CVS support?
	//DataSectionPtr pDS = BWResource::openSection( space + "/CVS/Tag" );	
	//if (pDS)
	//{
	//	BW::string s = pDS->asString();

	//	// ignore the 1st character, it's a marker we don't need
	//	char* start = &s[1];

	//	// ignore all CRs and LFs at the end
	//	char* end = &s[s.size() - 1];
	//	while (*end == '\r' || *end == '\n')
	//		end--;
	//	*(++end) = '\0';

	//	return start;
	//}
	//else
	{
		return "MAIN";
	}
}
}


namespace BWLock
{

BWLockDConnection::BWLockDConnection()
	: port_( 8168 )
	, connected_( false )
	, enabled_( false )
	, xExtent_( 0 )
	, zExtent_( 0 )
	, waitingForCommandReply_( false )
{
}


void BWLockDConnection::registerNotification( Notification* n )
{
	BW_GUARD;

	notifications_.insert( n );
}


void BWLockDConnection::unregisterNotification( Notification* n )
{
	BW_GUARD;

	notifications_.erase( n );
}


void BWLockDConnection::notify() const
{
	BW_GUARD;

	BW::set<Notification*> notifications( notifications_ );
	for( BW::set<Notification*>::iterator iter = notifications.begin();
		iter != notifications.end(); ++iter )
	{
		(*iter)->changed();
	}
}


namespace
{

	// the name is a combination of computer name and first network adapter's hardware address
	BW::string getUniqueComputerName()
	{
		BW_GUARD;

		char tbl[] = "abcdefghijklmnopqrstuvwxyz";
		char macAddress[ MAX_ADAPTER_ADDRESS_LENGTH * 2 + 1 ];
		ULONG macAddrLen = 0;

		// get adapter's hardware address
		ULONG adptLen = sizeof( IP_ADAPTER_INFO );
		IP_ADAPTER_INFO * adptInfo = (IP_ADAPTER_INFO *)bw_malloc( adptLen );

		if (adptInfo)
		{
			DWORD retv;

			if ((retv = GetAdaptersInfo( adptInfo, &adptLen )) == ERROR_BUFFER_OVERFLOW)
			{
				adptInfo = (IP_ADAPTER_INFO *)bw_realloc(adptInfo, adptLen);
				retv = GetAdaptersInfo( adptInfo, &adptLen );
			}

			if (retv == NO_ERROR)
			{
				// convert hardware address to a string made by 'a'to 'z'
				ULONG val = 0;

				for (ULONG i = 0; i < adptInfo->AddressLength; i ++)
				{
					val += adptInfo->Address[ i ];
					macAddress[ macAddrLen++ ] = tbl[ val % 26 ];
					val /= 26;
				}

				while (val)
				{
					macAddress[ macAddrLen++ ] = tbl[ val % 26 ];
					val /= 26;
				}
			}

			bw_free( adptInfo );
		}

		macAddress[ macAddrLen ] = '\0';

		// get computer name, convert it to lower case string and change all '.' in it to '_'
		/* note: RFCs mandate that a hostname's labels may contain only the ASCII 
				 letters 'a' through 'z' (case-insensitive), the digits '0' 
				 through '9', and the hyphen. Hostname labels cannot begin or 
				 end with a hyphen. No other symbols, punctuation characters, 
				 or blank spaces are permitted.

			http://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_host_names
		*/
		char computerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
		DWORD nmLen = MAX_COMPUTERNAME_LENGTH + 1;
		GetComputerNameA( computerName, &nmLen );
		computerName[ nmLen ] = '\0';
		strlwr( computerName );

		BW::string result = computerName;
		std::replace( result.begin(), result.end(), '.', '_' );

		return result + '-' + macAddress;

	}
}

bool BWLockDConnection::init( const BW::string& hoststrArg, const BW::string& username, int xExtent, int zExtent )
{
	BW_GUARD;

	if( connected() )
		disconnect();
	enabled_ = true;
	port_ = 8168;
	username_ = username;
	xExtent_ = xExtent;
	zExtent_ = zExtent;
	xMin_ = zMin_ = std::numeric_limits<short>::max();
	xMax_ = zMax_ = std::numeric_limits<short>::min();

	char localhost[ 10240 ];
	gethostname( localhost, 10240 );

	self_ = getUniqueComputerName();

	if( hostent* ent = gethostbyname( localhost ) )
	{
		char** addr_list = ent->h_addr_list;
		while( *addr_list )
		{
			unsigned long ip = *(DWORD*)( *addr_list );
			if( HOSTENT* host = gethostbyaddr( (char*)&ip, sizeof( ip ), AF_INET ) )
			{
				BW::string defaultHost = "fd";
				const BW::string & hoststr = (!hoststrArg.empty()) ? hoststrArg : defaultHost;

				BW::string::size_type pos = hoststr.find_first_of( ':' );
				if (pos == BW::string::npos)
					host_ = hoststr;
				else
				{
					host_ = hoststr.substr( 0, pos );
					BW::string portstr = hoststr.substr( pos + 1 );
					port_ = atoi( portstr.c_str() );
				}
				return true;
			}
			++addr_list;
		}
	}
	return false;
}


bool BWLockDConnection::connect()
{
	BW_GUARD;

	if( !enabled() )
		return false;

	MF_ASSERT( !connected_ );

	u_int32_t addr;

	if( Endpoint::convertAddress( host_.c_str(), (u_int32_t&)addr ) != 0 )
	{
		INFO_MSG( "BWLockDConnection::Connect(): Couldn't resolve address %s\n", host_.c_str() );
		addCommentary( Localise( L"WORLDEDITOR/WORLDEDITOR/PROJECT/BIGBANGD_CONNECTION/CANNOT_RESOLVE_ADDR", host_ ), true );
		return false;
	}
	ep_.socket( SOCK_STREAM );
	if (ep_.connect( htons( port_ ), addr ) == SOCKET_ERROR)
	{
		INFO_MSG( "BWLockDConnection::Connect(): Couldn't connect, last error is %i\n", WSAGetLastError() );
		addCommentary( Localise( L"WORLDEDITOR/WORLDEDITOR/PROJECT/BIGBANGD_CONNECTION/CAMNOT_CONNECT", WSAGetLastError() ), true );
		return false;
	}

	connected_ = true;
	INFO_MSG( "Connected to bwlockd\n" );
	addCommentary( Localise( L"WORLDEDITOR/WORLDEDITOR/PROJECT/BIGBANGD_CONNECTION/CONNECTED" ), false );

	processReply( BWLOCKCOMMAND_CONNECT );

	return connected_;
}


bool BWLockDConnection::changeSpace( BW::string newSpace )
{
	BW_GUARD;

	lockspace_ = newSpace + "/" + getCurrentTag( newSpace );

	if( !enabled() )
		return true;

	if( !connected() && !connect() )
		return false;

	computers_.clear();

	xMin_ = zMin_ = std::numeric_limits<short>::max();
	xMax_ = zMax_ = std::numeric_limits<short>::min();

	gridStatus_.clear();


	SetUserCommand userCmd( self_ + "::" + username_ );
	sendCommand( &userCmd );
	processReply( BWLOCKCOMMAND_SETUSER );

	SetSpaceCommand spaceCmd( lockspace_ );
	sendCommand( &spaceCmd );
	processReply( BWLOCKCOMMAND_SETSPACE );


	GetStatusCommand statCmd;
	sendCommand( &statCmd );
	BW::vector<unsigned char> reply = getReply( BWLOCKCOMMAND_GETSTATUS );

	if( connected_ )
	{
		int offset = sizeof( Command );
		unsigned char* command = &reply[0];
		int total = *(int*)command;

		while( offset < total )
		{
			int recordSize = getNum( command, offset );
			Computer computer;
			computer.name_ = getString( command, offset );
			computer.name_ = computer.name_.substr( 0, computer.name_.find( '.' ) );

			int lockNum = getNum( command, offset );
			while( lockNum )
			{
				Lock lock;
				lock.rect_.left_ = getNum( command, offset );
				lock.rect_.top_ = getNum( command, offset );
				lock.rect_.right_ = getNum( command, offset );
				lock.rect_.bottom_ = getNum( command, offset );
				lock.username_ = getString( command, offset );
				lock.desc_ = getString( command, offset );
				lock.time_ = getNum( command, offset );
				computer.locks_.push_back( lock );
				--lockNum;
			}
			computers_.push_back( computer );
		}
		rebuildGridStatus();
		notify();
	}

	return connected();
}


void BWLockDConnection::disconnect()
{
	BW_GUARD;

	if( !enabled() )
		return;
	computers_.clear();
	ep_.close();

	xMin_ = zMin_ = std::numeric_limits<short>::max();
	xMax_ = zMax_ = std::numeric_limits<short>::min();

	gridStatus_.clear();

	connected_ = false;
}


bool BWLockDConnection::connected() const
{
	return connected_;
}


void BWLockDConnection::linkPoint( int16 oldLeft, int16 oldTop, int16 newLeft, int16 newTop )
{
	BW_GUARD;

	BW::set<Rect> oldRects = getLockRects( oldLeft, oldTop );
	BW::set<Rect> newRects = getLockRects( newLeft, newTop );
	BW::set<Rect>::size_type oldSize = oldRects.size();
	oldRects.insert( newRects.begin(), newRects.end() );
	if( oldRects.size() != oldSize )
		linkPoints_.push_back( LinkPoint( Point( oldLeft, oldTop ), Point( newLeft, newTop ) ) );
}


bool BWLockDConnection::lock( const GridRect& rect, const BW::string description )
{
	BW_GUARD;

	if( !enabled() || !connected_ )
	{
		INFO_MSG( "not connected, not aquiring lock\n");
		return false;
	}

	INFO_MSG( "starting lock\n" );

	int left = min( rect.bottomLeft.x, rect.topRight.x ) - xExtent_;
	int right = max( rect.bottomLeft.x, rect.topRight.x ) + xExtent_ - 1;
	int top = min( rect.bottomLeft.y, rect.topRight.y ) - zExtent_;
	int bottom = max( rect.bottomLeft.y, rect.topRight.y ) + zExtent_ - 1;

	LockCommand lockCmd( left, top, right, bottom, description );
	sendCommand( &lockCmd );
	BW::vector<unsigned char> result = getReply( BWLOCKCOMMAND_LOCK, true );
	Command* command = (Command*)&result[0];


	int offset = sizeof( Command );
	BW::string comment = getCstr( (unsigned char*)command, offset );
	addCommentary( comment, !!command->flag_ );

	if( command->flag_ == BWLOCKFLAG_SUCCESS )
	{
		waitingForCommandReply_ = true;
		while (connected() && waitingForCommandReply_)
		{
			tick();
		}
		return true;
	}
	return false;
}


void BWLockDConnection::unlock( Rect rect, const BW::string description )
{
	BW_GUARD;

	if( !enabled() || !connected_ )
		return;

	INFO_MSG( "starting unlock\n" );

	UnlockCommand unlockCmd( rect.left_, rect.top_, rect.right_, rect.bottom_, description );
	sendCommand( &unlockCmd );
	BW::vector<unsigned char> result = getReply( BWLOCKCOMMAND_UNLOCK, true );
	Command* command = (Command*)&result[0];
	if( command->flag_ == BWLOCKFLAG_SUCCESS )
	{
		waitingForCommandReply_ = true;
		while (connected() && waitingForCommandReply_)
		{
			tick();
		}
	}


	int offset = sizeof( Command );
	BW::string comment = getCstr( (unsigned char*)command, offset );
	addCommentary( comment,	!!command->flag_ );
}


bool BWLockDConnection::isWritableByMe( int16 x, int16 z ) const
{
	BW_GUARD;

	if( !enabled() )
		return true;
	if( x >= xMin_ && x <= xMax_ && z >= zMin_ && z <= zMax_ )
	{
		int start = ( z - zMin_ ) * ( xMax_ - xMin_ + 1 );
		return gridStatus_[ start + x - xMin_ ] == GS_WRITABLE_BY_ME;
	}
	return false;
}


bool BWLockDConnection::isLockedByMe( int16 x, int16 z ) const
{
	BW_GUARD;

	if( !enabled() )
		return true;
	if( x >= xMin_ && x <= xMax_ && z >= zMin_ && z <= zMax_ )
	{
		int start = ( z - zMin_ ) * ( xMax_ - xMin_ + 1 );
		return gridStatus_[ start + x - xMin_ ] == GS_WRITABLE_BY_ME ||
			gridStatus_[ start + x - xMin_ ] == GS_LOCKED_BY_ME;
	}
	return false;
}


bool BWLockDConnection::isLockedByOthers( int16 x, int16 z ) const
{
	BW_GUARD;

	if( !enabled() )
		return false;
	if( x >= xMin_ && x <= xMax_ && z >= zMin_ && z <= zMax_ )
	{
		int start = ( z - zMin_ ) * ( xMax_ - xMin_ + 1 );
		return gridStatus_[ start + x - xMin_ ] == GS_LOCKED_BY_OTHERS;
	}
	return false;
}


bool BWLockDConnection::isSameLock( int16 x1, int16 z1, int16 x2, int16 z2 ) const
{
	BW_GUARD;

	if( !enabled() )
		return true;
	if( !isLockedByMe( x1, z1 ) || !isLockedByMe( x2, z2 ) )
		return false;
	BW::set<Rect> rects = getLockRects( x1, z1 );
	for( BW::set<Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
		if( iter->in( x2, z2 ) )
			return true;
	return false;
}


bool BWLockDConnection::isAllLocked() const
{
	BW_GUARD;

	if( !enabled() )
		return true;
	for( BW::vector<GridStatus>::const_iterator iter = gridStatus_.begin();
		iter != gridStatus_.end(); ++iter )
		if( *iter != GS_WRITABLE_BY_ME && *iter != GS_LOCKED_BY_ME )
			return false;
	return true;
}


GridInfo BWLockDConnection::getGridInformation( int16 x, int16 z ) const
{
	BW_GUARD;

	GridInfo result;

	if( !enabled() )
		return result;

	for( BW::vector<Computer>::const_iterator iter = computers_.begin(); iter != computers_.end(); ++iter )
	{
		for( BW::vector<Lock>::const_iterator it = iter->locks_.begin();
			it != iter->locks_.end(); ++it )
		{
			if( it->rect_.in( x, z ) )
			{
				char tmpbuf[128];
				time_t time = (time_t)it->time_;
				ctime_s( tmpbuf, 26, &time );
				result.push_back( std::make_pair( "Who:", it->username_ + " at " + iter->name_ ) );
				result.push_back( std::make_pair( "When:", tmpbuf ) );
				result.push_back( std::make_pair( "Message:", it->desc_ ) );
				return result;
			}
		}
	}
	return result;
}


BW::set<Rect> BWLockDConnection::getLockRectsNoLink( int16 x, int16 z ) const
{
	BW_GUARD;

	if( !enabled() )
	{
		BW::set<Rect> result;
		result.insert( Rect( std::numeric_limits<short>::min(), std::numeric_limits<short>::min(),
			std::numeric_limits<short>::max(), std::numeric_limits<short>::max() ) );
		return result;
	}
	for( BW::vector<Computer>::const_iterator iter = computers_.begin(); iter != computers_.end(); ++iter )
	{
		if( stricmp( iter->name_.c_str(), self_.c_str() ) == 0 )
		{
			for( BW::vector<Lock>::const_iterator it = iter->locks_.begin();
				it != iter->locks_.end(); ++it )
			{
				if( it->rect_.in( x, z ) )
				{
					BW::set<Rect> result;
					result.insert( it->rect_ );
					bool changed = true;
					while( changed )
					{
						changed = false;
						for( BW::vector<Lock>::const_iterator it = iter->locks_.begin();
							it != iter->locks_.end(); ++it )
						{
							if( result.find( it->rect_ ) != result.end() )
								continue;
							for( BW::set<Rect>::iterator sit = result.begin(); sit != result.end(); ++sit )
							{
								if( sit->intersect( it->rect_ ) )
								{
									result.insert( it->rect_ );
									changed = true;
									break;
								}
							}
						}
					}
					return result;
				}
			}
		}
	}
	return BW::set<Rect>();
}


BW::set<Rect> BWLockDConnection::getLockRects( int16 x, int16 z ) const
{
	BW_GUARD;

	BW::set<Rect> result = getLockRectsNoLink( x, z );
	for( BW::vector<LinkPoint>::const_iterator iter = linkPoints_.begin(); iter != linkPoints_.end(); ++iter )
	{
		std::pair<int16,int16> p1 = iter->first;
		std::pair<int16,int16> p2 = iter->second;
		for( BW::set<Rect>::const_iterator siter = result.begin(); siter != result.end(); ++siter )
			if( siter->in( p1.first, p1.second ) )
			{
				BW::set<Rect> sub = getLockRectsNoLink( p2.first, p2.second );
				result.insert( sub.begin(), sub.end() );
				break;
			}
			else if( siter->in( p2.first, p2.second ) )
			{
				BW::set<Rect> sub = getLockRectsNoLink( p1.first, p1.second );
				result.insert( sub.begin(), sub.end() );
				break;
			}
	}
	return result;
}


void BWLockDConnection::sendCommand( const Command* command )
{
	BW_GUARD;

	if( connected_ )
	{
		unsigned int offset = 0;
		while( offset != command->size_ )
		{
			int ret = ep_.send( (const unsigned char*)command + offset, command->size_ - offset );
			if( ret == SOCKET_ERROR || ret == 0 )
			{
				INFO_MSG( "sendCommand socket error or socket broken, last error is %d\n", WSAGetLastError() );
				disconnect();
				break;
			}

			offset += ret;
		}
	}
}


BW::vector<unsigned char> BWLockDConnection::recvCommand()
{
	BW_GUARD;

	BW::vector<unsigned char> result;
	if( connected_ && available() )
	{
		unsigned int offset = 0;
		result.resize( sizeof( unsigned int ) );
		while( offset != result.size() )
		{
			int ret = ep_.recv( &result[0] + offset, int( result.size() - offset ) );

			if( ret == SOCKET_ERROR || ret == 0 )
			{
				INFO_MSG( "recvCommand socket error or socket broken, last error is %d\n", WSAGetLastError() );
				result.clear();
				disconnect();
				return result;
			}

			offset += ret;
		}

		result.resize( *(unsigned int*)&result[0] );
		while( offset != result.size() )
		{
			int ret = ep_.recv( &result[0] + offset, int( result.size() - offset ) );

			if( ret == SOCKET_ERROR || ret == 0 )
			{
				INFO_MSG( "recvAll socket error or socket broken, last error is %d\n", WSAGetLastError() );
				result.clear();
				disconnect();
				return result;
			}

			offset += ret;
		}
	}
	return result;
}


BW::vector<unsigned char> BWLockDConnection::getReply( unsigned char command,
														bool processInternalCommand /*= false*/ )
{
	BW_GUARD;

	while( connected_ )
	{
		BW::vector<unsigned char> reply = recvCommand();
		if( reply.empty() )
		{
			Sleep( 10 );
			continue;
		}
		Command* c = (Command*)&reply[0];
		if( c->id_ >= 'a' && c->id_ <= 'z' )
		{
			if( processInternalCommand )
			{
				this->processInternalCommand( reply );
				continue;
			}
			else
				return reply;
		}
		else if( c->id_ != command && command != BWLOCKCOMMAND_INVALID)
		{
			if (c->id_ == BWLOCKCOMMAND_CONNECT && c->flag_ != BWLOCKFLAG_SUCCESS)
			{
				disconnect();
			}
			else
			{
				continue;
			}
		}
		return reply;
	}
	return BW::vector<unsigned char>();
}


void BWLockDConnection::processInternalCommand( const BW::vector<unsigned char>& comm )
{
	BW_GUARD;

	if( connected() )
	{
		Command* c = (Command*)&comm[0];
		int offset = sizeof( Command );
		const unsigned char* command = &comm[0];
		int total = *(int*)command;

		Lock lock;
		BW::string computerName;
		lock.rect_.left_ = getNum( command, offset );
		lock.rect_.top_ = getNum( command, offset );
		lock.rect_.right_ = getNum( command, offset );
		lock.rect_.bottom_ = getNum( command, offset );
		computerName = getString( command, offset );
		computerName = computerName.substr( 0, computerName.find( '.' ) );
		lock.username_ = getString( command, offset );
		lock.desc_ = getString( command, offset );
		lock.time_ = getNum( command, offset );

		if( c->id_ == 'l' )
		{
			if (stricmp( computerName.c_str(), self_.c_str() ) == 0)
			{
				waitingForCommandReply_ = false;
			}

			bool found = false;
			for( BW::vector<Computer>::iterator iter = computers_.begin();
				iter != computers_.end(); ++iter )
			{
				if( stricmp( iter->name_.c_str(), computerName.c_str() ) == 0 )
				{
					iter->locks_.push_back( lock );
					found = true;
					break;
				}
			}
			if( !found )
			{
				Computer computer;
				computer.name_ = computerName;
				computer.locks_.push_back( lock );
				computers_.push_back( computer );
			}
			addCommentary( Localise( L"WORLDEDITOR/WORLDEDITOR/PROJECT/BIGBANGD_CONNECTION/LOCK",
				lock.username_, computerName, lock.rect_.left_, lock.rect_.top_,
				lock.rect_.right_, lock.rect_.bottom_, lock.desc_ ), false );
		}
		else if( c->id_ == 'u' )
		{
			if (stricmp( computerName.c_str(), self_.c_str() ) == 0)
			{
				waitingForCommandReply_ = false;
			}

			bool found = false;
			for( BW::vector<Computer>::iterator iter = computers_.begin();
				iter != computers_.end(); ++iter )
			{
				if( stricmp( iter->name_.c_str(), computerName.c_str() ) == 0 )
				{
					for( BW::vector<Lock>::iterator it = iter->locks_.begin();
						it != iter->locks_.end(); ++it )
					{
						if( it->rect_ == lock.rect_ )
						{
							iter->locks_.erase( it );
							if( iter->locks_.empty() )
								computers_.erase( iter );
							found = true;
							addCommentary( Localise( L"WORLDEDITOR/WORLDEDITOR/PROJECT/BIGBANGD_CONNECTION/UNLOCK",
								lock.username_, computerName, lock.rect_.left_, lock.rect_.top_,
								lock.rect_.right_, lock.rect_.bottom_, lock.desc_ ), false );
							break;
						}
					}
				}
				if( found )
					break;
			}
		}
		rebuildGridStatus();
		notify();
	}
}


void BWLockDConnection::processReply( unsigned char command )
{
	BW_GUARD;

	BW::vector<unsigned char> c = getReply( command );
	if( !c.empty() )
	{
		Command* command = (Command*)&c[0];

		int offset = sizeof( Command );
		BW::string comment = getCstr( (unsigned char*)command, offset );
		addCommentary( comment, !!command->flag_ );

		if( command->flag_ )
			disconnect();
		return;
	}
}


bool BWLockDConnection::available()
{
	BW_GUARD;

	if( connected() )
	{
		fd_set read;
		FD_ZERO( &read );
		FD_SET( ep_.fileno(), &read );
		timeval timeval = { 0 };
		int result = select( 0, &read, 0, 0, &timeval );
		if( result == SOCKET_ERROR )
		{
			disconnect();
			return false;
		}
		return result != 0;
	}
	return false;
}


BW::vector<unsigned char> BWLockDConnection::getLockData( int minX, int minY, unsigned int gridWidth, unsigned int gridHeight )
{
	BW_GUARD;

	BW::vector<unsigned char> result;

	if( !enabled() )
	{
		result.resize( gridWidth * gridHeight, GS_WRITABLE_BY_ME );
		return result;
	}
	result.reserve( gridWidth * gridHeight );
	for( int y = 0; y < (int)gridHeight; ++y )
		for( int x = 0; x < (int)gridWidth; ++x )
			if( isWritableByMe( x + minX, y + minY ) )
				result.push_back( GS_WRITABLE_BY_ME );
			else if( isLockedByMe( x + minX, y + minY ) )
				result.push_back( GS_LOCKED_BY_ME );
			else if( isLockedByOthers( x + minX, y + minY ) )
				result.push_back( GS_LOCKED_BY_OTHERS );
			else
				result.push_back( GS_NOT_LOCKED );
	return result;
}


bool BWLockDConnection::tick()
{
	BW_GUARD;

	if( available() )
	{
		BW::vector<unsigned char> command = getReply( BWLOCKCOMMAND_INVALID );
		processInternalCommand( command );
		return true;
	}
	return false;
}


BW::string BWLockDConnection::host() const
{
	return host_;
}


void BWLockDConnection::addCommentary( const BW::wstring& msg, bool isCritical )
{
#ifndef _NAVGEN
	Commentary::instance().addMsg( msg, isCritical ? Commentary::CRITICAL : Commentary::COMMENT );
#endif//_NAVGEN
}


void BWLockDConnection::addCommentary( const BW::string& msg, bool isCritical )
{
#ifndef _NAVGEN
	Commentary::instance().addMsg( msg, isCritical ? Commentary::CRITICAL : Commentary::COMMENT );
#endif//_NAVGEN
}


void BWLockDConnection::rebuildGridStatus()
{
	BW_GUARD;

	xMin_ = zMin_ = std::numeric_limits<short>::max();
	xMax_ = zMax_ = std::numeric_limits<short>::min();

	if( computers_.empty() )
		return;

	// first get the min/max value of each dimension
	for( BW::vector<Computer>::const_iterator iter = computers_.begin(); iter != computers_.end(); ++iter )
	{
		for( BW::vector<Lock>::const_iterator it = iter->locks_.begin();
			it != iter->locks_.end(); ++it )
		{
			if( xMin_ >= it->rect_.left_ )
				xMin_ = it->rect_.left_;
			if( zMin_ >= it->rect_.top_ )
				zMin_ = it->rect_.top_;
			if( xMax_ <= it->rect_.right_ )
				xMax_ = it->rect_.right_;
			if( zMax_ <= it->rect_.bottom_ )
				zMax_ = it->rect_.bottom_;
		}
	}

	// fill the grid info
	gridStatus_.assign( ( xMax_ - xMin_ + 1 ) * ( zMax_ - zMin_ + 1 ), GS_NOT_LOCKED );
	for( BW::vector<Computer>::const_iterator iter = computers_.begin(); iter != computers_.end(); ++iter )
	{
		bool me = stricmp( iter->name_.c_str(), self_.c_str() ) == 0;
		for( BW::vector<Lock>::const_iterator it = iter->locks_.begin();
			it != iter->locks_.end(); ++it )
		{
			for( short z = it->rect_.top_; z <= it->rect_.bottom_; ++z )
			{
				int start = ( z - zMin_ ) * ( xMax_ - xMin_ + 1 );
				for( short x = it->rect_.left_; x <= it->rect_.right_; ++x )
				{
					if( me )
						gridStatus_[ start + ( x - xMin_ ) ] = GS_LOCKED_BY_ME;
					else
						gridStatus_[ start + ( x - xMin_ ) ] = GS_LOCKED_BY_OTHERS;
				}
			}
		}
	}

	// fill writable info
	for( int z = zMin_; z <= zMax_; ++z )
	{
		int start = ( z - zMin_ ) * ( xMax_ - xMin_ + 1 );
		for( int x = xMin_; x <= xMax_; ++x )
		{
			bool writable = true;
			for( int i = -xExtent_; i <= xExtent_ && writable; ++i )
			{
				for( int j = -zExtent_; j <= zExtent_; ++j )
				{
					int curX = x + i;
					int curZ = z + j;

					if( curX < xMin_ || curX > xMax_ ||
						curZ < zMin_ || curZ > zMax_ )
					{
						writable = false;
						break;
					}
					if( !isLockedByMe( curX, curZ ) )
					{
						writable = false;
						break;
					}
				}
			}
			if( writable )
				gridStatus_[ start + ( x - xMin_ ) ] = GS_WRITABLE_BY_ME;
		}
	}
}

};

BW_END_NAMESPACE
