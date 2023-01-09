#include "message_filter.hpp"
#include "common/baseapp_ext_interface.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

namespace Testing
{

// -----------------------------------------------------------------------------
// Section: MessageFilter
// -----------------------------------------------------------------------------

/**
 * Creates a new MessageFilter with the given flags (which is a bitwise
 * combination of FILTER_ON_SEND and FILTER_ON_RECV)
 */
MessageFilter::MessageFilter( ChannelData &data, uint8 flags ) :
	PacketFilter( data ),
	pFuncTable_(),
	flags_( flags )
{
}

int MessageFilter::send( Packet *p )
{
	if (flags_ & (ADD_PRE_SEND | ADD_POST_SEND | FILTER_ON_SEND ))
		INFO_MSG( "==========\n" );
	
	if (flags_ & ADD_PRE_SEND)
		addMessages( p );
	
	if (flags_ & FILTER_ON_SEND)
		filter( p );
		
	if (flags_ & ADD_POST_SEND)
		addMessages( p );
	
	return PacketFilter::send( p );
}

Reason MessageFilter::recv( Packet *p )
{
	if (flags_ & (ADD_PRE_RECV | ADD_POST_RECV | FILTER_ON_RECV ))	
		INFO_MSG( "==========\n" );
	
	if (flags_ & ADD_PRE_RECV)
		addMessages( p );
	
	if (flags_ & FILTER_ON_RECV)
		filter( p );
		
	if (flags_ & ADD_POST_RECV)
		addMessages( p );

	return PacketFilter::recv( p );
}

/**
 * Applies registered message filters to the messages in this packet.
 */
void MessageFilter::filter( Packet *p )
{
	uint8 flags = p->data[0];
	char *data = p->data + 1;
	char *back = findBack( p );
	
	INFO_MSG( "Flags: %hhx Length: %d Payload: %d Max: %d Source: %s\n", flags,
			  p->curlen,(int)(back - data), p->maxlen, (char*)data_.addr() );

	// Process messages
	while (data < back)
	{
		// Read out the message ID
		uint8 messageID = *(uint8*)data;

		// Get the corresponding interface element
 		const InterfaceElement &ie =
			pFuncTable_->getInterfaceElement( messageID );

		// Determine the message length and move pointer past header
		int length = ie.expandLength( data );
		data += ie.headerSize();
		
		// Do something with the message data
		INFO_MSG( "Filtering msg id %d (payload %d bytes)\n",
				  messageID, length );
		pFuncTable_->filter( messageID, data, length );

		// Remember that we've seen this type of message
		BW::vector<uint8>::iterator b = seen_.begin(), e = seen_.end();
		if (std::find( b, e, messageID ) == e)
		{
			INFO_MSG( "Saw message ID %d for the first time:\n", messageID );
			dump( data, length, 'd' );
			seen_.push_back( messageID );
		}
// 		if (!std::binary_search( b, e, messageID ))
// 		{
// 			INFO_MSG( "Saw message ID %d for the first time:\n", messageID );
// 			dump( data, length, 'd' );
// 			seen_.insert( std::lower_bound( b, e, messageID ), messageID );
// 		}

		// Cycle data pointer to next message
		data += length;
	}	

	MF_ASSERT( data == back );

	// Print the messages we've seen so far
	BW::string s;
	for (uint i=0; i < seen_.size(); i++)
	{
		char buf[32];
		sprintf( buf, "%d ", seen_[i] );
		s.append( buf );
	}
	INFO_MSG( "Seen message IDs (%d): %s\n", seen_.size(), s.c_str() );
}

/**
 * Returns a pointer to the start of the packet's footers.  Doesn't alter
 * anything.  If for some reason the firstRequestOffset is needed in the calling
 * context of this method, the return pointer will be positioned at it (assuming
 * FLAG_HAS_REQUESTS is set).
 */
char* MessageFilter::findBack( Packet *p )
{
	uint8 flags = p->data[0];
	char *back = p->data + p->curlen;
	
	// Macro for stripping data off the front and back of the packet
#	define READ_FOOTER( TYPE )	*( TYPE* )( back -= sizeof( TYPE ) )

	if (flags & Bundle::FLAG_HAS_ACKS)
	{
		// Take off the ack count and then cycle back pointer back accordingly
		uint8 cAck = READ_FOOTER( uint8 );
		back -= cAck * sizeof( uint32 );
	}

	if (flags & Bundle::FLAG_HAS_SEQUENCE_NUMBER)
	{
		// Had to comment this out to avoid unused variable warning
		//uint32 seq = READ_FOOTER( uint32 );
		READ_FOOTER( uint32 );
	}

	if (flags & Bundle::FLAG_IS_FRAGMENT)
	{
		int firstSeq = READ_FOOTER( int );
		int lastSeq = READ_FOOTER( int );
		INFO_MSG( "frag seq limits are %d/%d\n", firstSeq, lastSeq );
	}

	if (flags & Bundle::FLAG_HAS_REQUESTS )
	{
		uint16 firstReqOffset = READ_FOOTER( uint16 );
		INFO_MSG( "FIRST REQ OFFSET IS %d\n", firstReqOffset );
		MF_ASSERT( !"Request handling not implemented yet, better do it now" );
	}

	return back;
}

/**
 * Makes space at the end of the messages on a packet (ie right where the
 * footers used to be) for a new message of the given size.  Returns a pointer
 * to the space where the message should be inserted
 */
char* MessageFilter::reserveMessage( Packet *p, int length )
{
	char *back = findBack( p );
	int footerLength = (int)(p->data + p->curlen - (int)back);

	// Make sure there's enough room
	if (p->curlen + length > p->maxlen)
	{
		WARNING_MSG( "Couldn't reserve %d bytes for a new message!\n", length );
		return NULL;
	}
	
	// Move the footers down by the required amount
	memmove( back + length, back, footerLength );

	// Update packet fields
	p->curlen += length;

	// New message will be written in where footers used to be
	return back;
}

// -----------------------------------------------------------------------------
// Section: FilterFunctionTable
// -----------------------------------------------------------------------------

FilterFunctionTable::FilterFunctionTable( InterfaceMinder &minder )
{
	// Zero out all tables
	for (int i=0; i < 256; i++)
	{
		interfaceElements_[i] = NULL;
		filterFuncs_[i] = NULL;
	}
	
	// Set up all the interface elements for the registered mercury interface
	InterfaceMinder::iterator iter = minder.begin();
	InterfaceMinder::iterator end = minder.end();
	uint8 id = 0;

	for (; iter != end; iter++, id++)
		interfaceElements_[ id ] = &(*iter);

	interfaceSize_ = id;
}

// Returns the InterfaceElement that this table thinks corresponds to the given
// id
const InterfaceElement& FilterFunctionTable::getInterfaceElement( uint8 id )
{
	const InterfaceElement *iep = interfaceElements_[ id ];
	MF_ASSERT( iep != NULL );
	return *iep;
}

// Returns the InterfaceElement that this table thinks corresponds to the given
// id
FilterFunction FilterFunctionTable::getFilterFunction( uint8 id )
{
	FilterFunction pfunc = filterFuncs_[ id ];
	MF_ASSERT( pfunc != NULL );
	return pfunc;
}

void FilterFunctionTable::setFilterFunction( uint8 id, FilterFunction func )
{
	filterFuncs_[ id ] = func;
}

void FilterFunctionTable::setInterfaceElement( uint8 id, const InterfaceElement &ie )
{
	interfaceElements_[ id ] = &ie;
}

void FilterFunctionTable::filter( uint8 id, void *data, size_t length )
{
	FilterFunction func = filterFuncs_[ id ];
	if (func != NULL)
	{
		MF_ASSERT( interfaceElements_[ id ] != NULL );
		(*func)( id, *interfaceElements_[ id ], data, length );
	}
}

void FilterFunctionTable::aliasSlot( uint8 from, uint8 to )
{
	filterFuncs_[ from ] = filterFuncs_[ to ];
	interfaceElements_[ from ] = interfaceElements_[ to ];
}

// Format should either be 'd' or 'x' for decimal or hex
void dump( void *data, int nBytes, char format )
{
	BW::string s = "";
	uint8 *p = (uint8*)data;
	char buf[16];
	char fmt[16];

	sprintf( fmt, "%%0hh%c ", format );

	for (int i=0; i < nBytes; i++, p++)
	{
		sprintf( buf, fmt, *p );
		s.append( buf );
	}

	INFO_MSG("RAW: %s\n", s.c_str() );
}



}; // namespace Testing

// MESSAGE_FILTER_CPP
