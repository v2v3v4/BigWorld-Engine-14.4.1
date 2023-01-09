#ifndef MESSAGE_FILTER_HPP
#define MESSAGE_FILTER_HPP

#include "network/mercury.hpp"
using namespace Mercury;

namespace Testing
{
	
class FilterFunctionTable;

/**
 * A PacketFilter that understands all Mercury headers and footers and can
 * process just the message payloads.  The actual processing is handled by the
 * methods contained within the FilterFunctionTable that must be passed to the
 * MessageFilter when constructed.
 */
class MessageFilter : public PacketFilter
{
public:
	MessageFilter( ChannelData &data, uint8 flags );
	virtual int send( Packet *p );
	virtual Reason recv( Packet *p );

protected:
	virtual void addMessages( Packet *p ) {}
	void filter( Packet *p );
	char* findBack( Packet *p );
	char* reserveMessage( Packet *p, int length );
	FilterFunctionTable *pFuncTable_;
	uint8 flags_;
	BW::vector<uint8> seen_;

	enum { PASS_THROUGH = 0x0,
		   FILTER_ON_SEND = 0x1,
		   FILTER_ON_RECV = 0x2,
		   ADD_PRE_SEND = 0x4,
		   ADD_PRE_RECV = 0x8,
		   ADD_POST_SEND = 0x10,
		   ADD_POST_RECV = 0x20 };
};

// Functions which alter message payloads
typedef void (*FilterFunction)( uint8 id,
								const InterfaceElement &ie,
								void *data,
								size_t length );

/**
 * A table that maps message IDs to a FilterFunction and an InterfaceElement.
 */
class FilterFunctionTable
{
public:
	FilterFunctionTable( InterfaceMinder &minder );
	void filter( uint8 id, void *data, size_t length );
	
	void setFilterFunction( uint8 id, FilterFunction func );
	void setInterfaceElement( uint8 id, const InterfaceElement &ie );
	const InterfaceElement &getInterfaceElement( uint8 id );
	FilterFunction getFilterFunction( uint8 id );
	
	void aliasSlot( uint8 from, uint8 to );
	int interfaceSize() { return interfaceSize_; }

private:
	int interfaceSize_;
	FilterFunction filterFuncs_[ 256 ];
	const InterfaceElement* interfaceElements_[ 256 ];
};

/**
 * Utility functions
 */
void dump( void *data, int nBytes, char format );

}; // namespace Testing

#endif // MESSAGE_FILTER_HPP
