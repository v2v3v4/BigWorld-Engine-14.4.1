#include "server/backup_hash_chain.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/binary_stream.hpp"

#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
BackupHashChain::BackupHashChain():
		history_()
{
}


/**
 *	Destructor. 
 */
BackupHashChain::~BackupHashChain()
{
}


/**
 *	Add an app failure to this hash chain.
 */
void BackupHashChain::adjustForDeadBaseApp( const Mercury::Address & deadApp, 
											const BackupHash & hash )
{
	history_.insert( std::make_pair( deadApp, hash ) );
}


/**
 *	Resolve an address for a potentially dead app address through this hash
 *	chain.
 */
Mercury::Address 
BackupHashChain::addressFor( const Mercury::Address & address,
							 EntityID entityID ) const
{
	// Quickly look up the address to see if it needs redirection
	History::const_iterator firstMatch = history_.find( address );
	// If we don't know about any crash at that address, it's valid.
	if (firstMatch == history_.end())
	{
		return address;
	}

	// Leave a trail to prevent infinite recursion
	BW::set< Mercury::Address > visited;

	// Keep the first match around for the failure case.
	History::const_iterator match = firstMatch;
	Mercury::Address position = address;

	// Iteration is easier to track loops than recursion, also could be
	// implemented by passing visited up the call chain.
	while (match != history_.end())
	{
		if (visited.count( position ))
		{
			WARNING_MSG( "BackupHashChain::addressFor( %s, %u ): "
						 "Infinite loop\n",
						 address.c_str(), entityID );
			return address;
		}
		visited.insert( position );

		// Do the same thing again to find out where it would have gone.
		position = match->second.addressFor( entityID );
		match = history_.find( position );

	}

	// We've encountered a live app, hurrah! 
	return position;
}


/**
 *	Streaming operator for BackupHashChain.
 *
 *	@param os			The output binary stream.
 *	@param hashChain	The hash chain to stream.
 *
 *	@return 			The reference to the output binary stream.
 */
BinaryOStream & operator<<( BinaryOStream & os,
		const BackupHashChain & hashChain )
{
	os << uint16( hashChain.history_.size() );
	typedef BackupHashChain::History History;
	History::const_iterator iHash = hashChain.history_.begin();

	while (iHash != hashChain.history_.end())
	{
		os << iHash->first << iHash->second;
		++iHash;
	}

	return os;
}


/**
 *	De-streaming operator for BackupHashChain.
 *
 *	@param is 			The input binary stream.
 *	@param hashChain 	The hash chain to de-stream.
 *
 *	@return 			The reference to the input binary stream.
 */
BinaryIStream & operator>>( BinaryIStream & is, 
		BackupHashChain & hashChain )
{
	hashChain.history_.clear();

	uint16 numHashChainEntries;
	is >> numHashChainEntries;

	while (numHashChainEntries--)
	{
		Mercury::Address addr;
		BackupHash hash;

		is >> addr >> hash;

		hashChain.history_.insert( std::make_pair( addr, hash ) );
	}

	return is;
}

BW_END_NAMESPACE

// backup_hash_chain.cpp

