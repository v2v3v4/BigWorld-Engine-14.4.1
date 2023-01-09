#include "script/first_include.hpp"

#include "entity_channel_finder.hpp"

#include "baseapp.hpp"
#include "base.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityChannelFinder
// -----------------------------------------------------------------------------

/**
 *	This method finds the entity channel associated with a channel id.
 */
Mercury::UDPChannel * EntityChannelFinder::find( Mercury::ChannelID id,
		const Mercury::Address & srcAddr,
		const Mercury::Packet * pPacket,
		bool & rHasBeenHandled )
{
	if (BaseApp::pInstance() == NULL)
	{
		WARNING_MSG("EntityChannelFinder::find:"
						"BaseApp has already shutdown\n");
		return NULL;
	}

	Base * pEntity = BaseApp::instance().bases().findEntity( (EntityID)id );

	if (pEntity != NULL)
	{
		return &pEntity->channel();
	}
	else
	{
		if (BaseApp::instance().entityWasOffloaded( id ))
		{
			// Drop entity channel packets destined for offloaded entities.
			// These will be resent by the cell entity as per the normal
			// situation when packets are dropped.
			rHasBeenHandled = true;
		}

		return NULL;
	}
}

BW_END_NAMESPACE

// entity_channel_finder.cpp
