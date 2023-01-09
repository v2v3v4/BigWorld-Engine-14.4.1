#include "baseapp_message_filter.hpp"
#include "mangle.hpp"
#include "common/baseapp_ext_interface.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

namespace Testing
{

// ----------------------------------------------------------------
// Section: Message Handlers
// ----------------------------------------------------------------
void mangle( uint8 id,
			 const InterfaceElement &ie,
			 void *data,
			 size_t length )
{
	//HACK_MSG( "Scrambling random byte in message %d\n", id );
	mangleSingleByte( data, length );
}

void mangleMaybe( uint8 id,
				  const InterfaceElement &ie,
				  void *data,
				  size_t length )
{
	if (unlucky())
		mangle( id, ie, data, length );
}

// ----------------------------------------------------------------
// Section: BaseAppMessageFilter
// ----------------------------------------------------------------
FilterFunctionTable *BaseAppMessageFilter::funcTable = NULL;

BaseAppMessageFilter::BaseAppMessageFilter( ChannelData &data ) :
	MessageFilter( data, FILTER_ON_RECV )
{
}

PacketFilter* BaseAppMessageFilter::create( ChannelData &data )
{
	BaseAppMessageFilter *filt = new BaseAppMessageFilter( data );

	// Create the function table if it doesn't exist yet
	if (funcTable == NULL)
	{
		funcTable = new FilterFunctionTable( BaseAppExtInterface::gMinder );

		// Map handled messages
		for (int i=0; i < 15; i++)
			funcTable->setFilterFunction( i, &mangleMaybe );

		// Entity messages are always scrambled
		funcTable->setFilterFunction( 15, &mangle );
		
		// Map all message id's above 128 to the handler for message 15
		// (entityMessage)
		for (int id=128; id < 256; id++)
			funcTable->aliasSlot( id, 15 );
	}

	filt->pFuncTable_ = funcTable;

	return filt;
}

}; // namespace Testing

// BASEAPP_MESSAGE_FILTER_CPP
