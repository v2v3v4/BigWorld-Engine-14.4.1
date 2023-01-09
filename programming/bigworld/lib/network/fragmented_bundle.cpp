#include "pch.hpp"

#include "fragmented_bundle.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor.
 */
FragmentedBundle::FragmentedBundle( Packet * pFirstPacket ) :
	lastFragment_( pFirstPacket->fragEnd() ),
	// Remaining is one less than the size
	remaining_( pFirstPacket->fragEnd() - pFirstPacket->fragBegin() ),
	touched_( timestamp() ),
	pChain_( pFirstPacket )
{
}


/**
 *	This method adds a packet to this fragmented bundle.
 */
bool FragmentedBundle::addPacket( Packet * p,
		bool isExternal, const char * sourceStr )
{
	// If the last fragment seqnums for existing packets in this bundle
	// and this one don't match up, we can't process it.
	if ((pChain_->fragBegin() != p->fragBegin()) ||
			(pChain_->fragEnd() != p->fragEnd()))
	{
		// If the incoming fragment is unreliable, then we're still waiting
		// for the reliable bundle attached to this to complete.
		if (!p->hasFlags( Packet::FLAG_IS_RELIABLE ))
		{
			MF_ASSERT( this->isReliable() || isExternal );

			WARNING_MSG( "FragmentedBundle::addPacket( %s ): "
					"Discarding unreliable fragment #%u (#%u,#%u) while "
					"waiting for reliable chain (#%u,#%u) to complete\n",
				sourceStr, p->seq(), p->fragBegin(), p->fragEnd(),
				pChain_->fragBegin(), pChain_->fragEnd() );

			return true;
		}

		// If we're on an external interface, then it could be someone mangling
		// the packets on purpose.
		if (isExternal)
		{
			WARNING_MSG( "FragmentedBundle::addPacket( %s ): "
					"Mangled fragment footers on packet #%u, got [%u,%u], "
					"expecting [%u,%u]\n",
				sourceStr, p->seq(),
				p->fragBegin(), p->fragEnd(),
				pChain_->fragBegin(), pChain_->fragEnd() );

		}
		else
		{
			// We should never get this for reliable traffic on internal
			// interfaces.
			CRITICAL_MSG( "FragmentedBundle::addPacket( %s ): "
					"Mangled fragment footers on packet #%u, got [%u,%u], "
					"expecting [%u,%u]\n",
				sourceStr, p->seq(),
				p->fragBegin(), p->fragEnd(),
				pChain_->fragBegin(), pChain_->fragEnd() );
		}
		return false;
	}

	touched_ = timestamp();

	// find where this goes in the chain
	Packet * pre = NULL;
	Packet * walk;

	for (walk = pChain_.get(); walk; walk = walk->next())
	{
		// If p is already in this chain, bail now
		if (walk->seq() == p->seq())
		{
			WARNING_MSG( "FragmentedBundle::addPacket( %s ): "
				"Discarding duplicate fragment #%u\n",
				sourceStr, p->seq() );

			return true;
		}

		// Stop when 'walk' is the packet in the chain after p.
		if (seqLessThan( p->seq(), walk->seq() ))
		{
			break;
		}

		pre = walk;
	}

	// add it to the chain
	p->chain( walk );

	if (pre == NULL)
	{
		pChain_ = p; // chain -> p -> walk
	}
	else
	{
		pre->chain( p ); // chain -> ... -> pre -> p -> walk
	}

	MF_ASSERT( remaining_ > 0 );

	--remaining_;

	return true;
}


/**
 *	This static method adds the given fragmented bundle to the given stream.
 *	This can later be retrieved using FragmentedBundle::createFromStream.
 */
void FragmentedBundle::addToStream( FragmentedBundlePtr pFragments,
		BinaryOStream & data )
{
	// Stream on chained fragments
	if (pFragments)
	{
		data << (uint16)pFragments->pChain_->chainLength();

		for (const Packet * p = pFragments->pChain_.get();
			 p != NULL; p = p->next())
		{
			Packet::addToStream( data, p, Packet::CHAINED_FRAGMENT );
		}
	}
	else
	{
		data << (uint16)0;
	}
}


/**
 *	This static method creates a fragmented bundle from the given stream.
 *	The stream should have been created with FragmentedBundle::addToStream.
 */
FragmentedBundlePtr FragmentedBundle::createFromStream( BinaryIStream & data )
{
	// Destream any chained fragments
	uint16 numChainedFragments;

	data >> numChainedFragments;

	FragmentedBundlePtr pFragments = NULL;

	for (uint16 i = 0; i < numChainedFragments; i++)
	{
		PacketPtr pPacket =
			Packet::createFromStream( data, Packet::CHAINED_FRAGMENT );

		if (pFragments == NULL)
		{
			pFragments = new FragmentedBundle( pPacket.get() );
		}
		else
		{
			pFragments->addPacket( pPacket.get(),
					/*isExternal*/false, "STREAMED" );
		}
	}

	return pFragments;
}

} // namespace Mercury

BW_END_NAMESPACE

// fragmented_bundle.cpp
